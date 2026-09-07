#include "decision_log_writer.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <variant>

namespace riftbound {

namespace {

const char* playSourceToString(Intent::PlaySource s) {
    switch (s) {
        case Intent::PlaySource::Hand:         return "Hand";
        case Intent::PlaySource::Trash:        return "Trash";
        case Intent::PlaySource::Banishment:   return "Banishment";
        case Intent::PlaySource::ChampionZone: return "ChampionZone";
        case Intent::PlaySource::Hidden:       return "Hidden";
        case Intent::PlaySource::ChainZone:    return "ChainZone";
    }
    return "Unknown";
}

const char* flowSourceToString(Intent::FlowSource s) {
    switch (s) {
        case Intent::FlowSource::None:     return "None";
        case Intent::FlowSource::Printed:  return "Printed";
        case Intent::FlowSource::Granted:  return "Granted";
    }
    return "Unknown";
}

nlohmann::json locationToJson(const std::optional<LocationId>& loc) {
    if (!loc.has_value()) return nullptr;
    if (const auto* bf = std::get_if<BattlefieldLocation>(&*loc)) {
        return nlohmann::json{{"type", "battlefield"}, {"id", bf->id}};
    }
    if (const auto* base = std::get_if<BaseLocation>(&*loc)) {
        return nlohmann::json{{"type", "base"}, {"player", std::string(toString(base->player))}};
    }
    return nullptr;
}

std::string basename(const std::string& path) {
    if (path.empty()) return path;
    return std::filesystem::path(path).filename().string();
}

} // namespace

DecisionLogWriter::DecisionLogWriter(const std::string& output_path,
                                     DecisionLogHeader header)
    : header_(std::move(header)) {
    auto parent = std::filesystem::path(output_path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);

    file_.open(output_path);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open decision log file: " + output_path);
    }

    nlohmann::json decks;
    decks["p1"] = nlohmann::json{{"path", header_.deck1.path},
                                  {"legend", header_.deck1.legend},
                                  {"champion", header_.deck1.champion}};
    decks["p2"] = nlohmann::json{{"path", header_.deck2.path},
                                  {"legend", header_.deck2.legend},
                                  {"champion", header_.deck2.champion}};

    nlohmann::json agents{{"p1", header_.agent1_spec}, {"p2", header_.agent2_spec}};

    // Seat map (deck -> P1/P2), keyed by deck-file basename so the analysis
    // script can group by physical deck across games where seats swap.
    nlohmann::json seats;
    seats[basename(header_.deck1.path)] = "P1";
    seats[basename(header_.deck2.path)] = "P2";

    file_ << "{\n";
    file_ << "\"schema\":1,\n";
    file_ << "\"decks\":" << decks.dump() << ",\n";
    file_ << "\"agents\":" << agents.dump() << ",\n";
    file_ << "\"seed\":" << header_.seed << ",\n";
    file_ << "\"seats\":" << seats.dump() << ",\n";
    file_ << "\"engine_version\":" << nlohmann::json(header_.engine_version).dump() << ",\n";
    file_ << "\"log\":[\n";
    file_.flush();
}

DecisionLogWriter::~DecisionLogWriter() {
    if (!finished_ && file_.is_open()) {
        file_ << "\n],\n\"truncated\":true\n}\n";
        file_.flush();
    }
}

void DecisionLogWriter::setNextRootValue(double value) {
    pending_root_value_ = value;
}

void DecisionLogWriter::recordDecision(const GameState& state,
                                       const std::vector<Intent>& legal_actions,
                                       const Intent& chosen_action) {
    if (finished_) return;

    nlohmann::json rec;
    rec["idx"] = state.decision_index;
    rec["turn"] = state.turn.turn_number;
    rec["phase"] = nlohmann::json{
        {"phase", toString(state.turn.phase)},
        {"ns_state", state.turn.ns_state == NeutralShowdownState::Showdown
                         ? "Showdown" : "Neutral"},
        {"oc_state", state.turn.oc_state == OpenClosedState::Closed
                         ? "Closed" : "Open"},
    };
    rec["actor"] = std::string(toString(chosen_action.player));
    rec["scores"] = nlohmann::json::array({state.players[0].score, state.players[1].score});

    nlohmann::json players = nlohmann::json::array();
    for (PlayerId p : {PlayerId::Player1, PlayerId::Player2}) {
        const auto& ps = state.player(p);
        int runes_ready = 0, runes_exhausted = 0;
        for (auto rid : state.runesInBase(p)) {
            if (!state.objectExists(rid)) continue;
            if (state.getObject(rid).is_exhausted) ++runes_exhausted;
            else ++runes_ready;
        }
        bool legend_empowered = false;
        if (ps.legend_zone != kInvalidId && state.objectExists(ps.legend_zone)) {
            legend_empowered = state.getObject(ps.legend_zone).is_empowered;
        }
        players.push_back(nlohmann::json{
            {"seat", std::string(toString(p))},
            {"hand", ps.hand.size()},
            {"deck", ps.main_deck.size()},
            {"trash", ps.trash.size()},
            {"banishment", ps.banishment.size()},
            {"runes_ready", runes_ready},
            {"runes_exhausted", runes_exhausted},
            {"legend_empowered", legend_empowered},
        });
    }
    rec["players"] = players;

    nlohmann::json battlefields = nlohmann::json::array();
    for (const auto& bf : state.battlefields) {
        std::string name;
        if (bf.card_object_id != kInvalidId && state.objectExists(bf.card_object_id)) {
            name = state.getObject(bf.card_object_id).name;
        }
        auto unitsSummary = [&](PlayerId p) {
            auto units = state.unitsAt(BattlefieldLocation{bf.id}, p);
            int might = 0;
            for (auto uid : units) {
                if (state.objectExists(uid)) might += state.getObject(uid).current_might;
            }
            return nlohmann::json{{"count", units.size()}, {"might", might}};
        };
        battlefields.push_back(nlohmann::json{
            {"id", bf.id},
            {"name", name},
            {"controller", bf.controller.has_value()
                                ? std::string(toString(*bf.controller))
                                : std::string("None")},
            {"contested", bf.is_contested},
            {"units", nlohmann::json{{"p1", unitsSummary(PlayerId::Player1)},
                                      {"p2", unitsSummary(PlayerId::Player2)}}},
        });
    }
    rec["battlefields"] = battlefields;

    rec["legal_count"] = legal_actions.size();

    nlohmann::json chosen;
    chosen["type"] = std::string(toString(chosen_action.type));
    if (chosen_action.card != kInvalidId && state.objectExists(chosen_action.card)) {
        chosen["card"] = state.getObject(chosen_action.card).name;
    } else {
        chosen["card"] = nullptr;
    }
    chosen["source_zone"] = playSourceToString(chosen_action.play_source);
    chosen["flow_source"] = flowSourceToString(chosen_action.flow_source);
    if (chosen_action.target_battlefield_restriction.has_value()) {
        chosen["restricted_bf"] = *chosen_action.target_battlefield_restriction;
    } else {
        chosen["restricted_bf"] = nullptr;
    }
    // Destination battlefield for moves/plays: StandardMove carries
    // move_destination, PlayCard carries play_location. At most one is
    // ever set on a given intent.
    chosen["destination"] = locationToJson(chosen_action.move_destination.has_value()
                                                ? chosen_action.move_destination
                                                : chosen_action.play_location);
    rec["chosen"] = chosen;

    if (pending_root_value_.has_value()) {
        rec["root_value"] = *pending_root_value_;
        pending_root_value_.reset();
    } else {
        rec["root_value"] = nullptr;
    }

    if (wrote_first_) file_ << ",\n";
    wrote_first_ = true;
    file_ << rec.dump();
    file_.flush();
}

std::string DecisionLogWriter::winnerDeckName(PlayerId winner) const {
    if (winner == PlayerId::Player1) {
        return header_.deck1.legend + " / " + header_.deck1.champion;
    }
    if (winner == PlayerId::Player2) {
        return header_.deck2.legend + " / " + header_.deck2.champion;
    }
    return "";
}

void DecisionLogWriter::finish(PlayerId winner,
                               const std::string& reason,
                               int turns,
                               const int final_scores[2],
                               int decision_count) {
    if (finished_) return;

    file_ << "\n],\n";

    nlohmann::json footer;
    footer["winner"] = nlohmann::json{{"deck", winnerDeckName(winner)},
                                       {"seat", std::string(toString(winner))}};
    footer["reason"] = reason;
    footer["turns"] = turns;
    footer["final_scores"] = nlohmann::json::array({final_scores[0], final_scores[1]});
    footer["decisions"] = decision_count;

    // footer.dump() is a self-contained "{...}" object; splice its inner
    // key:value list (order-independent, comma-correct by construction)
    // directly into our own top-level object rather than nesting it under
    // another key.
    std::string body = footer.dump();
    file_ << body.substr(1, body.size() - 2) << "\n}\n";
    file_.flush();
    finished_ = true;
}

} // namespace riftbound
