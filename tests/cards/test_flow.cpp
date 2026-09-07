/// @file test_flow.cpp
/// Kennen/Heart of the Tempest spec §3 + addendum #1/#6: the Flow
/// alternate-cost subsystem (CR 829).
///
/// Flow lets a spell be played FROM THE TRASH for its flow cost, which
/// REPLACES the base cost (CR 829.1.c.1). Timing is unchanged (CR
/// 829.1.b.2 — the Action/Reaction gate the hand path uses), the play is
/// tagged `play_source = Trash` + `flow_source`, and the chain item is
/// marked `banish_on_leave` (CR 829.1.b.1 — the disposal itself is Task 5;
/// this file only proves the flag is set).
///
/// Tests here, numbered per spec Section 8 / the addendum:
///   #6  — offered once from trash when the FLOW cost is affordable, and
///         not offered when only the printed cost is.
///   #7  — timing parity in the closed state: the [Reaction] flow spell is
///         offered as a PlayReaction, the [Action]-only one is not.
///   #8  — executing a flow play charges exactly the flow cost (2 runes
///         exhausted for energy + 1 recycled for power), never the printed
///         cost on top; the card leaves the trash and the chain item it
///         creates carries banish_on_leave.
///   #12 — a trash spell with BOTH a Death-from-Below replay grant AND a
///         printed Flow cost yields two distinct intents (one per cost).
///   #29 — (addendum #1) a live GRANTED flow plus a printed one yields two
///         flow intents, one per cost, and each pays its own cost.
///
/// Fix round 1 adds the execution branch's failure modes and the offer paths
/// the first round left uncovered: a hand-tagged or stale `flow_source` is
/// rejected loudly with nothing mutated and no grant consumed; a printed-cost
/// play leaves a live grant alone; the showdown call site offers flow plays;
/// and a spell lockout suppresses them.

#include "tests/cards/card_test_fixture.h"

#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/events.h"
#include "core/game_state.h"
#include "engine/game_engine.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace riftbound;
using namespace riftbound::test;

namespace {

// Test-local ids, well above the shipped registry (787 cards).
constexpr CardDefId kFlowActionSpell   = 901;
constexpr CardDefId kFlowReactionSpell = 902;

/// Base shape for both test spells: printed 1E (cheap, so "only the printed
/// cost is affordable" is a reachable state), Flow 2E + 1 power in ANY
/// domain. No targets, no ability text — the point is the cost path, not the
/// effect.
CardDef makeFlowSpellDef(CardDefId id, const char* name, Keyword timing) {
    CardDef d;
    d.id = id;
    d.name = name;
    d.card_type = CardType::Spell;
    d.domains = {Domain::Fury};
    d.energy_cost = 1;
    d.keywords.set(Keyword::Flow);
    d.keywords.set(timing);
    d.flow_energy = 2;
    d.flow_power = 1;
    d.flow_any_domain = true;
    return d;
}

class FlowActionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ =
        makeFlowSpellDef(kFlowActionSpell, "Flow Action Test Spell",
                          Keyword::Action);
};

class FlowReactionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ =
        makeFlowSpellDef(kFlowReactionSpell, "Flow Reaction Test Spell",
                          Keyword::Reaction);
};

}  // namespace

// ─── Fixture ───────────────────────────────────────────────────────────────

class FlowTest : public CardTestFixture {
protected:
    void SetUp() override {
        CardTestFixture::SetUp();
        // Register the test-local spells and REBUILD the CardDB from the
        // registry: executePlaySpell reads card_db_ (ability_text for
        // [Repeat], printed energy_cost) and CardDB::get throws on an
        // unknown id.
        card_registry.registerCard(kFlowActionSpell,
                                    std::make_unique<FlowActionSpell>());
        card_registry.registerCard(kFlowReactionSpell,
                                    std::make_unique<FlowReactionSpell>());
        card_db.buildFromClasses(card_registry);
    }

    /// Main Phase / Neutral Open, P1 to act.
    void primeMainPhase(GameEngine& engine) {
        auto& s = engine.mutableState();
        s.mode             = ModeOfPlay{};
        s.players[0].id    = P1;
        s.players[1].id    = P2;
        s.turn.turn_player = P1;
        s.turn.turn_number = 3;
        s.turn.phase       = TurnPhase::MainPhase;
        s.turn.ns_state    = NeutralShowdownState::Neutral;
        s.turn.oc_state    = OpenClosedState::Open;
        BattlefieldState b0; b0.id = 0; s.battlefields.push_back(b0);
        BattlefieldState b1; b1.id = 1; s.battlefields.push_back(b1);
    }

    /// Closed State with P1 holding priority (CR 309.1.a — Reactions only).
    void primeClosedState(GameEngine& engine) {
        primeMainPhase(engine);
        auto& s = engine.mutableState();
        s.turn.oc_state       = OpenClosedState::Closed;
        s.turn.priority_holder = P1;
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

    /// Put a registered card object straight into `owner`'s trash. Builds
    /// into the ENGINE's state (the base fixture's addToHand/addToDeck build
    /// into the fixture-owned `state`, which the engine never sees).
    GameObjectId addToZoneIn(GameState& s, PlayerId owner, CardDefId def_id,
                              ZoneType zone) {
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
        obj.zone = zone;
        if (zone == ZoneType::Trash) s.player(owner).trash.push_back(id);
        else if (zone == ZoneType::Hand) s.player(owner).hand.push_back(id);
        return id;
    }

    GameObjectId addToTrash(GameState& s, PlayerId owner, CardDefId def_id) {
        return addToZoneIn(s, owner, def_id, ZoneType::Trash);
    }

    GameObjectId addToHandIn(GameState& s, PlayerId owner, CardDefId def_id) {
        return addToZoneIn(s, owner, def_id, ZoneType::Hand);
    }

    static std::vector<Intent> intentsFor(const std::vector<Intent>& actions,
                                           GameObjectId card_id) {
        std::vector<Intent> out;
        for (const auto& a : actions) {
            if (a.card != card_id) continue;
            if (a.type != IntentType::PlayCard &&
                a.type != IntentType::PlayActionCard &&
                a.type != IntentType::PlayReaction) continue;
            out.push_back(a);
        }
        return out;
    }

    static int countExhausted(const GameState& s, PlayerId owner) {
        int n = 0;
        for (const auto& [id, obj] : s.objects) {
            if (obj.controller != owner) continue;
            if (!obj.isRune()) continue;
            if (obj.zone != ZoneType::Base) continue;
            if (obj.is_exhausted) ++n;
        }
        return n;
    }

    static int countReady(const GameState& s, PlayerId owner) {
        int n = 0;
        for (const auto& [id, obj] : s.objects) {
            if (obj.controller != owner) continue;
            if (!obj.isRune()) continue;
            if (obj.zone != ZoneType::Base) continue;
            if (!obj.is_exhausted) ++n;
        }
        return n;
    }
};

// ─── Test #6: the offer ────────────────────────────────────────────────────

TEST_F(FlowTest, OfferedFromTrashWhenFlowCostAffordable) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);

    // 4 ready Fury runes: the flow cost (2E + 1 [A] power) is payable —
    // recycle 1 for power, exhaust 2 for energy, 1 left over.
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "A [Flow] spell in the trash with an affordable flow cost must be "
           "offered exactly once (one intent per live flow cost — spec "
           "addendum #1; only the printed cost is live here).";
    EXPECT_EQ(offers[0].type, IntentType::PlayCard);
    EXPECT_EQ(offers[0].play_source, Intent::PlaySource::Trash)
        << "A flow play comes out of the trash — the intent must say so.";
    EXPECT_EQ(offers[0].flow_source, Intent::FlowSource::Printed)
        << "The offer must name the cost it pays: the card's own printed "
           "[Flow] cost.";
}

TEST_F(FlowTest, NotOfferedWhenOnlyThePrintedCostIsAffordable) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);

    // 1 ready Fury rune: enough for the PRINTED 1E cost, nowhere near the
    // 2E + 1P flow cost. Flow is an alternate cost — affordability is
    // checked against IT, never against the printed one.
    addReadyRune(s, P1, Domain::Fury);

    EXPECT_TRUE(intentsFor(engine.generateLegalActions(), spell).empty())
        << "The flow offer must be gated on the FLOW cost. One ready rune "
           "affords the printed 1E but not [E2][P1], so nothing may be "
           "offered for this trash spell.";
}

// ─── Test #7: timing parity in the closed state ────────────────────────────

TEST_F(FlowTest, ClosedStateOffersOnlyTheReactionFlowSpell) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeClosedState(engine);
    auto& s = engine.mutableState();

    auto action_spell   = addToTrash(s, P1, kFlowActionSpell);
    auto reaction_spell = addToTrash(s, P1, kFlowReactionSpell);

    // 8 ready Fury runes — both flow costs are comfortably affordable, so
    // the only thing that can suppress an offer is the timing gate.
    for (int i = 0; i < 8; ++i) addReadyRune(s, P1, Domain::Fury);

    auto actions = engine.generateLegalActions();

    EXPECT_TRUE(intentsFor(actions, action_spell).empty())
        << "CR 309.1.a — in the Closed State only [Reaction] spells may be "
           "played. A flow play gets the SAME timing gate as a hand play, so "
           "the [Action]-only flow spell must not be offered.";

    auto reaction_offers = intentsFor(actions, reaction_spell);
    ASSERT_EQ(reaction_offers.size(), 1u)
        << "The [Reaction] flow spell must still be offered in the Closed "
           "State (timing is unchanged by Flow — CR 829.1.b.2).";
    EXPECT_EQ(reaction_offers[0].type, IntentType::PlayReaction)
        << "Closed-State plays use the PlayReaction intent type, exactly as "
           "the hand path does.";
    EXPECT_EQ(reaction_offers[0].play_source, Intent::PlaySource::Trash);
    EXPECT_EQ(reaction_offers[0].flow_source, Intent::FlowSource::Printed);
}

// ─── Test #8: paying the flow cost ─────────────────────────────────────────

TEST_F(FlowTest, FlowPlayPaysOnlyTheFlowCostAndBanishesOnLeave) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    // The chain item is popped before the spell resolves, and (Task 5 aside)
    // the spell lands back in the trash afterwards — so the "on the chain,
    // out of the trash" facts have to be sampled while the item is live.
    // ChainItemFinalizedEvent fires with the item still in state.chain.items.
    bool saw_item = false;
    bool banish_on_leave = false;
    int chain_energy = -1;
    bool in_trash_while_on_chain = true;
    ZoneType zone_while_on_chain = ZoneType::Trash;
    auto fin = events.on_chain_item_finalized.connect(
        [&](const ChainItemFinalizedEvent& e) {
            if (e.source != spell) return;
            for (const auto& it : s.chain.items) {
                if (it.id != e.item) continue;
                saw_item = true;
                banish_on_leave = it.banish_on_leave;
                chain_energy = it.total_energy_spent;
            }
            const auto& t = s.player(P1).trash;
            in_trash_while_on_chain =
                std::find(t.begin(), t.end(), spell) != t.end();
            zone_while_on_chain = s.getObject(spell).zone;
        });

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u) << "sanity: the flow play must be offered";
    ASSERT_EQ(offers[0].flow_source, Intent::FlowSource::Printed);

    engine.testHook_executeIntent(offers[0]);

    // ── Rune deltas: exactly the flow cost, [E2][P1] ──
    // payAdditionalCost recycles 1 Fury rune for the power and exhausts 2
    // ready runes for the energy. The printed 1E must NOT be charged on top:
    // with 4 runes that would leave 3 exhausted and 0 ready.
    EXPECT_EQ(countExhausted(s, P1), 2)
        << "Exactly 2 runes may be exhausted — the flow cost's [E2]. A third "
           "exhausted rune means the printed 1E was charged as well, which "
           "Flow explicitly REPLACES (CR 829.1.c.1).";
    EXPECT_EQ(countReady(s, P1), 1)
        << "4 runes - 2 exhausted for energy - 1 recycled for power = 1 still "
           "ready.";
    EXPECT_EQ(s.player(P1).rune_deck.size(), 1u)
        << "The flow cost's [P1] is paid by recycling exactly one Fury rune "
           "back into the rune deck.";

    // ── The event/tracking numbers report the FLOW energy, not the printed ──
    ASSERT_EQ(played.size(), 1u);
    EXPECT_EQ(played[0].object, spell);
    EXPECT_EQ(played[0].play_source, Intent::PlaySource::Trash);
    EXPECT_EQ(played[0].energy_spent, 2)
        << "CardPlayedEvent must report the energy actually paid (the flow "
           "cost's 2), not the printed 1.";
    EXPECT_EQ(s.player(P1).max_spell_spent_this_turn, 2)
        << "max_spell_spent_this_turn gates cards like Jhin — it must count "
           "the flow energy actually spent.";

    // ── Chain item: out of the trash, on the chain, flagged for banish ──
    ASSERT_TRUE(saw_item)
        << "The flow play must put a chain item on the chain for the spell.";
    EXPECT_FALSE(in_trash_while_on_chain)
        << "The spell must have left the trash to be on the chain.";
    EXPECT_NE(zone_while_on_chain, ZoneType::Trash)
        << "The spell object's zone must no longer be Trash while it is on "
           "the chain.";
    EXPECT_EQ(chain_energy, 2);
    EXPECT_TRUE(banish_on_leave)
        << "CR 829.1.b.1 — a spell played for its Flow cost is banished as it "
           "leaves the chain; executePlaySpell must set banish_on_leave on "
           "the chain item (Task 5 performs the disposal).";
}

// ─── Test #12: a replay grant and a printed Flow are independent offers ────

TEST_F(FlowTest, TrashReplayGrantAndPrintedFlowYieldTwoDistinctIntents) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    // Death from Below (747) pushes exactly this grant on resolve: "play a
    // spell from your trash for [1]". Pushed straight onto the state here —
    // the card's own resolution is not what's under test.
    PlayerState::TrashReplayGrant grant;
    grant.card = spell;
    grant.energy = 1;
    grant.power = 0;
    grant.any_domain = true;
    s.player(P1).trash_replay_grants.push_back(grant);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 2u)
        << "The two permissions are independent costs, so the spell must be "
           "offered twice: once for the Death-from-Below grant and once for "
           "its own printed [Flow] cost. Neither generator may swallow the "
           "other's offer.";

    int grant_offers = 0, flow_offers = 0;
    for (const auto& o : offers) {
        EXPECT_EQ(o.play_source, Intent::PlaySource::Trash);
        if (o.flow_source == Intent::FlowSource::None) ++grant_offers;
        else if (o.flow_source == Intent::FlowSource::Printed) ++flow_offers;
    }
    EXPECT_EQ(grant_offers, 1)
        << "generateTrashReplayActions must still emit its untagged "
           "(flow_source == None) grant play.";
    EXPECT_EQ(flow_offers, 1)
        << "generateFlowPlayActions must emit the printed-Flow play alongside "
           "it.";
}

// ─── Test #29 (addendum #1): granted + printed → two flow intents ──────────

TEST_F(FlowTest, GrantedAndPrintedFlowYieldTwoIntentsEachPayingItsOwnCost) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    // Kennen's grant: a CHEAPER, power-free flow cost, live only this turn.
    // Deliberately different from the printed [E2][P1] so the two payments
    // are distinguishable by rune deltas alone.
    GameObject::GrantedFlow gf;
    gf.energy = 1;
    gf.power = 0;
    gf.any_domain = false;
    gf.power_domain = Domain::Fury;
    gf.valid_on_turn = s.turn.turn_number;
    s.getObject(spell).granted_flow = gf;

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 2u)
        << "CR 829.1.c.3 / addendum #1 — both live Flow costs are offered and "
           "the controller chooses; one intent per cost.";

    Intent printed_play{}, granted_play{};
    for (const auto& o : offers) {
        EXPECT_EQ(o.play_source, Intent::PlaySource::Trash);
        if (o.flow_source == Intent::FlowSource::Printed) printed_play = o;
        if (o.flow_source == Intent::FlowSource::Granted) granted_play = o;
    }
    ASSERT_EQ(printed_play.flow_source, Intent::FlowSource::Printed)
        << "the printed [Flow] cost must still be offered while a grant is "
           "live — the grant does not replace it";
    ASSERT_EQ(granted_play.flow_source, Intent::FlowSource::Granted)
        << "a grant stamped with the current turn must be offered";

    // A stale grant (last turn's) is not live — the evaluated-expiry rule.
    s.getObject(spell).granted_flow->valid_on_turn = s.turn.turn_number - 1;
    EXPECT_EQ(intentsFor(engine.generateLegalActions(), spell).size(), 1u)
        << "\"until end of turn\" expiry is EVALUATED: a grant stamped with an "
           "earlier turn is dead, leaving only the printed offer.";
    s.getObject(spell).granted_flow->valid_on_turn = s.turn.turn_number;

    // Execute the GRANTED offer: it must pay the GRANT's [E1], not the
    // printed [E2][P1].
    engine.testHook_executeIntent(granted_play);

    EXPECT_EQ(countExhausted(s, P1), 1)
        << "The granted flow cost is [E1] — exactly one rune exhausted. Two "
           "would mean the printed flow cost was paid instead.";
    EXPECT_EQ(countReady(s, P1), 3);
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "The granted cost has no power component, so nothing is recycled.";
    EXPECT_EQ(s.player(P1).max_spell_spent_this_turn, 1)
        << "The energy reported must be the granted cost actually paid.";
    EXPECT_FALSE(s.getObject(spell).granted_flow.has_value())
        << "A granted Flow is consumed by the play — it must be cleared so a "
           "later return to the trash can't reuse it.";
}

// ─── Fix round 1 — the execution branch validates its intent ───────────────
//
// A flow_source tag on an Intent is a CLAIM, and executePlaySpell is
// reachable with hand-built intents (agents, the OpenSpiel bridge, replays).
// The claim is re-checked against live state before anything mutates, and an
// intent that fails the check is rejected LOUDLY rather than quietly
// degrading into a printed-cost play.

TEST_F(FlowTest, HandTaggedFlowIntentIsRejectedAndMutatesNothing) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    // The SAME card, but in hand — a flow cost is only ever payable out of
    // the trash (CR 829.1.b).
    auto hand_spell = addToHandIn(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });
    int warnings = 0;
    auto log = events.on_log.connect([&](const LogEvent& e) {
        if (e.level == LogLevel::Warning &&
            e.message.find("FLOW: illegal flow intent") != std::string::npos)
            ++warnings;
    });

    // Hand-forged: claims the printed flow cost for a card that is in HAND.
    Intent forged;
    forged.type = IntentType::PlayCard;
    forged.player = P1;
    forged.card = hand_spell;
    forged.play_source = Intent::PlaySource::Hand;
    forged.flow_source = Intent::FlowSource::Printed;
    engine.testHook_executeIntent(forged);

    EXPECT_TRUE(played.empty())
        << "A flow claim on a hand play must be rejected outright — no card "
           "may be played at all.";
    EXPECT_EQ(countExhausted(s, P1), 0)
        << "Rejection must happen BEFORE any mutation: not one rune may be "
           "exhausted.";
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "…and not one rune recycled.";
    EXPECT_EQ(s.player(P1).hand.size(), 1u)
        << "The card must still be in hand.";
    EXPECT_TRUE(s.chain.items.empty())
        << "Nothing may reach the chain.";
    EXPECT_EQ(s.player(P1).cards_played_this_turn, 0);

    // Same forgery, relabelled play_source = Trash: the card still is not in
    // the trash, and liveFlowCosts only speaks for objects that are.
    Intent forged_trash = forged;
    forged_trash.play_source = Intent::PlaySource::Trash;
    engine.testHook_executeIntent(forged_trash);

    EXPECT_TRUE(played.empty())
        << "Relabelling play_source must not launder the claim — the object's "
           "actual zone decides.";
    EXPECT_EQ(countExhausted(s, P1), 0);
    EXPECT_EQ(s.player(P1).hand.size(), 1u);
    EXPECT_TRUE(s.chain.items.empty());

    EXPECT_EQ(warnings, 2)
        << "Both rejections must fail LOUDLY: a WARNING-level "
           "\"FLOW: illegal flow intent\" line each. Trace level would be "
           "invisible in an ordinary run.";
}

TEST_F(FlowTest, StaleFlowSourceIsRejectedAndLeavesGrantsIntact) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    // A grant that expired LAST turn — dead by the evaluated-expiry rule, so
    // there is no live Granted offer for the intent below to name.
    GameObject::GrantedFlow gf;
    gf.energy = 1;
    gf.valid_on_turn = s.turn.turn_number - 1;
    s.getObject(spell).granted_flow = gf;

    // An unrelated Death-from-Below grant sitting on the same player: the
    // rejected flow play must not consume it as a consolation payment.
    PlayerState::TrashReplayGrant replay_grant;
    replay_grant.card = spell;
    replay_grant.energy = 1;
    replay_grant.any_domain = true;
    s.player(P1).trash_replay_grants.push_back(replay_grant);

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });
    std::vector<std::string> warnings;
    auto log = events.on_log.connect([&](const LogEvent& e) {
        if (e.level == LogLevel::Warning) warnings.push_back(e.message);
    });

    Intent stale;
    stale.type = IntentType::PlayCard;
    stale.player = P1;
    stale.card = spell;
    stale.play_source = Intent::PlaySource::Trash;
    stale.flow_source = Intent::FlowSource::Granted;
    engine.testHook_executeIntent(stale);

    EXPECT_TRUE(played.empty())
        << "A flow_source naming a cost that is no longer live must be "
           "rejected, not silently downgraded to the printed cost.";
    EXPECT_EQ(countExhausted(s, P1), 0)
        << "Rejection happens before any payment.";
    EXPECT_EQ(s.player(P1).trash.size(), 1u)
        << "The card must still be in the trash.";
    EXPECT_TRUE(s.chain.items.empty());
    EXPECT_TRUE(s.getObject(spell).granted_flow.has_value())
        << "A rejected play consumes nothing — the (dead) grant record must "
           "survive, since clearing it is the payment's job, not the "
           "rejection's.";
    EXPECT_EQ(s.player(P1).trash_replay_grants.size(), 1u)
        << "The unrelated Death-from-Below grant must not be eaten by a "
           "rejected flow play.";

    ASSERT_EQ(warnings.size(), 1u)
        << "The rejection must fail LOUDLY — exactly one WARNING line.";
    EXPECT_NE(warnings[0].find("FLOW: illegal flow intent"), std::string::npos)
        << "actual: " << warnings[0];
    EXPECT_NE(warnings[0].find("no live granted flow cost"), std::string::npos)
        << "The warning must name WHY the intent was rejected. actual: "
        << warnings[0];
}

TEST_F(FlowTest, PrintedFlowPlayLeavesALiveGrantInPlace) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    GameObject::GrantedFlow gf;
    gf.energy = 1;
    gf.valid_on_turn = s.turn.turn_number;
    s.getObject(spell).granted_flow = gf;

    Intent printed_play{};
    for (const auto& o : intentsFor(engine.generateLegalActions(), spell))
        if (o.flow_source == Intent::FlowSource::Printed) printed_play = o;
    ASSERT_EQ(printed_play.flow_source, Intent::FlowSource::Printed);

    engine.testHook_executeIntent(printed_play);

    EXPECT_EQ(countExhausted(s, P1), 2)
        << "The PRINTED flow cost [E2] was chosen — the cheaper live grant "
           "must not be substituted for it.";
    EXPECT_EQ(s.player(P1).rune_deck.size(), 1u)
        << "…including its [P1], paid by recycling one rune.";
    EXPECT_TRUE(s.getObject(spell).granted_flow.has_value())
        << "Paying the PRINTED cost consumes nothing granted — only a play "
           "that actually spends the grant may clear it.";
}

// ─── Fix round 1 — coverage of the remaining offer paths ───────────────────

TEST_F(FlowTest, ShowdownCallSiteOffersFlowPlays) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();
    s.turn.ns_state    = NeutralShowdownState::Showdown;
    s.turn.oc_state    = OpenClosedState::Open;
    s.turn.focus_holder = P1;

    auto spell = addToTrash(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "generateFlowPlayActions must be wired into the SHOWDOWN action "
           "generator too, not just the main phase and closed state.";
    EXPECT_EQ(offers[0].type, IntentType::PlayActionCard)
        << "Showdown plays use PlayActionCard, as the hand path does.";
    EXPECT_EQ(offers[0].flow_source, Intent::FlowSource::Printed);
    EXPECT_EQ(offers[0].play_source, Intent::PlaySource::Trash);
}

TEST_F(FlowTest, CantPlaySpellsThisTurnSuppressesFlowOffers) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto spell = addToTrash(s, P1, kFlowActionSpell);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    ASSERT_EQ(intentsFor(engine.generateLegalActions(), spell).size(), 1u)
        << "sanity: the flow play is offered while nothing suppresses it";

    s.player(P1).cant_play_spells_this_turn = true;
    EXPECT_TRUE(intentsFor(engine.generateLegalActions(), spell).empty())
        << "A spell lockout suppresses flow plays exactly as it suppresses "
           "hand plays and trash-replay plays — Flow changes the cost, not "
           "the permission to play a spell at all.";
}
