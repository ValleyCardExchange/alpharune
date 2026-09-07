/// @file test_empower.cpp
/// Tests for Empower / Disempower (CR 441, 442) — EffectExecutor's
/// empowerObject / disempowerObject and the disempower_self activation
/// cost component (CR 828).
///
/// 1. Empowering an object sets the status; empowering again is a no-op
///    and emits no second event.
/// 2. Disempowering a non-empowered object leaves it non-empowered and
///    emits nothing.
/// 3. An ability with disempower_self is absent from the offered intents
///    while the source is not empowered, present when it is, and
///    activating it leaves the source disempowered and exhausted.

#include "tests/cards/card_test_fixture.h"

#include "cards/card.h"
#include "cards/card_registry.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"

using namespace riftbound;
using namespace riftbound::test;

namespace {

using EmpowerTest = CardTestFixture;

TEST_F(EmpowerTest, EmpoweringSetsStatusAndEmitsEventOnce) {
    auto obj_id = addUnit(P1, kInvalidId, /*might=*/1);

    EffectExecutor exec(state, events, card_db);

    int empower_events = 0;
    auto conn = events.on_object_empowered.connect(
        [&](const ObjectEmpoweredEvent&) { ++empower_events; });

    ASSERT_FALSE(state.getObject(obj_id).is_empowered);

    exec.empowerObject(obj_id);
    EXPECT_TRUE(state.getObject(obj_id).is_empowered);
    EXPECT_EQ(empower_events, 1);

    // Re-empowering an already-empowered object is a no-op (CR 441.1.c):
    // the flag stays set and no second event fires.
    exec.empowerObject(obj_id);
    EXPECT_TRUE(state.getObject(obj_id).is_empowered);
    EXPECT_EQ(empower_events, 1);
}

TEST_F(EmpowerTest, DisempoweringNonEmpoweredObjectIsNoOp) {
    auto obj_id = addUnit(P1, kInvalidId, /*might=*/1);

    EffectExecutor exec(state, events, card_db);

    int empower_events = 0;
    auto conn = events.on_object_empowered.connect(
        [&](const ObjectEmpoweredEvent&) { ++empower_events; });

    ASSERT_FALSE(state.getObject(obj_id).is_empowered);

    exec.disempowerObject(obj_id);
    EXPECT_FALSE(state.getObject(obj_id).is_empowered);
    EXPECT_EQ(empower_events, 0);
}

TEST_F(EmpowerTest, DisempoweringEmpoweredObjectClearsStatus) {
    auto obj_id = addUnit(P1, kInvalidId, /*might=*/1);
    EffectExecutor exec(state, events, card_db);
    exec.empowerObject(obj_id);
    ASSERT_TRUE(state.getObject(obj_id).is_empowered);

    exec.disempowerObject(obj_id);
    EXPECT_FALSE(state.getObject(obj_id).is_empowered);
}

// ─── Empowered clears when the object leaves the board (spec §1) ──────────

TEST_F(EmpowerTest, EmpoweredClearsWhenKilled) {
    auto obj_id = addUnit(P1, kInvalidId, /*might=*/3, /*at_bf=*/0);
    EffectExecutor exec(state, events, card_db);
    exec.empowerObject(obj_id);
    ASSERT_TRUE(state.getObject(obj_id).is_empowered);

    exec.killObject(obj_id);
    EXPECT_FALSE(state.getObject(obj_id).is_empowered)
        << "Empowered must clear when the object leaves the board via death";
}

TEST_F(EmpowerTest, EmpoweredClearsWhenBouncedToHand) {
    auto obj_id = addUnit(P1, kInvalidId, /*might=*/3, /*at_bf=*/0);
    EffectExecutor exec(state, events, card_db);
    exec.empowerObject(obj_id);
    ASSERT_TRUE(state.getObject(obj_id).is_empowered);

    exec.bounceToHand(obj_id);
    EXPECT_FALSE(state.getObject(obj_id).is_empowered)
        << "Empowered must clear when the object leaves the board via bounce";
}

// Test #32: combat death (GameEngine::killUnit, via processLethalDamage) is
// also a board-exit path — separate code from EffectExecutor::killObject,
// exercised the way ElderDragonShieldTest in
// test_targeting_and_combat_invariants.cpp drives lethal-damage cleanup.
TEST_F(EmpowerTest, EmpoweredClearsOnCombatDeath) {
    GameEngine engine(card_db, events, card_registry);
    auto& s = engine.mutableState();
    s.mode          = ModeOfPlay{};
    s.players[0].id = P1;
    s.players[1].id = P2;

    auto unit_id = s.createObject();
    auto& u = s.getObject(unit_id);
    u.owner = P1;
    u.controller = P1;
    u.card_type = CardType::Unit;
    u.name = "TestUnit";
    u.base_might = 1;
    u.current_might = 1;
    u.zone = ZoneType::Base;
    u.location = BaseLocation{P1};
    u.damage_marked = 1;  // lethal at 1M
    u.is_empowered = true;

    engine.testHook_processLethalDamage();

    ASSERT_FALSE(s.objectExists(unit_id) &&
                 s.getObject(unit_id).zone == ZoneType::Base)
        << "sanity: the unit must actually have died (moved off the board)";
    EXPECT_FALSE(s.getObject(unit_id).is_empowered)
        << "Empowered must clear on combat death (GameEngine::killUnit), "
           "not just the effect/ability-kill path (EffectExecutor::killObject)";
}

// ─── Test #3: disempower_self activation cost gate + payment ───────────────

namespace {

/// Minimal test-local legend with one activated ability whose cost is
/// {exhaust, disempower_self}. No targets, no [Action]/[Reaction] timing —
/// just enough shape to exercise the three offer gates + the payment path
/// in GameEngine.
class EmpowerCostTestLegend : public LegendCard {
public:
    const CardDef& def() const override { return def_; }

    std::vector<ActivatedAbility> activatedAbilities() const override {
        return {{
            .cost = {.exhaust = true, .disempower_self = true},
            .targets = TargetRequirements{},
            .is_action = false,
            .is_reaction = false,
        }};
    }

    void onActivate(CardContext&, int,
                    const std::vector<GameObjectId>&) override {}

private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 900;  // unused test-only id, well above the 787-card registry
        d.name = "Empower Cost Test Legend";
        d.card_type = CardType::Legend;
        return d;
    }();
};

}  // namespace

TEST_F(EmpowerTest, DisempowerSelfCostGatesAndPaysOnActivation) {
    constexpr CardDefId kTestLegendId = 900;
    card_registry.registerCard(kTestLegendId, std::make_unique<EmpowerCostTestLegend>());

    GameEngine engine(card_db, events, card_registry);
    // Activating the ability runs it through the chain (processFEPR),
    // which queries both players' agents for priority — install FirstChoiceAgents
    // per testHook_setAgents' doc comment so runChain doesn't dereference a
    // null agent.
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    // A bare-constructed GameEngine leaves ChainManager/EffectExecutor/
    // TriggerManager null (normally set up per-game in runGame); the
    // disempower_self payment and the ability's chain resolution both need
    // them.
    engine.testHook_initSubsystems();
    auto& s = engine.mutableState();
    s.mode             = ModeOfPlay{};
    s.players[0].id    = P1;
    s.players[1].id    = P2;
    s.turn.turn_player = P1;
    s.turn.phase       = TurnPhase::MainPhase;
    s.turn.ns_state    = NeutralShowdownState::Neutral;
    s.turn.oc_state    = OpenClosedState::Open;
    BattlefieldState b0; b0.id = 0; s.battlefields.push_back(b0);
    BattlefieldState b1; b1.id = 1; s.battlefields.push_back(b1);

    auto legend = s.createObject();
    auto& leg = s.getObject(legend);
    leg.owner        = P1;
    leg.controller   = P1;
    leg.card_def_id  = kTestLegendId;
    leg.name         = "Empower Cost Test Legend";
    leg.card_type    = CardType::Legend;
    leg.zone         = ZoneType::LegendZone;
    leg.is_exhausted = false;
    leg.is_empowered = false;
    s.player(P1).legend_zone = legend;

    auto hasActivateFor = [&](GameObjectId id) {
        auto actions = engine.generateLegalActions();
        for (const auto& a : actions) {
            if (a.type == IntentType::ActivateAbility && a.ability_source == id)
                return true;
        }
        return false;
    };

    // Not empowered — the disempower_self ability must not be offered,
    // even though the legend is otherwise ready.
    EXPECT_FALSE(hasActivateFor(legend))
        << "disempower_self ability must be gated while the source is not "
           "empowered";

    // Empower it directly (bypassing whatever card would normally grant
    // Empowered) and confirm the ability is now offered.
    s.getObject(legend).is_empowered = true;
    EXPECT_TRUE(hasActivateFor(legend))
        << "disempower_self ability must be offered once the source is "
           "empowered";

    // Find and execute the intent via the engine's public test hook for
    // executeIntent — the same entry point the main-phase loop drives.
    Intent activate;
    activate.type = IntentType::ActivateAbility;
    activate.player = P1;
    activate.ability_source = legend;
    activate.ability_index = 0;

    engine.testHook_executeIntent(activate);

    EXPECT_FALSE(s.getObject(legend).is_empowered)
        << "activating a disempower_self ability must clear Empowered";
    EXPECT_TRUE(s.getObject(legend).is_exhausted)
        << "activating a disempower_self ability must also pay its "
           "exhaust component";
}

// ─── The disempower cost is re-validated at execution (review finding #6) ──
//
// The action generator gates a disempower_self ability on the source being
// empowered, but executeIntent is reachable with hand-built intents — agents,
// the OpenSpiel bridge, replays. It used to pay the cost's other components
// (exhaust, energy) and then call disempowerObject, which no-ops on a
// non-empowered source: the ability got activated for free. CR 828 — a cost
// that cannot be paid is not paid, and the activation does not happen.

TEST_F(EmpowerTest, DisempowerSelfActivationIsRejectedWhenTheSourceIsNotEmpowered) {
    constexpr CardDefId kTestLegendId = 900;
    card_registry.registerCard(kTestLegendId, std::make_unique<EmpowerCostTestLegend>());

    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    auto& s = engine.mutableState();
    s.mode             = ModeOfPlay{};
    s.players[0].id    = P1;
    s.players[1].id    = P2;
    s.turn.turn_player = P1;
    s.turn.phase       = TurnPhase::MainPhase;
    s.turn.ns_state    = NeutralShowdownState::Neutral;
    s.turn.oc_state    = OpenClosedState::Open;
    BattlefieldState b0; b0.id = 0; s.battlefields.push_back(b0);
    BattlefieldState b1; b1.id = 1; s.battlefields.push_back(b1);

    auto legend = s.createObject();
    auto& leg = s.getObject(legend);
    leg.owner        = P1;
    leg.controller   = P1;
    leg.card_def_id  = kTestLegendId;
    leg.name         = "Empower Cost Test Legend";
    leg.card_type    = CardType::Legend;
    leg.zone         = ZoneType::LegendZone;
    leg.is_exhausted = false;
    leg.is_empowered = false;   // the cost cannot be paid
    s.player(P1).legend_zone = legend;

    Intent activate;
    activate.type = IntentType::ActivateAbility;
    activate.player = P1;
    activate.ability_source = legend;
    activate.ability_index = 0;

    engine.testHook_executeIntent(activate);

    EXPECT_FALSE(s.getObject(legend).is_exhausted)
        << "The disempower component of the cost cannot be paid, so NOTHING "
           "is paid — the exhaust component must not be charged either.";
    EXPECT_FALSE(s.getObject(legend).is_empowered)
        << "sanity: the source was never empowered";
    EXPECT_FALSE(s.chain.exists())
        << "An activation whose cost cannot be paid does not happen: no chain "
           "item may be created for it.";
}

}  // namespace
