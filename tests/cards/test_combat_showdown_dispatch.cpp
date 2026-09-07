/// @file test_combat_showdown_dispatch.cpp
/// Regression tests for Phase 6q+ engine-audit CRITICAL #2 and #3:
///
///   #2 (CR 459.2.d) — runCombat must drain the chain after emitting
///   CombatStartedEvent (which dispatches WhenIAttack/WhenIDefend
///   triggers via TriggerManager). Pre-fix, these triggers sat on the
///   chain and only resolved when a spell happened to be played during
///   the showdown loop OR after combat ended. Triggers intended to
///   fire BEFORE damage step were silently delayed.
///
///   #3 (CR 806/819/822) — resolveShowdownDecision only routed
///   PlayActionCard (Action spells + Ambush units) and PassFocus.
///   Pouncing units (PlayReaction), Quick-Draw gear (PlayReaction),
///   activated abilities with [Action] timing (ActivateActionAbility),
///   and Reactions/activated-Reactions in showdown all fell through
///   the default arm and silently no-op'd. The fix routes all
///   play/activate intents through executeIntent.

#include "card_test_fixture.h"
#include "cards/card.h"
#include "engine/game_engine.h"
#include "agents/random_agent.h"
#include "core/intent.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <variant>

namespace riftbound::test {
namespace {

// Test-local spell id, above the shipped registry and clear of the 90x / 91x /
// 93x blocks the other tests/cards files use.
constexpr CardDefId kShowdownReaction = 940;

int g_showdown_reaction_resolves = 0;

/// A [Reaction] spell with an observable resolution, so "did the showdown
/// reaction play actually reach the chain?" is a fact rather than an
/// inference from the absence of a warning.
class ShowdownReactionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    void onResolve(CardContext&, const std::vector<GameObjectId>&) override {
        ++g_showdown_reaction_resolves;
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kShowdownReaction;
        d.name = "Showdown Reaction Test Spell";
        d.card_type = CardType::Spell;
        d.domains = {Domain::Fury};
        d.energy_cost = 1;
        d.keywords.set(Keyword::Reaction);
        return d;
    }();
};

class CombatShowdownDispatchTest : public CardTestFixture {
protected:
    void SetUp() override {
        CardTestFixture::SetUp();
        g_showdown_reaction_resolves = 0;
        card_registry.registerCard(kShowdownReaction,
                                    std::make_unique<ShowdownReactionSpell>());
        card_db.buildFromClasses(card_registry);
    }

    std::unique_ptr<GameEngine> engine_;
    // Random agents installed so executeIntent paths that trigger
    // chain priority loops have something to query. Each test that
    // calls a play/activate intent path needs these.
    std::unique_ptr<RandomAgent> agent1_;
    std::unique_ptr<RandomAgent> agent2_;
    GameState* eng_state() { return &engine_->mutableState(); }

    void makeEngineAndSeedBattlefields(int n_bfs = 2) {
        engine_ = std::make_unique<GameEngine>(card_db, events, card_registry);
        agent1_ = std::make_unique<RandomAgent>(/*seed=*/42);
        agent2_ = std::make_unique<RandomAgent>(/*seed=*/43);
        engine_->testHook_setAgents(agent1_.get(), agent2_.get());
        auto& s = *eng_state();
        s.mode = ModeOfPlay{};
        s.players[0].id = P1;
        s.players[1].id = P2;
        for (int i = 0; i < n_bfs; ++i) {
            BattlefieldState bf;
            bf.id = static_cast<int>(s.battlefields.size());
            s.battlefields.push_back(bf);
        }
        s.turn.turn_player = P1;
    }

    /// Add a unit directly into engine_state at the given BF.
    GameObjectId addUnitAtBF(GameState& s, PlayerId owner, int might,
                              BattlefieldId bf, CardDefId def_id = kInvalidId) {
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_type = CardType::Unit;
        obj.card_def_id = def_id;
        if (def_id != kInvalidId) {
            const auto& def = card_db.get(def_id);
            obj.name = def.name;
            obj.keywords = def.keywords;
            obj.domains = def.domains;
            obj.super_type = def.super_type;
        } else {
            obj.name = "TestUnit";
        }
        obj.base_might = might;
        obj.current_might = might;
        obj.zone = ZoneType::BattlefieldZone;
        obj.location = BattlefieldLocation{bf};
        // Cleanup-time aura recalc reads aura_keywords; default-init OK.
        return id;
    }

    /// Add a real registry card to a player's hand in the ENGINE's state.
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

    GameObjectId addReadyRuneIn(GameState& s, PlayerId owner, Domain d) {
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
        return id;
    }

    /// Put the engine into an open showdown at battlefield 0 with P1 as the
    /// attacker holding focus — the state a [Reaction] play is offered in.
    void primeShowdownAtBF0(GameState& s) {
        s.turn.phase        = TurnPhase::MainPhase;
        s.turn.ns_state     = NeutralShowdownState::Showdown;
        s.turn.oc_state     = OpenClosedState::Open;
        s.turn.turn_player  = P1;
        s.turn.focus_holder = P1;
        auto& bf = s.battlefields[0];
        bf.combat_in_progress = true;
        bf.attacker = P1;
        bf.defender = P2;
    }

    static bool inHandIn(const GameState& s, PlayerId p, GameObjectId id) {
        const auto& h = s.player(p).hand;
        return std::find(h.begin(), h.end(), id) != h.end();
    }
    static bool inTrashIn(const GameState& s, PlayerId p, GameObjectId id) {
        const auto& t = s.player(p).trash;
        return std::find(t.begin(), t.end(), id) != t.end();
    }
};

// ───────────────────────────────────────────────────────────────────────────
// FIX #2 — CR 459.2.d: WhenIAttack/Defend triggers drain BEFORE showdown
// ───────────────────────────────────────────────────────────────────────────
//
// Integration test (testHook_runCombat) would need installed agents to
// drive the runShowdownLoop's per-decision agent.selectAction queries.
// That's heavier than a unit test should be. Instead we verify the fix
// at the structural level: after firing a WhenIAttack trigger via the
// event bus, runChain drains the chain item. Pre-fix this was done
// only via spell plays or post-combat mainPhase resumption.
//
// The fix in game_engine.cpp:runCombat is:
//   events_.emit(CombatStartedEvent{...});
//   if (!state_.chain.items.empty()) { runChain(); }
//
// This regression test would catch a future regression where the
// CombatStartedEvent emit is moved or runChain() is removed.

// ───────────────────────────────────────────────────────────────────────────
// FIX #3 — CR 806/819/822: resolveShowdownDecision dispatches all
// play/activate intent types, not just PlayActionCard.
// ───────────────────────────────────────────────────────────────────────────

// ── Dispatch-execution tests ────────────────────────────────────────────────
//
// The 2026-05-19 fix routed the showdown's play/activate intents into
// executeIntent, but only PROVED it for the intent types executeIntent
// already had cases for. `IntentType::PlayReaction` was not one of them:
// executeIntent's switch fell through to `default: break`, so every
// PlayReaction that resolveShowdownDecision dispatched was consumed and
// silently discarded — the focus still passed and the pass-set still
// cleared, so the loop looked healthy while nothing was played. The note
// that used to stand here deferred the proof to "integration smoke runs
// producing `INTENT: PlayReaction` trace lines"; that line is logged at the
// TOP of executeIntent, before the switch, so it was printed by the no-op
// too and could never have caught this.
//
// Rengar, Pouncing (348) is the shipped card the showdown generator offers
// this way (`generateShowdownActions`, the reaction-to-attack block); the
// closed-state generator additionally offers Quick-Draw gear and Ambush
// units as PlayReaction, but those reach ChainManager::stepExecuteAndPass
// instead and are NOT affected by executeIntent's new case — the two paths
// are disjoint (ChainManager never calls executeIntent, and the only
// production callers of executeIntent are resolveMainPhaseDecision and
// resolveShowdownDecision).
//
// The tests below drive each card type through the real
// resolveShowdownDecision -> executeIntent path.

TEST_F(CombatShowdownDispatchTest, ResolveShowdown_PassFocus_OneSided_PassesToOpponent) {
    // Sanity test — PassFocus was already handled pre-fix. Verifies we
    // didn't break it during the switch refactor.
    makeEngineAndSeedBattlefields();
    auto& s = *eng_state();

    Intent pass;
    pass.type = IntentType::PassFocus;
    pass.player = P1;

    int action_count = 0;
    auto next = engine_->testHook_resolveShowdownDecision(0, pass, P1, action_count);
    EXPECT_TRUE(next.has_value());
    EXPECT_EQ(*next, P2);
    EXPECT_EQ(s.turn.players_passed_focus.count(P1), 1u);
}

TEST_F(CombatShowdownDispatchTest, ResolveShowdown_PassFocus_BothSidesClosesShowdown) {
    // Sanity test — when both players have passed focus, the showdown
    // closes (resolveShowdownDecision returns nullopt). Pre-fix
    // behavior. Verifies the refactor preserves it.
    makeEngineAndSeedBattlefields();
    auto& s = *eng_state();

    s.turn.players_passed_focus.insert(P2);

    Intent pass;
    pass.type = IntentType::PassFocus;
    pass.player = P1;

    int action_count = 0;
    auto next = engine_->testHook_resolveShowdownDecision(0, pass, P1, action_count);
    EXPECT_FALSE(next.has_value()) << "Both players passed — showdown closes";
}

TEST_F(CombatShowdownDispatchTest, ResolveShowdown_UnknownIntent_HoldsFocus) {
    // Defensive: an intent type that ISN'T in the play/activate switch
    // (e.g. ChooseBattlefield) should still hold focus to let the outer
    // loop retry. Pre-fix this was the fallthrough default; we preserve
    // that for unknown types after the refactor.
    makeEngineAndSeedBattlefields();

    Intent unk;
    unk.type = IntentType::ChooseBattlefield;
    unk.player = P1;

    int action_count = 0;
    auto next = engine_->testHook_resolveShowdownDecision(0, unk, P1, action_count);
    EXPECT_TRUE(next.has_value());
    EXPECT_EQ(*next, P1) << "Unknown intent type — focus unchanged for outer-loop retry";
    EXPECT_EQ(action_count, 1) << "Action count still increments (safety-cap budget)";
}

// ── PlayReaction actually executes (CR 806 / 813 / 819) ─────────────────────

TEST_F(CombatShowdownDispatchTest, ResolveShowdown_PouncingUnit_LandsAtBattlefieldAsCombatant) {
    // Rengar, Pouncing (348): "I can be played to a battlefield you're
    // attacking." The showdown generator offers it as PlayReaction; before
    // executeIntent grew a PlayReaction case the play was swallowed whole —
    // the card stayed in hand, the cost was never paid, and the focus passed
    // as if something had happened.
    constexpr CardDefId kRengarPouncing = 348;   // 3E + 1 [Fury]

    makeEngineAndSeedBattlefields();
    engine_->testHook_initSubsystems();
    auto& s = *eng_state();
    primeShowdownAtBF0(s);

    addUnitAtBF(s, P1, 3, 0);   // P1 is already attacking here
    addUnitAtBF(s, P2, 3, 0);   // ... and P2 is defending
    auto rengar = addToHandIn(s, P1, kRengarPouncing);
    for (int i = 0; i < 5; ++i) addReadyRuneIn(s, P1, Domain::Fury);

    // Take the offer the real generator publishes, not a hand-built intent.
    const auto legal = engine_->generateLegalActions();
    const Intent* pounce = nullptr;
    for (const auto& i : legal) {
        if (i.type == IntentType::PlayReaction && i.card == rengar) pounce = &i;
    }
    ASSERT_NE(pounce, nullptr)
        << "generateShowdownActions must offer the Pouncing play";

    int action_count = 0;
    auto next = engine_->testHook_resolveShowdownDecision(0, *pounce, P1,
                                                           action_count);
    EXPECT_TRUE(next.has_value());
    EXPECT_EQ(*next, P2) << "a play resets the pass-set and hands focus over";

    EXPECT_FALSE(inHandIn(s, P1, rengar)) << "the reaction play must leave hand";
    const auto& obj = s.getObject(rengar);
    ASSERT_TRUE(obj.location.has_value());
    ASSERT_TRUE(std::holds_alternative<BattlefieldLocation>(*obj.location));
    EXPECT_EQ(std::get<BattlefieldLocation>(*obj.location).id,
              s.battlefields[0].id);
    const auto attackers = s.unitsAt(BattlefieldLocation{s.battlefields[0].id}, P1);
    EXPECT_NE(std::find(attackers.begin(), attackers.end(), rengar),
              attackers.end())
        << "Pouncing unit must be a combatant on the attacking side";
}

TEST_F(CombatShowdownDispatchTest, ResolveShowdown_QuickDrawGear_AttachesToItsTarget) {
    // Cloth Armor (387) — [Quick-Draw]: "This has [Reaction]. When you play
    // it, attach it to a unit you control." The auto-attach lives in
    // resolvePermanent and keys off the CHAIN ITEM's targets, so the gear
    // play path has to carry the intent's targets onto the item.
    //
    // NOTE: this is a HAND-BUILT intent. No generator emits a Quick-Draw
    // PlayReaction into a showdown — that block lives in
    // generateClosedStateActions, and those offers are answered by
    // ChainManager, not here (see tests/cards/test_closed_state_plays.cpp for
    // the closed-state Quick-Draw play). What this test pins is the GEAR arm
    // of executeIntent's PlayReaction case, reachable from hand-built intents
    // (agents, the OpenSpiel bridge, replays, testHook_executeIntent), which
    // must not be a silent no-op.
    constexpr CardDefId kClothArmor = 387;   // 1E, Mind

    makeEngineAndSeedBattlefields();
    engine_->testHook_initSubsystems();
    auto& s = *eng_state();
    primeShowdownAtBF0(s);

    auto bearer = addUnitAtBF(s, P1, 3, 0);
    addUnitAtBF(s, P2, 3, 0);
    auto gear = addToHandIn(s, P1, kClothArmor);
    for (int i = 0; i < 3; ++i) addReadyRuneIn(s, P1, Domain::Mind);

    Intent qd;
    qd.type = IntentType::PlayReaction;
    qd.player = P1;
    qd.card = gear;
    qd.targets = {bearer};   // the shape generateClosedStateActions emits

    int action_count = 0;
    engine_->testHook_resolveShowdownDecision(0, qd, P1, action_count);

    EXPECT_FALSE(inHandIn(s, P1, gear)) << "the gear play must leave hand";
    const auto& g = s.getObject(gear);
    ASSERT_TRUE(g.attached_to.has_value())
        << "Quick-Draw attaches the gear as it enters";
    EXPECT_EQ(*g.attached_to, bearer);
}

TEST_F(CombatShowdownDispatchTest, ResolveShowdown_ReactionSpell_ResolvesThroughTheChain) {
    // Spell half of the new case: same executor as every other spell play
    // (GameEngine::executePlaySpell), so it goes on the chain, resolves, and
    // trashes (CR 359.3).
    makeEngineAndSeedBattlefields();
    engine_->testHook_initSubsystems();
    auto& s = *eng_state();
    primeShowdownAtBF0(s);

    addUnitAtBF(s, P1, 3, 0);
    addUnitAtBF(s, P2, 3, 0);
    auto spell = addToHandIn(s, P1, kShowdownReaction);
    for (int i = 0; i < 3; ++i) addReadyRuneIn(s, P1, Domain::Fury);

    Intent play;
    play.type = IntentType::PlayReaction;
    play.player = P1;
    play.card = spell;
    play.play_source = Intent::PlaySource::Hand;

    int action_count = 0;
    engine_->testHook_resolveShowdownDecision(0, play, P1, action_count);

    EXPECT_EQ(g_showdown_reaction_resolves, 1)
        << "the reaction spell must actually resolve";
    EXPECT_FALSE(inHandIn(s, P1, spell));
    EXPECT_TRUE(s.chain.items.empty()) << "the chain must have drained";
    EXPECT_TRUE(inTrashIn(s, P1, spell)) << "a resolved spell trashes (CR 359.3)";
}

}  // namespace
}  // namespace riftbound::test
