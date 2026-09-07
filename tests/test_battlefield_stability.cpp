/// @file test_battlefield_stability.cpp
/// Guard tests for the lifetime contract of `GameState::battlefields`.
///
/// Background (see
/// `.superpowers/sdd/2026-09-07-corpus-evaluator/crash-analysis.md`):
/// `GameEngine::setupBattlefields` builds the container with exactly two
/// appends and no reserve, so a `std::vector` ends the setup at
/// size == capacity == 2. `EffectExecutor::addBattlefieldToken` (the
/// Baron Pit spawn, `decks/kennen_tyler.txt`) appends a third element,
/// which with a vector is a GUARANTEED reallocation. Three engine sites
/// hold a `BattlefieldState&` across card resolution and then read and
/// write it — `GameEngine::advanceMainPhase`'s staged-battlefield loop,
/// `GameEngine::runShowdown`, `GameEngine::runCombat`. The reallocation
/// freed the buffer under those references: reads produced garbage
/// battlefield ids (`Battlefield not found: 3691939024`) and writes
/// landed in freed memory, corrupting whatever reused it.
///
/// The contract these tests pin: **appending a battlefield must not
/// invalidate references, pointers, or ids of the existing ones.** That
/// is a property of the container choice, so it is tested at the
/// container level rather than by trying to reproduce a
/// timing-and-allocator-dependent crash.
///
/// RED/GREEN: with `std::vector<BattlefieldState>` (the pre-fix type)
/// `BattlefieldReferencesSurviveTokenAppend` fails deterministically —
/// the fixture's two appends leave size == capacity == 2, so the third
/// append moves every element to a new buffer and the recorded address
/// no longer matches. With `std::deque<BattlefieldState>` it passes:
/// deque appends never relocate existing elements.

#include "tests/cards/card_test_fixture.h"

#include "core/game_state.h"
#include "agents/random_agent.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <variant>

using namespace riftbound;
using namespace riftbound::test;

namespace {

// ─── 1. Reference / pointer stability across a battlefield-token append ────

TEST_F(CardTestFixture, BattlefieldReferencesSurviveTokenAppend) {
    // The fixture appends exactly two battlefields into a freshly
    // default-constructed container — the same shape
    // `GameEngine::setupBattlefields` leaves. Under a vector that is
    // size == capacity == 2 and the next append must reallocate.
    ASSERT_EQ(state.battlefields.size(), 2u);

    const BattlefieldState* bf0_before = &state.battlefields[0];
    const BattlefieldState* bf1_before = &state.battlefields[1];
    const BattlefieldId bf0_id_before = state.battlefields[0].id;
    const BattlefieldId bf1_id_before = state.battlefields[1].id;

    // A live reference of exactly the kind the three engine sites hold
    // across card resolution.
    BattlefieldState& held = state.battlefields[0];
    held.showdown_staged = true;

    EffectExecutor exec(state, events, card_db, &card_registry);
    const BattlefieldId pit = exec.addBattlefieldToken(
        "Baron Pit", /*accepts_any_inbound=*/true);

    ASSERT_EQ(state.battlefields.size(), 3u);
    EXPECT_EQ(pit, 2u);

    // The load-bearing assertion: the existing elements did not move.
    EXPECT_EQ(bf0_before, &state.battlefields[0])
        << "Appending a battlefield token relocated battlefield 0. Every "
           "BattlefieldState& held across card resolution (the staged-"
           "battlefield loop, runShowdown, runCombat) is now dangling.";
    EXPECT_EQ(bf1_before, &state.battlefields[1])
        << "Appending a battlefield token relocated battlefield 1.";

    // The reference taken before the append still names the same live
    // element, and the write made through it is still visible in the
    // container. Only meaningful once the addresses above match; with a
    // relocating container this read is of freed memory.
    if (bf0_before == &state.battlefields[0]) {
        EXPECT_EQ(held.id, bf0_id_before);
        EXPECT_TRUE(state.battlefields[0].showdown_staged)
            << "A write through a reference taken before the append was "
               "lost — the reference no longer names the live element.";
    }

    // Ids are untouched and still unique/dense.
    EXPECT_EQ(state.battlefields[0].id, bf0_id_before);
    EXPECT_EQ(state.battlefields[1].id, bf1_id_before);
    EXPECT_EQ(state.battlefields[2].id, 2u);
    EXPECT_TRUE(state.battlefields[2].is_token);
}

// ─── 2. The same, driven twice — two token appends in one game ─────────────
//
// A second BF-token card (or a second Baron on the other side of the
// table once "the Pit already exists" stops short-circuiting) must not
// reopen the hazard either. This is the guard that must not be weakened
// if another battlefield-token card lands.

TEST_F(CardTestFixture, BattlefieldReferencesSurviveRepeatedAppends) {
    ASSERT_EQ(state.battlefields.size(), 2u);

    EffectExecutor exec(state, events, card_db, &card_registry);

    std::vector<const BattlefieldState*> addrs;
    for (const auto& bf : state.battlefields) addrs.push_back(&bf);

    for (int i = 0; i < 6; ++i) {
        exec.addBattlefieldToken("Token BF " + std::to_string(i),
                                  /*accepts_any_inbound=*/false);
        addrs.push_back(&state.battlefields.back());

        ASSERT_EQ(state.battlefields.size(), addrs.size());
        for (size_t j = 0; j < addrs.size(); ++j) {
            EXPECT_EQ(addrs[j], &state.battlefields[j])
                << "Battlefield " << j << " moved on append #" << i;
            EXPECT_EQ(state.battlefields[j].id, static_cast<BattlefieldId>(j));
        }
    }
}

}  // namespace

// ─── 3. Baron Nashor end-to-end: Pit spawn, then a real combat ─────────────
//
// The container-level guards above are the deterministic RED. This one is
// the integration guard the engine did not have: Baron Nashor is played
// through the REAL play path (`executePlayCard` → `Card::onPlay` →
// `EffectExecutor::addBattlefieldToken`), which grows the battlefield
// container mid-game, and the turn then runs a real combat — the
// `runCombat` / `runShowdownLoop` site that used to hold a
// `BattlefieldState&` across resolution. Nothing else in the suite ever
// ran the engine with three battlefields.
//
// HONEST NOTE ON RED: this test does NOT go red on the pre-fix vector
// build. The Pit append happens BEFORE the combat, so the references
// `runCombat` takes are taken after the reallocation and are valid. The
// fatal ordering (an append DURING a live showdown/combat, which is what
// the batch runner hit) needs a battlefield-token spawn from inside
// showdown resolution; Baron Nashor is a plain 10-cost unit with no
// Ambush, so no legal line plays him there, and reproducing it would take
// a synthetic card plus ASan to observe. Recorded here as an end-to-end
// consistency guard, not as the failing test.

namespace {

class BaronPitEngineTest : public CardTestFixture {
protected:
    std::unique_ptr<GameEngine> engine_;
    std::unique_ptr<RandomAgent> agent1_;
    std::unique_ptr<RandomAgent> agent2_;

    void makeEngine(int n_bfs = 2) {
        engine_ = std::make_unique<GameEngine>(card_db, events, card_registry);
        agent1_ = std::make_unique<RandomAgent>(/*seed=*/42);
        agent2_ = std::make_unique<RandomAgent>(/*seed=*/43);
        engine_->testHook_setAgents(agent1_.get(), agent2_.get());
        engine_->testHook_initSubsystems();
        auto& s = engine_->mutableState();
        s.mode = ModeOfPlay{};
        s.players[0].id = P1;
        s.players[1].id = P2;
        for (int i = 0; i < n_bfs; ++i) {
            BattlefieldState bf;
            bf.id = static_cast<BattlefieldId>(s.battlefields.size());
            s.battlefields.push_back(bf);
        }
        s.turn.turn_player = P1;
        s.turn.phase = TurnPhase::MainPhase;
    }

    GameObjectId addUnitAtBF(GameState& s, PlayerId owner, int might,
                              BattlefieldId bf) {
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_type = CardType::Unit;
        obj.name = "TestUnit";
        obj.base_might = might;
        obj.current_might = might;
        obj.zone = ZoneType::BattlefieldZone;
        obj.location = BattlefieldLocation{bf};
        return id;
    }

    GameObjectId addToHandIn(GameState& s, PlayerId owner, CardDefId def_id) {
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_def_id = def_id;
        const auto& def = card_db.get(def_id);
        obj.name = def.name;
        obj.card_type = def.card_type;
        obj.super_type = def.super_type;
        obj.keywords = def.keywords;
        obj.domains = def.domains;
        obj.tags = def.tags;
        obj.base_might = def.might;
        obj.current_might = def.might;
        obj.zone = ZoneType::Hand;
        s.player(owner).hand.push_back(id);
        return id;
    }

    void addReadyRunesIn(GameState& s, PlayerId owner, Domain d, int n) {
        for (int i = 0; i < n; ++i) {
            auto id = s.createObject();
            auto& r = s.getObject(id);
            r.owner = owner;
            r.controller = owner;
            r.card_type = CardType::Rune;
            r.name = "Test Rune";
            r.domains = {d};
            r.zone = ZoneType::Base;
            r.location = BaseLocation{owner};
            r.is_exhausted = false;
        }
    }

    static std::optional<BattlefieldId> findPit(const GameState& s) {
        for (const auto& bf : s.battlefields) {
            if (!s.objectExists(bf.card_object_id)) continue;
            if (s.getObject(bf.card_object_id).name == "Baron Pit") return bf.id;
        }
        return std::nullopt;
    }
};

}  // namespace

TEST_F(BaronPitEngineTest, BaronPitSpawnsThenCombatRunsOnThreeBattlefields) {
    makeEngine(/*n_bfs=*/2);
    auto& s = engine_->mutableState();
    ASSERT_EQ(s.battlefields.size(), 2u);

    // Baron Nashor [709] costs 10 energy + 3 [Chaos] power.
    auto baron = addToHandIn(s, P1, 709);
    addReadyRunesIn(s, P1, Domain::Chaos, 16);

    // Both seats hold BF 0 so the combat below is a real one.
    addUnitAtBF(s, P1, /*might=*/3, /*bf=*/0);
    addUnitAtBF(s, P2, /*might=*/3, /*bf=*/0);

    Intent play;
    play.type = IntentType::PlayCard;
    play.player = P1;
    play.card = baron;
    play.play_location = BattlefieldLocation{1};
    ASSERT_NO_THROW(engine_->testHook_executeIntent(play));

    // The Pit spawned and Baron entered it (CR "If you do, I enter there").
    ASSERT_EQ(s.battlefields.size(), 3u)
        << "Baron Nashor's onPlay must append the Baron Pit battlefield.";
    auto pit = findPit(s);
    ASSERT_TRUE(pit.has_value()) << "Baron Pit battlefield token not found.";
    EXPECT_EQ(*pit, 2u);
    EXPECT_TRUE(s.battlefields[2].accepts_any_inbound);
    ASSERT_TRUE(s.objectExists(baron));
    ASSERT_TRUE(s.getObject(baron).location.has_value());
    ASSERT_TRUE(std::holds_alternative<BattlefieldLocation>(
        *s.getObject(baron).location));
    EXPECT_EQ(std::get<BattlefieldLocation>(*s.getObject(baron).location).id,
              *pit);

    // Now run a real combat at BF 0 with three battlefields on the board.
    s.battlefields[0].is_contested = true;
    s.battlefields[0].contested_by = P2;
    ASSERT_NO_THROW(engine_->testHook_runCombat(0));

    // The engine is still coherent: every battlefield id still resolves,
    // the Pit is still there, and BF 0's combat ran to its resolution step.
    ASSERT_EQ(s.battlefields.size(), 3u);
    for (BattlefieldId i = 0; i < 3; ++i) {
        EXPECT_EQ(s.battlefields[i].id, i);
    }
    EXPECT_TRUE(findPit(s).has_value());
    // combatResolutionStep's closing writes (combat_in_progress,
    // combat_phase, is_contested, attacker/defender) must land on the LIVE
    // battlefield 0 — they are made through the `BattlefieldState&` runCombat
    // and combatResolutionStep hold across the showdown loop.
    EXPECT_FALSE(s.battlefields[0].combat_in_progress)
        << "runCombat's writes through its BattlefieldState& must land on "
           "the live battlefield 0.";
    EXPECT_EQ(s.battlefields[0].combat_phase, CombatPhase::None);
    EXPECT_FALSE(s.battlefields[0].is_contested);
    EXPECT_FALSE(s.battlefields[0].attacker.has_value());
    EXPECT_FALSE(s.battlefields[0].showdown_in_progress);
}
