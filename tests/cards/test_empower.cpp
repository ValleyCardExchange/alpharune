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

}  // namespace
