/// @file test_play_from_non_hand.cpp
/// Kennen/Heart of the Tempest spec §2 + addendum #2/#3/#7:
/// `CardPlayedEvent::play_source` is derived from the played card's ZONE
/// at execution time (never from the intent), and
/// `TriggerType::WhenYouPlayFromNonHand` fires on the player's other
/// on-board cards + their legend whenever a CardPlayedEvent carries a
/// non-Hand source.
///
/// Test #4 (spec Section 8 / task-3-brief Step 1): a spell played from
/// hand does not fire the trigger; the SAME mechanism replaying a unit
/// out of trash (The Harrowing, id 198) does, and the observed event's
/// play_source is Trash.
///
/// Test #5 (addendum #3): a champion played from the champion zone fires
/// the trigger with play_source == ChampionZone.
///
/// Fix round 1 (controller-widened scope): a hidden card revealed and
/// played as a reaction goes through the LIVE CR 811 path,
/// `ChainManager::stepExecuteAndPass` (src/engine/chain_manager.cpp) —
/// NOT `GameEngine::executePlayFromHidden`, which nothing in src/ or
/// tests/ calls (dead code, left as-is per the controller's ruling).
/// Drives `ChainManager` + a hand-wired `TriggerManager` directly (no
/// full `GameEngine`, mirroring `driveThroughChain`'s shape in
/// card_test_fixture.h, since that helper doesn't cover the
/// facedown-reaction branch) to prove play_source == Hidden and the
/// WhenYouPlayFromNonHand dispatch on the real event-bus path.
///
/// Optional guard (addendum #7): EffectExecutor::createToken emits no
/// CardPlayedEvent, so token creation can never feed this trigger.

#include "tests/cards/card_test_fixture.h"

#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/events.h"
#include "core/game_state.h"
#include "engine/chain_manager.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"
#include "engine/trigger_manager.h"

#include <memory>
#include <vector>

using namespace riftbound;
using namespace riftbound::test;

namespace {

// The Harrowing (OGN-198): "[Action] Play a unit from your trash,
// ignoring its Energy cost." 6E + 2 Chaos P, resolves via
// EffectExecutor::playIgnoringCost — exactly the free-play helper this
// task wires the shared zone->source mapping into.
constexpr CardDefId kTheHarrowing = 198;

// Darius, Trifarian (OGN-027): a Champion unit, 5E + 1 Fury P. Used only
// as a cheap real champion to drive through the champion-zone play path;
// its own printed trigger (fires on cards_played_this_turn == 2) is a
// no-op here since it's the only card played.
constexpr CardDefId kDariusChampion = 27;

constexpr CardDefId kNonHandWatcherLegend = 970;
constexpr CardDefId kNonHandWatcherUnit = 971;

// Test-local legend whose only job is to record that
// WhenYouPlayFromNonHand fired on it, and what the triggering subject
// (the played card) was — mirrors EmpowerCostTestLegend's shape in
// test_empower.cpp.
class NonHandWatcherLegend : public LegendCard {
public:
    const CardDef& def() const override { return def_; }
    TriggerType triggerType() const override {
        return TriggerType::WhenYouPlayFromNonHand;
    }
    void onTrigger(CardContext& ctx, const std::vector<GameObjectId>&) override {
        if (!ctx.state.objectExists(ctx.source)) return;
        auto& obj = ctx.state.getObject(ctx.source);
        obj.card_counters["fired"] += 1;
        GameObjectId subject = ctx.state.chain.resuming
            ? ctx.state.chain.resuming->triggering_subject
            : kInvalidId;
        obj.card_counters["subject"] = static_cast<int>(subject);
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kNonHandWatcherLegend;
        d.name = "NonHand Watcher Legend";
        d.card_type = CardType::Legend;
        return d;
    }();
};

// Same trigger, but as a plain on-board card — exercises the
// "player's on-board cards" loop shape (same shape as WhenYouPlayASpell),
// separately from the legend broadcast.
class NonHandWatcherUnit : public UnitCard {
public:
    const CardDef& def() const override { return def_; }
    TriggerType triggerType() const override {
        return TriggerType::WhenYouPlayFromNonHand;
    }
    void onTrigger(CardContext& ctx, const std::vector<GameObjectId>&) override {
        if (!ctx.state.objectExists(ctx.source)) return;
        ctx.state.getObject(ctx.source).card_counters["fired"] += 1;
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kNonHandWatcherUnit;
        d.name = "NonHand Watcher Unit";
        d.card_type = CardType::Unit;
        return d;
    }();
};

void primeMainPhase(GameEngine& engine) {
    auto& s = engine.mutableState();
    s.mode             = ModeOfPlay{};
    s.players[0].id    = PlayerId::Player1;
    s.players[1].id    = PlayerId::Player2;
    s.turn.turn_player = PlayerId::Player1;
    s.turn.phase       = TurnPhase::MainPhase;
    s.turn.ns_state    = NeutralShowdownState::Neutral;
    s.turn.oc_state    = OpenClosedState::Open;
    BattlefieldState b0; b0.id = 0; s.battlefields.push_back(b0);
    BattlefieldState b1; b1.id = 1; s.battlefields.push_back(b1);
}

GameObjectId addReadyRune(GameState& s, PlayerId owner, Domain d) {
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

GameObjectId addRealCardToHand(GameState& s, const CardDB& card_db,
                                PlayerId owner, CardDefId def_id) {
    auto id = s.createObject();
    auto& obj = s.getObject(id);
    obj.owner = owner;
    obj.controller = owner;
    obj.card_def_id = def_id;
    const auto& def = card_db.get(def_id);
    obj.name = def.name;
    obj.card_type = def.card_type;
    obj.super_type = def.super_type;
    obj.domains = def.domains;
    obj.zone = ZoneType::Hand;
    s.player(owner).hand.push_back(id);
    return id;
}

Intent findPlayCardIntent(const std::vector<Intent>& actions, GameObjectId card_id) {
    for (const auto& a : actions) {
        if (a.type == IntentType::PlayCard && a.card == card_id) return a;
    }
    return Intent{};
}

}  // namespace

using PlayFromNonHandTest = CardTestFixture;

// ─── Test #4 (task-3-brief Step 1/2/4) ─────────────────────────────────────

TEST_F(PlayFromNonHandTest, HandPlayDoesNotFire_TrashReplayFiresWithTrashSource) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    // Watchers: a legend (legend_zone) AND a plain on-board unit, both
    // declaring WhenYouPlayFromNonHand — covers both halves of
    // TriggerManager::onCardPlayed's dispatch (the board-card loop and
    // the legend broadcast).
    card_registry.registerCard(kNonHandWatcherLegend,
                                std::make_unique<NonHandWatcherLegend>());
    card_registry.registerCard(kNonHandWatcherUnit,
                                std::make_unique<NonHandWatcherUnit>());

    auto legend_id = s.createObject();
    {
        auto& leg = s.getObject(legend_id);
        leg.owner = PlayerId::Player1;
        leg.controller = PlayerId::Player1;
        leg.card_def_id = kNonHandWatcherLegend;
        leg.name = "NonHand Watcher Legend";
        leg.card_type = CardType::Legend;
        leg.zone = ZoneType::LegendZone;
    }
    s.player(PlayerId::Player1).legend_zone = legend_id;

    auto watcher_unit_id = s.createObject();
    {
        auto& wu = s.getObject(watcher_unit_id);
        wu.owner = PlayerId::Player1;
        wu.controller = PlayerId::Player1;
        wu.card_def_id = kNonHandWatcherUnit;
        wu.name = "NonHand Watcher Unit";
        wu.card_type = CardType::Unit;
        wu.base_might = 1;
        wu.current_might = 1;
        wu.zone = ZoneType::Base;
        wu.location = BaseLocation{PlayerId::Player1};
    }

    // 10 ready Chaos runes — affords The Harrowing (6E + 2 Chaos P):
    // exhaust 6 for energy, recycle 2 of those exhausted Chaos runes for
    // power (CR cost-payment ordering; see test_dazzling_aurora.cpp).
    for (int i = 0; i < 10; ++i) addReadyRune(s, PlayerId::Player1, Domain::Chaos);

    // A unit sitting in P1's trash — The Harrowing's target, replayed
    // ignoring its energy cost via EffectExecutor::playIgnoringCost.
    auto unit_in_trash = s.createObject();
    {
        auto& u = s.getObject(unit_in_trash);
        u.owner = PlayerId::Player1;
        u.controller = PlayerId::Player1;
        u.card_type = CardType::Unit;
        u.name = "Trashed Test Unit";
        u.base_might = 1;
        u.current_might = 1;
        u.zone = ZoneType::Trash;
    }
    s.player(PlayerId::Player1).trash.push_back(unit_in_trash);

    auto harrowing_id = addRealCardToHand(s, card_db, PlayerId::Player1, kTheHarrowing);

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    auto actions = engine.generateLegalActions();
    Intent play = findPlayCardIntent(actions, harrowing_id);
    ASSERT_EQ(play.type, IntentType::PlayCard)
        << "The Harrowing (198, 6E+2 Chaos P) must be a legal hand play "
           "with 10 ready Chaos runes in Main Phase / Neutral Open.";

    engine.testHook_executeIntent(play);

    ASSERT_EQ(played.size(), 2u)
        << "Expected exactly two CardPlayedEvents: The Harrowing itself "
           "(from hand), then the trash unit it replays via "
           "playIgnoringCost.";
    EXPECT_EQ(played[0].object, harrowing_id);
    EXPECT_EQ(played[0].play_source, Intent::PlaySource::Hand)
        << "The Harrowing is played from hand — its own event must carry "
           "play_source == Hand.";
    EXPECT_EQ(played[1].object, unit_in_trash);
    EXPECT_EQ(played[1].play_source, Intent::PlaySource::Trash)
        << "The replayed unit's zone was Trash at the moment "
           "playIgnoringCost ran — its event must carry play_source == "
           "Trash, derived from the zone, not the (nonexistent) intent.";

    ASSERT_TRUE(s.objectExists(legend_id));
    EXPECT_EQ(s.getObject(legend_id).card_counters["fired"], 1)
        << "WhenYouPlayFromNonHand must fire on the legend exactly once — "
           "from the trash replay only, never from The Harrowing's own "
           "hand play (play_source == Hand must not dispatch it).";
    EXPECT_EQ(s.getObject(legend_id).card_counters["subject"],
              static_cast<int>(unit_in_trash))
        << "The played card (the trash-replayed unit) must be the chain "
           "item's triggering subject for the legend firing.";

    ASSERT_TRUE(s.objectExists(watcher_unit_id));
    EXPECT_EQ(s.getObject(watcher_unit_id).card_counters["fired"], 1)
        << "WhenYouPlayFromNonHand must also fire on the player's other "
           "on-board cards (same loop shape as WhenYouPlayASpell/AUnit), "
           "exactly once, from the trash replay only.";
}

// ─── Test #5 (addendum #3 / task-3-brief Step 5) ───────────────────────────

TEST_F(PlayFromNonHandTest, ChampionPlayedFromChampionZoneFiresWithChampionZoneSource) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    card_registry.registerCard(kNonHandWatcherLegend,
                                std::make_unique<NonHandWatcherLegend>());
    auto legend_id = s.createObject();
    {
        auto& leg = s.getObject(legend_id);
        leg.owner = PlayerId::Player1;
        leg.controller = PlayerId::Player1;
        leg.card_def_id = kNonHandWatcherLegend;
        leg.name = "NonHand Watcher Legend";
        leg.card_type = CardType::Legend;
        leg.zone = ZoneType::LegendZone;
    }
    s.player(PlayerId::Player1).legend_zone = legend_id;

    // Darius, Trifarian (27) in the champion zone: 5E + 1 Fury P.
    auto champion_id = s.createObject();
    {
        auto& c = s.getObject(champion_id);
        c.owner = PlayerId::Player1;
        c.controller = PlayerId::Player1;
        c.card_def_id = kDariusChampion;
        const auto& def = card_db.get(kDariusChampion);
        c.name = def.name;
        c.card_type = def.card_type;
        c.super_type = def.super_type;
        c.domains = def.domains;
        c.base_might = def.might;
        c.current_might = def.might;
        c.zone = ZoneType::ChampionZone;
    }
    s.player(PlayerId::Player1).champion_zone = champion_id;

    // 6 ready Fury runes: exhaust 5 for energy, recycle 1 exhausted Fury
    // for the 1 Body... no — the printed power domain is Fury, so 1 of
    // the 5 just-exhausted Fury runes covers the power too.
    for (int i = 0; i < 6; ++i) addReadyRune(s, PlayerId::Player1, Domain::Fury);

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    auto actions = engine.generateLegalActions();
    Intent play = findPlayCardIntent(actions, champion_id);
    ASSERT_EQ(play.type, IntentType::PlayCard)
        << "Darius (5E+1 Fury P) must be a legal champion-zone play with "
           "6 ready Fury runes.";

    engine.testHook_executeIntent(play);

    ASSERT_EQ(played.size(), 1u);
    EXPECT_EQ(played[0].object, champion_id);
    EXPECT_EQ(played[0].play_source, Intent::PlaySource::ChampionZone)
        << "A champion played from the champion zone must carry "
           "play_source == ChampionZone (addendum #3 — champion zone is "
           "not hand; no errata makes it one).";

    ASSERT_TRUE(s.objectExists(legend_id));
    EXPECT_EQ(s.getObject(legend_id).card_counters["fired"], 1)
        << "Playing the champion from the champion zone must fire "
           "WhenYouPlayFromNonHand on the legend.";
}

// ─── Fix round 1: the LIVE hidden-reveal-as-reaction path ──────────────────

TEST_F(PlayFromNonHandTest, HiddenCardRevealedAsReactionFiresWithHiddenSource) {
    // The real CR 811 facedown-reveal-as-reaction path is
    // ChainManager::stepExecuteAndPass: a hidden card is offered as a
    // PlayReaction intent while still facedown, and the branch handles
    // everything (unhide, track play, emit CardPlayedEvent, add to
    // chain) inside ChainManager itself — GameEngine::executePlayFromHidden
    // is a separate, dead code path (nothing in src/ or tests/ calls it).
    //
    // No full GameEngine here: ChainManager + a directly-wired
    // TriggerManager, bound to the fixture's own state/events/card_db,
    // driven the way card_test_fixture.h's driveThroughChain drives a
    // spell through FEPR — that helper only covers hand plays, so this
    // test assembles the same pieces by hand for the facedown-reaction
    // branch specifically.
    card_registry.registerCard(kNonHandWatcherLegend,
                                std::make_unique<NonHandWatcherLegend>());
    auto legend_id = state.createObject();
    {
        auto& leg = state.getObject(legend_id);
        leg.owner = P1;
        leg.controller = P1;
        leg.card_def_id = kNonHandWatcherLegend;
        leg.name = "NonHand Watcher Legend";
        leg.card_type = CardType::Legend;
        leg.zone = ZoneType::LegendZone;
    }
    state.player(P1).legend_zone = legend_id;

    // A hidden card at BF#0 — offered as a PlayReaction while facedown.
    auto hidden_card = state.createObject();
    {
        auto& hc = state.getObject(hidden_card);
        hc.owner = P1;
        hc.controller = P1;
        hc.card_type = CardType::Spell;
        hc.name = "Hidden Test Reaction";
        hc.zone = ZoneType::FacedownZone;
        hc.is_hidden = true;
        hc.hidden_at = 0;
    }
    state.battlefields[0].facedown.push_back(hidden_card);

    // A dummy spell already on the chain, controlled by the same
    // player — opens the Execute/Pass priority window that the hidden
    // reaction is offered into (CR: newest item's controller gets
    // priority first, so P1 can respond to their own item immediately).
    auto dummy_spell = state.createObject();
    {
        auto& ds = state.getObject(dummy_spell);
        ds.owner = P1;
        ds.controller = P1;
        ds.card_type = CardType::Spell;
        ds.name = "Dummy Chain Spell";
    }

    ChainManager cm(state, events, card_db);
    EffectExecutor exec(state, events, card_db, &card_registry);
    cm.setEffectExecutor(&exec);
    TriggerManager tm(state, events, card_db, cm, card_registry);
    tm.setEffectExecutor(&exec);
    tm.subscribe();

    cm.addSpell(dummy_spell, P1, {});

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    // Drive the FEPR loop by hand: on the very first priority query,
    // play the hidden card as a reaction; every subsequent query passes
    // priority so the chain drains normally. stepExecuteAndPass only
    // inspects chosen.type/card/targets (never the injected `actions`
    // list), so a real legal-action generator isn't needed here — this
    // is "driving ChainManager directly."
    int priority_call = 0;
    auto query_agent = [&](PlayerId, const std::vector<Intent>&) -> Intent {
        ++priority_call;
        if (priority_call == 1) {
            Intent react;
            react.type = IntentType::PlayReaction;
            react.player = PlayerId::Player1;
            react.card = hidden_card;
            return react;
        }
        Intent pass;
        pass.type = IntentType::PassPriority;
        pass.player = PlayerId::Player1;
        return pass;
    };

    cm.processFEPR(
        query_agent,
        [](const ChainItem&) { /* no permanents added in this test */ },
        [&](const ChainItem& item) {
            // Mirrors GameEngine::resolveSpell's is_ability/onTrigger vs
            // onResolve dispatch, simplified (no target re-validation —
            // neither item here has real targets).
            Card* c = card_registry.get(item.card_def_id);
            if (!c) return;
            CardContext ctx{state, events, exec, item.controller, item.source};
            ctx.firing_trigger = item.fired_trigger;
            if (item.is_ability) {
                c->onTrigger(ctx, item.targets);
            } else {
                c->onResolve(ctx, item.targets);
            }
        },
        [](PlayerId) { return std::vector<Intent>{}; });

    ASSERT_FALSE(state.chain.exists())
        << "Sanity: the chain must have fully drained (dummy spell + the "
           "hidden card's own spell body + the fired ability all resolve).";

    ASSERT_EQ(played.size(), 1u)
        << "Only the hidden card's reveal-and-play emits a "
           "CardPlayedEvent (the dummy spell was already on the chain "
           "before this test's event subscription was installed).";
    EXPECT_EQ(played[0].object, hidden_card);
    EXPECT_EQ(played[0].play_source, Intent::PlaySource::Hidden)
        << "A facedown card revealed and played as a reaction through "
           "the LIVE CR 811 path (ChainManager::stepExecuteAndPass) must "
           "carry play_source == Hidden.";

    ASSERT_TRUE(state.objectExists(legend_id));
    EXPECT_EQ(state.getObject(legend_id).card_counters["fired"], 1)
        << "WhenYouPlayFromNonHand must fire on the legend for a live "
           "hidden-reveal-as-reaction play.";
    EXPECT_EQ(state.getObject(legend_id).card_counters["subject"],
              static_cast<int>(hidden_card))
        << "The revealed card must be the chain item's triggering subject.";

    EXPECT_FALSE(state.getObject(hidden_card).is_hidden)
        << "Sanity: the reveal actually happened.";
}

// ─── Optional guard (addendum #7) ──────────────────────────────────────────

TEST_F(CardTestFixture, TokenCreationEmitsNoCardPlayedEvent) {
    // Tokens are not cards (CR 185, 350.2) — createToken must never feed
    // WhenYouPlayFromNonHand (or any other play trigger).
    EffectExecutor exec(state, events, card_db, &card_registry);

    int card_played_count = 0;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent&) { ++card_played_count; });

    exec.createToken(P1, CardType::Unit, "Test Token", /*might=*/1, {}, {},
                      LocationId{BaseLocation{P1}}, /*enter_ready=*/false);

    EXPECT_EQ(card_played_count, 0)
        << "createToken must not emit CardPlayedEvent — tokens are not "
           "cards and must never dispatch WhenYouPlayFromNonHand.";
}
