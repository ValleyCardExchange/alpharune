#include "agent_spec.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace riftbound {

const char* toString(EvaluatorKind k) {
    switch (k) {
        case EvaluatorKind::Score:  return "score";
        case EvaluatorKind::Corpus: return "corpus";
    }
    return "score";
}

double intentFamilyWeight(IntentType type, bool moves_to_battlefield,
                          const FamilyWeights& family) {
    switch (type) {
        case IntentType::PlayCard:
        case IntentType::PlayReaction:
        case IntentType::PlayActionCard:
            return family.play;
        case IntentType::AssignCombatDamage:
            return family.combat_damage;
        case IntentType::StandardMove:
            // Move to a battlefield is meaningful; move to base is
            // usually retreating / shuffling.
            return moves_to_battlefield ? family.move_to_battlefield
                                        : family.move_to_base;
        case IntentType::ActivateAbility:
        case IntentType::ActivateReactionAbility:
        case IntentType::ActivateActionAbility:
            return family.activate;
        case IntentType::MakeChoice:
            return family.choice;
        case IntentType::MulliganDecision:
        case IntentType::ChooseBattlefield:
        case IntentType::PlayFirstDecision:
            return family.setup;
        case IntentType::EndTurn:
        case IntentType::PassPriority:
        case IntentType::PassFocus:
            return family.pass;
        case IntentType::Concede:
            return family.concede;
        default:
            // Not part of schema v1 — a handful of triggered-ability
            // response types with no evidence either way. Same neutral
            // weight every unclassified legal action got before this
            // refactor.
            return 1.0;
    }
}

namespace {

// ── Schema v1 key sets, for the "unknown keys are an error" guard ──────────

const std::set<std::string>& topLevelKeys() {
    static const std::set<std::string> keys = {
        "schema", "matchup", "written",
        "action_family_weights", "evaluator_weights"};
    return keys;
}

const std::set<std::string>& familyKeys() {
    static const std::set<std::string> keys = {
        "play", "combat_damage", "move_to_battlefield", "move_to_base",
        "activate", "choice", "setup", "pass", "concede"};
    return keys;
}

const std::set<std::string>& evaluatorKeys() {
    static const std::set<std::string> keys = {
        "score", "battlefield", "unit", "held_interaction",
        "trash_resource", "empowered_legend"};
    return keys;
}

void rejectUnknownKeys(const nlohmann::json& obj, const std::set<std::string>& allowed,
                       const std::string& path, const std::string& section) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!allowed.count(it.key())) {
            std::ostringstream msg;
            msg << "prior file '" << path << "' has unknown key '";
            if (!section.empty()) msg << section << ".";
            msg << it.key() << "'.";
            throw std::runtime_error(msg.str());
        }
    }
}

}  // namespace

PriorConfig loadPriorConfig(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error(
            "prior file '" + path + "' does not exist or is not readable.");
    }

    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            "prior file '" + path + "' is not valid JSON: " + e.what());
    }
    if (!j.is_object()) {
        throw std::runtime_error(
            "prior file '" + path + "' must contain a JSON object.");
    }

    rejectUnknownKeys(j, topLevelKeys(), path, "");

    PriorConfig out;

    if (j.contains("action_family_weights")) {
        const auto& fw = j.at("action_family_weights");
        if (!fw.is_object()) {
            throw std::runtime_error(
                "prior file '" + path + "': 'action_family_weights' must be an object.");
        }
        rejectUnknownKeys(fw, familyKeys(), path, "action_family_weights");
        if (fw.contains("play")) out.family.play = fw.at("play").get<double>();
        if (fw.contains("combat_damage"))
            out.family.combat_damage = fw.at("combat_damage").get<double>();
        if (fw.contains("move_to_battlefield"))
            out.family.move_to_battlefield = fw.at("move_to_battlefield").get<double>();
        if (fw.contains("move_to_base"))
            out.family.move_to_base = fw.at("move_to_base").get<double>();
        if (fw.contains("activate")) out.family.activate = fw.at("activate").get<double>();
        if (fw.contains("choice")) out.family.choice = fw.at("choice").get<double>();
        if (fw.contains("setup")) out.family.setup = fw.at("setup").get<double>();
        if (fw.contains("pass")) out.family.pass = fw.at("pass").get<double>();
        if (fw.contains("concede")) out.family.concede = fw.at("concede").get<double>();
    }

    if (j.contains("evaluator_weights")) {
        const auto& ew = j.at("evaluator_weights");
        if (!ew.is_object()) {
            throw std::runtime_error(
                "prior file '" + path + "': 'evaluator_weights' must be an object.");
        }
        rejectUnknownKeys(ew, evaluatorKeys(), path, "evaluator_weights");
        if (ew.contains("score")) out.evaluator.score = ew.at("score").get<double>();
        if (ew.contains("battlefield"))
            out.evaluator.battlefield = ew.at("battlefield").get<double>();
        if (ew.contains("unit")) out.evaluator.unit = ew.at("unit").get<double>();
        if (ew.contains("held_interaction"))
            out.evaluator.held_interaction = ew.at("held_interaction").get<double>();
        if (ew.contains("trash_resource"))
            out.evaluator.trash_resource = ew.at("trash_resource").get<double>();
        if (ew.contains("empowered_legend"))
            out.evaluator.empowered_legend = ew.at("empowered_legend").get<double>();
    }

    return out;
}

AgentSpec parseAgentSpec(const std::string& s) {
    AgentSpec out;
    out.raw = s;
    auto colon = s.find(':');
    out.kind = (colon == std::string::npos) ? s : s.substr(0, colon);
    if (out.kind != "random" && out.kind != "human" &&
        out.kind != "mcts" && out.kind != "ismcts") {
        throw std::runtime_error(
            "Unknown agent kind '" + out.kind + "'. Expected one of: "
            "random, human, mcts, ismcts.");
    }
    if (colon != std::string::npos) {
        auto rest = s.substr(colon + 1);
        // key=value pairs separated by commas. Unrecognised keys are
        // ignored (pre-existing behaviour); recognised keys with a bad
        // value are rejected.
        size_t i = 0;
        while (i < rest.size()) {
            auto eq    = rest.find('=', i);
            auto comma = rest.find(',', i);
            if (comma == std::string::npos) comma = rest.size();
            if (eq != std::string::npos && eq < comma) {
                auto key = rest.substr(i, eq - i);
                auto val = rest.substr(eq + 1, comma - eq - 1);
                if (key == "sims") out.sims = std::stoi(val);
                if (key == "eval") {
                    if      (val == "score")  out.eval = EvaluatorKind::Score;
                    else if (val == "corpus") out.eval = EvaluatorKind::Corpus;
                    else {
                        throw std::runtime_error(
                            "Unknown eval '" + val + "' in agent spec '" + s +
                            "'. Expected one of: score, corpus.");
                    }
                }
                if (key == "prior") {
                    // Validated (file exists, parses, no unknown keys) right
                    // here — a bad prior= path is an error at parse time,
                    // not a silent fallback to defaults at agent
                    // construction.
                    out.prior = loadPriorConfig(val);
                }
            }
            i = comma + 1;
        }
        if ((out.kind == "mcts" || out.kind == "ismcts") && out.sims <= 0) {
            throw std::runtime_error(
                "Agent '" + out.kind + "' requires sims=N (e.g. " + out.kind + ":sims=50).");
        }
    }
    return out;
}

}  // namespace riftbound
