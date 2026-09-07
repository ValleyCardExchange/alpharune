/// @file test_decision_log.cpp
/// Tests for DecisionLogWriter (L1 of the replay-analysis loop).
/// See docs/superpowers/specs/2026-09-07-replay-loop-iter0-design.md
/// "L1 — decision log".

#include "io/decision_log_writer.h"
#include "engine/batch_runner.h"
#include "engine/game_runner.h"
#include "core/card_db.h"
#include "cards/card_registry.h"
#include "rules/deck_validator.h"
#include "tests/cards/card_test_fixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace riftbound;
using namespace riftbound::test;
namespace fs = std::filesystem;

namespace {

std::string deckPath(const char* name) {
    for (auto& prefix : {"../decks/", "../../decks/", "decks/"}) {
        auto p = std::string(prefix) + name;
        if (fs::exists(p)) return p;
    }
    const char* root = std::getenv("RIFTBOUND_ROOT");
    if (root) {
        auto p = fs::path(root) / "decks" / name;
        if (fs::exists(p)) return p.string();
    }
    return std::string("decks/") + name;
}

nlohmann::json parseFile(const std::string& path) {
    std::ifstream in(path);
    EXPECT_TRUE(in.is_open()) << "could not open " << path;
    nlohmann::json j;
    in >> j;  // throws on malformed JSON -> test fails loudly
    return j;
}

DecisionLogHeader makeHeader() {
    DecisionLogHeader h;
    h.deck1 = {"decks/deck_one.txt", "Legend One", "Champion One"};
    h.deck2 = {"decks/deck_two.txt", "Legend Two", "Champion Two"};
    h.agent1_spec = "random";
    h.agent2_spec = "mcts:sims=20";
    h.seed = 4242;
    h.engine_version = "vTEST+deadbee";
    return h;
}

} // namespace

class DecisionLogTest : public CardTestFixture {
protected:
    fs::path tmp_path;

    void SetUp() override {
        CardTestFixture::SetUp();
        tmp_path = fs::temp_directory_path() /
            ("decision_log_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) +
             "_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ".json");
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove(tmp_path, ec);
    }
};

// (1) Writer fed two synthetic decisions and a footer produces valid JSON
// with every field the spec's contract names, at the right type.
TEST_F(DecisionLogTest, TwoDecisionsAndFooterProduceValidCompleteJson) {
    // Board furniture so the per-record player/battlefield summaries have
    // something non-trivial to report.
    auto hand_card = addToHand(P1, kInvalidId);
    addRune(P1, Domain::Fury, /*exhausted=*/false);
    addRune(P1, Domain::Fury, /*exhausted=*/true);
    auto unit = addUnit(P2, kInvalidId, /*might=*/3, /*at_bf=*/0);
    state.battlefields[0].card_object_id = kInvalidId;
    state.turn.turn_number = 3;
    state.turn.phase = TurnPhase::MainPhase;
    state.turn.ns_state = NeutralShowdownState::Neutral;
    state.turn.oc_state = OpenClosedState::Open;
    state.decision_index = 0;
    state.players[0].score = 2;
    state.players[1].score = 1;

    Intent legal_a = Intent::endTurn(P1);
    Intent legal_b;
    legal_b.type = IntentType::PlayCard;
    legal_b.player = P1;
    legal_b.card = hand_card;
    std::vector<Intent> legal = {legal_a, legal_b};

    Intent chosen = legal_b;

    {
        DecisionLogWriter writer(tmp_path.string(), makeHeader());
        state.decision_index = 0;
        writer.recordDecision(state, legal, chosen);

        state.decision_index = 1;
        state.turn.turn_number = 4;
        Intent chosen2 = Intent::endTurn(P2);
        writer.recordDecision(state, legal, chosen2);

        int final_scores[2] = {5, 3};
        writer.finish(PlayerId::Player1, "P1 reached the winning point total",
                      /*turns=*/4, final_scores, /*decision_count=*/2);
    }

    auto j = parseFile(tmp_path.string());

    // Header
    EXPECT_EQ(j.at("schema").get<int>(), 1);
    EXPECT_EQ(j.at("decks").at("p1").at("legend").get<std::string>(), "Legend One");
    EXPECT_EQ(j.at("decks").at("p1").at("champion").get<std::string>(), "Champion One");
    EXPECT_EQ(j.at("decks").at("p2").at("legend").get<std::string>(), "Legend Two");
    EXPECT_EQ(j.at("agents").at("p1").get<std::string>(), "random");
    EXPECT_EQ(j.at("agents").at("p2").get<std::string>(), "mcts:sims=20");
    EXPECT_EQ(j.at("seed").get<uint64_t>(), 4242u);
    ASSERT_TRUE(j.contains("seats"));
    EXPECT_TRUE(j.at("seats").is_object());
    EXPECT_EQ(j.at("engine_version").get<std::string>(), "vTEST+deadbee");

    ASSERT_TRUE(j.contains("log"));
    ASSERT_TRUE(j.at("log").is_array());
    ASSERT_EQ(j.at("log").size(), 2u);

    const auto& rec = j.at("log").at(0);
    EXPECT_EQ(rec.at("idx").get<int>(), 0);
    EXPECT_EQ(rec.at("turn").get<int>(), 3);
    ASSERT_TRUE(rec.contains("phase"));
    EXPECT_EQ(rec.at("phase").at("phase").get<std::string>(), "MainPhase");
    EXPECT_EQ(rec.at("phase").at("ns_state").get<std::string>(), "Neutral");
    EXPECT_EQ(rec.at("phase").at("oc_state").get<std::string>(), "Open");
    EXPECT_EQ(rec.at("actor").get<std::string>(), "P1");
    ASSERT_TRUE(rec.at("scores").is_array());
    EXPECT_EQ(rec.at("scores").size(), 2u);
    EXPECT_EQ(rec.at("scores").at(0).get<int>(), 2);
    EXPECT_EQ(rec.at("scores").at(1).get<int>(), 1);

    ASSERT_TRUE(rec.at("players").is_array());
    ASSERT_EQ(rec.at("players").size(), 2u);
    const auto& p1_rec = rec.at("players").at(0);
    EXPECT_EQ(p1_rec.at("seat").get<std::string>(), "P1");
    EXPECT_TRUE(p1_rec.contains("hand"));
    EXPECT_TRUE(p1_rec.contains("deck"));
    EXPECT_TRUE(p1_rec.contains("trash"));
    EXPECT_TRUE(p1_rec.contains("banishment"));
    EXPECT_EQ(p1_rec.at("runes_ready").get<int>(), 1);
    EXPECT_EQ(p1_rec.at("runes_exhausted").get<int>(), 1);
    EXPECT_TRUE(p1_rec.contains("legend_empowered"));
    EXPECT_FALSE(p1_rec.at("legend_empowered").get<bool>());

    ASSERT_TRUE(rec.at("battlefields").is_array());
    ASSERT_GE(rec.at("battlefields").size(), 1u);
    const auto& bf0 = rec.at("battlefields").at(0);
    EXPECT_TRUE(bf0.contains("id"));
    EXPECT_TRUE(bf0.contains("name"));
    EXPECT_TRUE(bf0.contains("controller"));
    EXPECT_TRUE(bf0.contains("contested"));
    EXPECT_EQ(bf0.at("units").at("p2").at("count").get<int>(), 1);
    EXPECT_EQ(bf0.at("units").at("p2").at("might").get<int>(), 3);
    EXPECT_EQ(bf0.at("units").at("p1").at("count").get<int>(), 0);

    EXPECT_EQ(rec.at("legal_count").get<int>(), 2);

    const auto& chosen_json = rec.at("chosen");
    EXPECT_EQ(chosen_json.at("type").get<std::string>(), "PlayCard");
    EXPECT_TRUE(chosen_json.contains("card"));
    EXPECT_TRUE(chosen_json.contains("source_zone"));
    EXPECT_TRUE(chosen_json.contains("flow_source"));
    EXPECT_TRUE(chosen_json.contains("restricted_bf"));
    EXPECT_TRUE(chosen_json.contains("destination"));

    EXPECT_TRUE(rec.contains("root_value"));
    EXPECT_TRUE(rec.at("root_value").is_null());

    // Second record advanced state
    EXPECT_EQ(j.at("log").at(1).at("turn").get<int>(), 4);
    EXPECT_EQ(j.at("log").at(1).at("actor").get<std::string>(), "P2");

    // Footer
    ASSERT_TRUE(j.contains("winner"));
    EXPECT_EQ(j.at("winner").at("seat").get<std::string>(), "P1");
    EXPECT_EQ(j.at("winner").at("deck").get<std::string>(), "Legend One / Champion One");
    EXPECT_EQ(j.at("reason").get<std::string>(), "P1 reached the winning point total");
    EXPECT_EQ(j.at("turns").get<int>(), 4);
    ASSERT_TRUE(j.at("final_scores").is_array());
    EXPECT_EQ(j.at("final_scores").at(0).get<int>(), 5);
    EXPECT_EQ(j.at("final_scores").at(1).get<int>(), 3);
    EXPECT_EQ(j.at("decisions").get<int>(), 2);

    EXPECT_FALSE(j.contains("truncated"));
}

// (2) An intent with flow_source = Granted and a target_battlefield_restriction
// serialises both.
TEST_F(DecisionLogTest, FlowGrantedAndRestrictedBattlefieldSerialize) {
    auto card = addToHand(P1, kInvalidId);
    state.turn.turn_number = 1;
    state.decision_index = 0;

    Intent chosen;
    chosen.type = IntentType::PlayCard;
    chosen.player = P1;
    chosen.card = card;
    chosen.flow_source = Intent::FlowSource::Granted;
    chosen.target_battlefield_restriction = BattlefieldId{1};

    std::vector<Intent> legal = {chosen};

    {
        DecisionLogWriter writer(tmp_path.string(), makeHeader());
        writer.recordDecision(state, legal, chosen);
        int final_scores[2] = {0, 0};
        writer.finish(PlayerId::None, "test-only, no real game", 1, final_scores, 1);
    }

    auto j = parseFile(tmp_path.string());
    const auto& rec = j.at("log").at(0);
    EXPECT_EQ(rec.at("chosen").at("flow_source").get<std::string>(), "Granted");
    ASSERT_FALSE(rec.at("chosen").at("restricted_bf").is_null());
    EXPECT_EQ(rec.at("chosen").at("restricted_bf").get<BattlefieldId>(), 1u);
}

// (3) End-to-end: drive GameRunner directly (random vs random, fast) for
// 2 games with the decision-log dir set -> two parseable files with
// matching seeds and footers.
TEST(DecisionLogEndToEnd, TwoGamesViaGameRunnerProduceTwoParseableFiles) {
    CardRegistry card_registry;
    card_registry.loadAll();
    CardDB card_db;
    card_db.buildFromClasses(card_registry);

    std::string deck1_path = deckPath("kennen_tyler.txt");
    std::string deck2_path = deckPath("rengar_test.txt");
    DeckSubmission deck1 = DeckValidator::loadFromDeckList(deck1_path, card_db);
    DeckSubmission deck2 = DeckValidator::loadFromDeckList(deck2_path, card_db);

    fs::path log_dir = fs::temp_directory_path() /
        ("decision_log_e2e_" + std::to_string(reinterpret_cast<uintptr_t>(&card_db)));
    std::error_code ec;
    fs::remove_all(log_dir, ec);

    GameConfig cfg;
    cfg.base_seed = 9001;
    cfg.do_render = false;
    cfg.total_games = 2;
    cfg.agent1_spec = "random";
    cfg.agent2_spec = "random";
    cfg.decision_log_dir = log_dir.string();
    cfg.deck1_path = deck1_path;
    cfg.deck2_path = deck2_path;

    AggregateResults results;
    results.total_games = 2;
    for (int i = 0; i < 2; ++i) {
        GameConfig per_game = cfg;
        per_game.game_index = i;
        GameRunner runner(card_db, card_registry, deck1, deck2, per_game, results);
        runner.run();
    }

    for (int i = 0; i < 2; ++i) {
        uint64_t expected_seed = (cfg.base_seed + i) & 0x7FFFFFFFu;
        fs::path expected = log_dir / ("game_" + std::to_string(i) + "_seed_" +
                                        std::to_string(expected_seed) + ".json");
        ASSERT_TRUE(fs::exists(expected)) << "missing " << expected;

        auto j = parseFile(expected.string());
        EXPECT_EQ(j.at("schema").get<int>(), 1);
        EXPECT_EQ(j.at("seed").get<uint64_t>(), expected_seed);
        EXPECT_TRUE(j.at("log").is_array());
        EXPECT_GT(j.at("log").size(), 0u);
        ASSERT_TRUE(j.contains("winner"));
        ASSERT_TRUE(j.contains("reason"));
        ASSERT_TRUE(j.contains("turns"));
        ASSERT_TRUE(j.contains("final_scores"));
        ASSERT_TRUE(j.contains("decisions"));
        EXPECT_FALSE(j.contains("truncated"));
    }

    fs::remove_all(log_dir, ec);
}
