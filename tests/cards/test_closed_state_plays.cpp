/// @file test_closed_state_plays.cpp
/// Closed-State (CR 309.1.a / 337) plays driven through the REAL chain — the
/// whole-branch review's critical finding #1 and its non-spell mirror.
///
/// `GameEngine::generateClosedStateActions` offers `IntentType::PlayReaction`
/// for hand spells, trash-replay grants, [Flow] plays, [Quick-Draw] gear,
/// [Ambush] and Rengar-style units, and facedown reveals of either kind.
/// Every one of those offers is answered by `ChainManager::stepExecuteAndPass`
/// and by nothing else: `GameEngine::executeIntent` does have a PlayReaction
/// case, but it serves the SHOWDOWN decision path
/// (`resolveShowdownDecision`), and ChainManager never calls executeIntent —
/// the two paths are disjoint. See
/// tests/cards/test_combat_showdown_dispatch.cpp for the showdown half.
///
/// stepExecuteAndPass used to hand-roll BOTH halves of the play, and both
/// were wrong. The spell copy paid via the injected `payCardCost` (no Flow,
/// no Sandswept Tomb staging), searched only `PlayerState::hand` for removal
/// (so a trash/Flow play was never removed from the trash and came back as a
/// DUPLICATE trash entry after resolving), never set `banish_on_leave`, never
/// consumed a granted Flow, and never stamped
/// `target_battlefield_restriction`. The non-spell copy ended in `addSpell`,
/// which sets `is_spell` — so a gear, an Ambush unit or a facedown PERMANENT
/// was paid for and then disposed into the TRASH instead of reaching the
/// board.
///
/// Both halves now route back out to the engine's real executors —
/// `GameEngine::executePlaySpell` and `GameEngine::executePlayCard` — via
/// injected callbacks, so one executor owns every play of each kind. These
/// tests drive the real FEPR loop:
///
///   (a) a [Reaction][Flow] spell in the trash, played in the Closed State,
///       pays the FLOW cost, leaves the trash exactly once, and ends in
///       banishment with no duplicate trash entry (CR 829.1.b.1/c.1);
///   (b) a granted-Flow [Reaction] spell (the Kennen 789 grant shape) played
///       closed pays the GRANTED cost and consumes the grant;
///   (c) Star-Crossed (690) played closed as a restricted Sandswept Tomb
///       (792) offer pays the discounted power and narrows the pair picker;
///   (d) a plain hand [Reaction] play still behaves exactly as before
///       (regression);
///   (e) a real facedown SPELL reveal through the chain emits
///       `PlayedFromFacedownEvent` (Katarina 585's WhenYouPlayFromFacedown) —
///       review finding #4: before the fix the only emit site was the dead
///       `GameEngine::executePlayFromHidden`, so the event never fired live;
///   (f) the four PERMANENT reactions — Pouncing, Ambush, Quick-Draw and a
///       facedown permanent reveal — land on the board rather than in trash.

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

// Test-local ids, well above the shipped registry (787 cards) and clear of
// the 90x block test_flow.cpp uses.
constexpr CardDefId kOpener           = 910;  // [Action], free — opens the chain
constexpr CardDefId kHandReaction     = 911;  // [Reaction] 1E, plain hand play
constexpr CardDefId kFlowReaction     = 912;  // [Reaction][Flow] 1E / flow 2E+1[A]
constexpr CardDefId kGrantedReaction  = 913;  // [Reaction] 3E, no printed Flow
constexpr CardDefId kHiddenSpell      = 914;  // [Hidden] 2E, revealed facedown
constexpr CardDefId kHiddenUnit       = 915;  // [Hidden] 2E UNIT, revealed facedown

// Real cards the deck this branch exists for actually contains.
constexpr CardDefId kSandsweptTomb = 792;
constexpr CardDefId kStarCrossed   = 690;  // [Reaction] 3E + 1 [Chaos], pair-pick
constexpr CardDefId kRengarPouncing = 348; // unit, 3E + 1 [Fury], reaction-to-attack
constexpr CardDefId kNidaleeCatForm = 676; // unit, 3E + 1 [Body], [Ambush]
constexpr CardDefId kClothArmor     = 387; // gear, 1E [Mind], [Quick-Draw]

CardDef makeSpellDef(CardDefId id, const char* name, int energy) {
    CardDef d;
    d.id = id;
    d.name = name;
    d.card_type = CardType::Spell;
    d.domains = {Domain::Fury};
    d.energy_cost = energy;
    return d;
}

/// Free [Action] spell with no effect. Its only job is to put an item on the
/// chain so the game enters the Closed State with a real FEPR loop running.
class OpenerSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kOpener, "Closed State Opener", 0);
        d.keywords.set(Keyword::Action);
        return d;
    }();
};

class HandReactionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kHandReaction, "Hand Reaction Test Spell", 1);
        d.keywords.set(Keyword::Reaction);
        return d;
    }();
};

class FlowReactionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kFlowReaction, "Flow Reaction Test Spell", 1);
        d.keywords.set(Keyword::Reaction);
        d.keywords.set(Keyword::Flow);
        d.flow_energy = 2;
        d.flow_power = 1;
        d.flow_any_domain = true;
        return d;
    }();
};

/// Printed 3E and NO printed [Flow]: the only flow cost it can ever have is a
/// granted one, so the cost actually charged tells the two apart.
class GrantedFlowReactionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kGrantedReaction, "Granted Flow Test Spell", 3);
        d.keywords.set(Keyword::Reaction);
        return d;
    }();
};

class HiddenTestSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kHiddenSpell, "Hidden Test Spell", 2);
        d.keywords.set(Keyword::Hidden);
        return d;
    }();
};

/// A PERMANENT hidden facedown. Deliberately trigger-free and target-free:
/// the regression it guards is the reveal mechanism itself (facedown-zone
/// removal, the is_hidden / hidden_at clear, zero cost, the play source and
/// PlayedFromFacedownEvent), not any one card's text.
class HiddenTestUnit : public UnitCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kHiddenUnit;
        d.name = "Hidden Test Unit";
        d.card_type = CardType::Unit;
        d.domains = {Domain::Fury};
        d.energy_cost = 2;
        d.might = 2;
        d.keywords.set(Keyword::Hidden);
        return d;
    }();
};

/// One agent for both jobs a routed closed-state play needs:
///   • at a priority query, take the first intent matching `want` (once),
///     then pass priority forever after;
///   • at a UNIT target prompt (every option a single-object MakeChoice whose
///     object is a live unit), record the published option list and take the
///     first entry. Mirrors test_sandswept_tomb.cpp's TargetPromptRecorder,
///     including the unit filter — the cost-payment cursor publishes
///     single-object MakeChoice sets too, but of RUNES.
class ClosedStateAgent : public AgentInterface {
public:
    std::function<bool(const Intent&)> want;
    bool taken = false;
    int call_count = 0;
    std::vector<std::vector<GameObjectId>> unit_prompts;

    Intent selectAction(const GameState& s,
                        const std::vector<Intent>& legal) override {
        ++call_count;
        if (legal.empty()) return Intent{};

        bool is_target_prompt = true;
        std::vector<GameObjectId> objs;
        for (const auto& i : legal) {
            if (i.type != IntentType::MakeChoice || i.chosen_objects.size() != 1) {
                is_target_prompt = false;
                break;
            }
            auto id = i.chosen_objects.front();
            if (!s.objectExists(id) || !s.getObject(id).isUnit()) {
                is_target_prompt = false;
                break;
            }
            objs.push_back(id);
        }
        if (is_target_prompt) unit_prompts.push_back(objs);

        if (!taken && want) {
            for (const auto& i : legal) {
                if (!want(i)) continue;
                taken = true;
                return i;
            }
        }
        for (const auto& i : legal)
            if (i.type == IntentType::PassPriority) return i;
        return legal.front();
    }
};

}  // namespace

// ─── Fixture ───────────────────────────────────────────────────────────────

class ClosedStatePlaysTest : public CardTestFixture {
protected:
    void SetUp() override {
        CardTestFixture::SetUp();
        card_registry.registerCard(kOpener, std::make_unique<OpenerSpell>());
        card_registry.registerCard(kHandReaction,
                                    std::make_unique<HandReactionSpell>());
        card_registry.registerCard(kFlowReaction,
                                    std::make_unique<FlowReactionSpell>());
        card_registry.registerCard(kGrantedReaction,
                                    std::make_unique<GrantedFlowReactionSpell>());
        card_registry.registerCard(kHiddenSpell,
                                    std::make_unique<HiddenTestSpell>());
        card_registry.registerCard(kHiddenUnit,
                                    std::make_unique<HiddenTestUnit>());
        // executePlaySpell reads card_db_ (printed energy_cost, [Repeat]
        // ability_text) and CardDB::get throws on an unknown id.
        card_db.buildFromClasses(card_registry);
    }

    /// Main Phase / Neutral Open, P1 to act, two battlefields in the ENGINE's
    /// state (the base fixture's `state` is a different object).
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
        // Permanents added this way go on to be played for real, so they need
        // their printed might; spells and the test spells all print 0, so this
        // is a no-op for every caller that predates the permanent tests.
        obj.base_might = def.might;
        obj.current_might = def.might;
        obj.zone = zone;
        if (zone == ZoneType::Trash) s.player(owner).trash.push_back(id);
        else if (zone == ZoneType::Hand) s.player(owner).hand.push_back(id);
        return id;
    }

    GameObjectId addUnitIn(GameState& s, PlayerId owner, int at_bf,
                            const char* name) {
        auto id = s.createObject();
        auto& u = s.getObject(id);
        u.owner = owner;
        u.controller = owner;
        u.card_type = CardType::Unit;
        u.name = name;
        u.base_might = 2;
        u.current_might = 2;
        u.zone = ZoneType::BattlefieldZone;
        u.location = BattlefieldLocation{static_cast<BattlefieldId>(at_bf)};
        return id;
    }

    /// Put the Sandswept Tomb card object on battlefield `bf` and run the
    /// aura pass, so `friendly_spell_power_discount` is live.
    void placeTomb(GameEngine& engine, BattlefieldId bf) {
        auto& s = engine.mutableState();
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        const auto& def = card_db.get(kSandsweptTomb);
        obj.card_def_id = kSandsweptTomb;
        obj.name = def.name;
        obj.card_type = CardType::Battlefield;
        obj.domains = def.domains;
        s.battlefields[bf].card_object_id = id;
        engine.testHook_cleanup();
    }

    /// Play the free opener from P1's hand. executePlaySpell puts it on the
    /// chain and runs the FEPR loop, so everything after this call happens
    /// in the Closed State with `agent1` holding priority first.
    void openTheChain(GameEngine& engine, GameObjectId opener) {
        Intent play;
        play.type = IntentType::PlayActionCard;
        play.player = P1;
        play.card = opener;
        play.play_source = Intent::PlaySource::Hand;
        engine.testHook_executeIntent(play);
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

    static int countIn(const std::vector<GameObjectId>& v, GameObjectId id) {
        return static_cast<int>(std::count(v.begin(), v.end(), id));
    }

    /// The base fixture's inHand() reads the fixture-owned `state`; these
    /// tests build into the ENGINE's state.
    static bool inHandIn(const GameState& s, PlayerId p, GameObjectId id) {
        const auto& h = s.player(p).hand;
        return std::find(h.begin(), h.end(), id) != h.end();
    }
};

// ─── (a) Flow play out of the trash, in the Closed State ───────────────────

TEST_F(ClosedStatePlaysTest, ClosedFlowPlayPaysFlowCostAndBanishesWithoutDuplicatingTrash) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto spell  = addToZoneIn(s, P1, kFlowReaction, ZoneType::Trash);
    // 4 ready Fury runes: the flow cost ([E2][P1] in any domain) is payable
    // with one to spare; the printed 1E would leave a different footprint.
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == spell &&
               i.flow_source == Intent::FlowSource::Printed;
    };

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: the closed-state generator must offer the trash [Flow] "
           "[Reaction] play for the agent to take.";

    // ── The FLOW cost, not the printed one ──
    EXPECT_EQ(countExhausted(s, P1), 2)
        << "CR 829.1.c.1 — Flow REPLACES the base cost. Exactly the flow "
           "cost's [E2] may be exhausted; the printed 1E must not be charged "
           "instead of, or on top of, it.";
    EXPECT_EQ(countReady(s, P1), 1);
    EXPECT_EQ(s.player(P1).rune_deck.size(), 1u)
        << "The flow cost's [P1] recycles exactly one rune.";

    // ── Removed from the trash exactly once, ends banished ──
    EXPECT_EQ(countIn(s.player(P1).trash, spell), 0)
        << "The spell was played OUT of the trash — it may not be in the "
           "trash afterwards. A stale copy here means the play never removed "
           "it and the post-resolution disposal pushed a duplicate.";
    EXPECT_EQ(s.getObject(spell).zone, ZoneType::Banishment)
        << "CR 829.1.b.1 — a spell played for its Flow cost is BANISHED as it "
           "leaves the chain.";
    EXPECT_EQ(countIn(s.player(P1).banishment, spell), 1);

    ASSERT_EQ(played.size(), 2u) << "the opener and the reaction";
    const auto& e = played[1];
    EXPECT_EQ(e.object, spell);
    EXPECT_EQ(e.play_source, Intent::PlaySource::Trash)
        << "A flow play comes out of the trash — CardPlayedEvent must say so.";
    EXPECT_EQ(e.energy_spent, 2)
        << "CardPlayedEvent must report the flow energy actually paid, not "
           "the printed 1.";
}

// ─── (b) Granted Flow (the Kennen 789 grant shape), in the Closed State ────

TEST_F(ClosedStatePlaysTest, ClosedGrantedFlowPlayPaysTheGrantAndConsumesIt) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto spell  = addToZoneIn(s, P1, kGrantedReaction, ZoneType::Trash);

    // The grant Kennen, Storm of Shuriken (789) writes: "until end of turn, a
    // spell in your trash gains [Flow] <cost>". Written straight onto the
    // object here — Kennen's own resolution is not what's under test, and a
    // cost that differs from the printed 3E is what tells the two apart.
    GameObject::GrantedFlow gf;
    gf.energy = 1;
    gf.power = 0;
    gf.power_domain = Domain::Fury;
    gf.any_domain = false;
    gf.valid_on_turn = s.turn.turn_number;
    s.getObject(spell).granted_flow = gf;

    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Fury);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == spell &&
               i.flow_source == Intent::FlowSource::Granted;
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: the granted flow play must be offered in the Closed State.";

    EXPECT_EQ(countExhausted(s, P1), 1)
        << "The GRANTED flow cost is [E1] — the printed 3E must not be "
           "charged in its place.";
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "The granted cost has no power component; nothing may be recycled.";
    EXPECT_FALSE(s.getObject(spell).granted_flow.has_value())
        << "A granted Flow is consumed by the play it paid for.";
    EXPECT_EQ(countIn(s.player(P1).trash, spell), 0);
    EXPECT_EQ(s.getObject(spell).zone, ZoneType::Banishment)
        << "A granted Flow play is still a Flow play — CR 829.1.b.1 banishes "
           "it as it leaves the chain.";
}

// ─── (c) Star-Crossed as a restricted Sandswept Tomb offer, closed ─────────

TEST_F(ClosedStatePlaysTest, ClosedRestrictedStarCrossedPaysTheDiscountAndNarrowsThePicker) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    auto tomb_friendly = addUnitIn(s, P1, /*at_bf=*/0, "Friendly At Tomb");
    auto far_friendly  = addUnitIn(s, P1, /*at_bf=*/1, "Friendly Elsewhere");
    auto enemy         = addUnitIn(s, P2, /*at_bf=*/1, "Enemy");

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto spell  = addToZoneIn(s, P1, kStarCrossed, ZoneType::Hand);

    // 3 ORDER runes: the [3] energy is covered, the [1] Chaos power is not —
    // so only the discounted, restricted play is offered at all.
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Order);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == spell &&
               i.target_battlefield_restriction.has_value();
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: the restricted Tomb offer must reach the Closed State — "
           "Star-Crossed is a [Reaction] and the deck this branch exists for "
           "plays it off the Tomb.";

    EXPECT_EQ(countExhausted(s, P1), 3);
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "Sandswept Tomb's [A] discount is charged: nothing is recycled for "
           "the printed [1] power. A recycled rune means the closed-state "
           "path paid the undiscounted cost.";

    ASSERT_EQ(agent1.unit_prompts.size(), 2u)
        << "Star-Crossed publishes two unit picks: the friendly, then the "
           "enemy.";
    EXPECT_EQ(agent1.unit_prompts[0], std::vector<GameObjectId>{tomb_friendly})
        << "The commitment the discount was paid for must reach the resolve-"
           "time picker: the A list is narrowed to friendly units at the Tomb.";
    EXPECT_EQ(agent1.unit_prompts[1], std::vector<GameObjectId>{enemy});

    EXPECT_EQ(s.getObject(tomb_friendly).zone, ZoneType::Hand);
    EXPECT_EQ(s.getObject(enemy).zone, ZoneType::Hand);
    EXPECT_TRUE(s.getObject(far_friendly).isAtBattlefield());
}

// ─── (d) Regression: the plain hand [Reaction] play is unchanged ───────────

TEST_F(ClosedStatePlaysTest, ClosedHandReactionPlayStillPaysAndTrashes) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto spell  = addToZoneIn(s, P1, kHandReaction, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Fury);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == spell;
    };

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken);
    EXPECT_FALSE(inHandIn(s, P1, spell)) << "the reaction left the hand";
    EXPECT_EQ(countExhausted(s, P1), 1)
        << "the printed [1] is charged, exactly as before";
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u);
    EXPECT_EQ(s.getObject(spell).zone, ZoneType::Trash)
        << "CR 359.3 — a spell that was NOT played for a Flow cost trashes "
           "when it leaves the chain.";
    EXPECT_EQ(countIn(s.player(P1).trash, spell), 1);

    ASSERT_EQ(played.size(), 2u);
    EXPECT_EQ(played[1].object, spell);
    EXPECT_EQ(played[1].play_source, Intent::PlaySource::Hand);
    EXPECT_EQ(played[1].energy_spent, 1);
}

// ─── (e) A live facedown reveal emits PlayedFromFacedownEvent ──────────────

TEST_F(ClosedStatePlaysTest, LiveHiddenRevealEmitsPlayedFromFacedownEvent) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);

    // A card hidden on an EARLIER turn — CR 811.1.d: it gains [Reaction] the
    // turn after it was hidden, which is what makes it a legal closed-state
    // play at all.
    auto spell = addToZoneIn(s, P1, kHiddenSpell, ZoneType::Hand);
    {
        auto& ph = s.player(P1).hand;
        ph.erase(std::remove(ph.begin(), ph.end(), spell), ph.end());
        auto& c = s.getObject(spell);
        c.zone = ZoneType::FacedownZone;
        c.location = std::nullopt;
        c.is_hidden = true;
        c.hidden_at = 0;
        c.hidden_on_turn = s.turn.turn_number - 1;
        s.battlefields[0].facedown.push_back(spell);
    }
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Fury);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == spell;
    };

    std::vector<PlayedFromFacedownEvent> facedown;
    auto fd = events.on_played_from_facedown.connect(
        [&](const PlayedFromFacedownEvent& e) { facedown.push_back(e); });
    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: a card hidden last turn must be offered as a closed-state "
           "play.";

    ASSERT_EQ(facedown.size(), 1u)
        << "CR 811 — revealing and playing a facedown card is 'playing a card "
           "from face down'. Katarina, Reckless (585) triggers on it, so the "
           "LIVE reveal path must emit PlayedFromFacedownEvent exactly once.";
    EXPECT_EQ(facedown[0].card, spell);
    EXPECT_EQ(facedown[0].player, P1);

    EXPECT_TRUE(s.battlefields[0].facedown.empty())
        << "the revealed card leaves the facedown zone";
    EXPECT_FALSE(s.getObject(spell).is_hidden);

    ASSERT_EQ(played.size(), 2u);
    EXPECT_EQ(played[1].object, spell);
    EXPECT_EQ(played[1].play_source, Intent::PlaySource::Hidden)
        << "the play source is derived from the facedown status the card had "
           "when it was played";
    EXPECT_EQ(played[1].energy_spent, 0)
        << "CR 811 — a card played from face down is played IGNORING its base "
           "cost, so nothing was spent on it.";
    EXPECT_EQ(countExhausted(s, P1), 0)
        << "no rune may be exhausted for a facedown reveal";
    EXPECT_EQ(countIn(s.player(P1).trash, spell), 1)
        << "the revealed spell trashes as it leaves the chain";
}

// ─── (f) NON-SPELL closed-state reactions: permanents land on the board ────
//
// The spell half of the closed-state branch was routed through
// GameEngine::executePlaySpell (above). The non-spell half kept a hand-rolled
// play that ended in `addSpell(...)` — which sets `is_spell = true`. A
// [Quick-Draw] gear, an [Ambush] unit, a Rengar-style reaction-to-attack unit
// and a facedown PERMANENT reveal therefore all: paid their full cost, were
// finalized as if they were spells (so CR 337.1.c's "permanents resolve
// immediately on finalize" never applied to them), resolved through
// Card::onResolve (a no-op on a UnitCard / GearCard), and were then disposed
// by stepResolve's `if (resolved.is_spell)` arm straight into the TRASH. The
// card was paid for and never reached the board.
//
// These four drive the real FEPR loop and assert where the permanent actually
// lands. The fix routes this half through GameEngine::executePlayCard, the
// same executor every other permanent play uses.

TEST_F(ClosedStatePlaysTest, ClosedPouncingUnitLandsAtTheAttackedBattlefield) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();
    // Rengar, Pouncing's permission is "a battlefield you're attacking", so
    // the closed state we open is the one inside a combat.
    s.turn.ns_state = NeutralShowdownState::Showdown;
    s.battlefields[0].combat_in_progress = true;
    s.battlefields[0].attacker = P1;
    s.battlefields[0].defender = P2;
    addUnitIn(s, P1, 0, "Attacker");
    addUnitIn(s, P2, 0, "Defender");

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto rengar = addToZoneIn(s, P1, kRengarPouncing, ZoneType::Hand);
    for (int i = 0; i < 6; ++i) addReadyRune(s, P1, Domain::Fury);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == rengar;
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: the closed-state generator must offer the Pouncing play";
    EXPECT_FALSE(inHandIn(s, P1, rengar)) << "the play leaves hand";
    EXPECT_EQ(countIn(s.player(P1).trash, rengar), 0)
        << "a PERMANENT reaction must not be disposed as if it were a spell";

    const auto& obj = s.getObject(rengar);
    ASSERT_TRUE(obj.location.has_value())
        << "the unit must be somewhere on the board";
    ASSERT_TRUE(std::holds_alternative<BattlefieldLocation>(*obj.location));
    EXPECT_EQ(std::get<BattlefieldLocation>(*obj.location).id,
              s.battlefields[0].id)
        << "it is played TO the battlefield the intent named";
    const auto attackers =
        s.unitsAt(BattlefieldLocation{s.battlefields[0].id}, P1);
    EXPECT_NE(std::find(attackers.begin(), attackers.end(), rengar),
              attackers.end())
        << "and is a combatant on the attacking side";

    EXPECT_EQ(countReady(s, P1), 3)
        << "3E exhausted; the [Fury] power recycles one already-exhausted rune";
    EXPECT_TRUE(s.chain.items.empty());
}

TEST_F(ClosedStatePlaysTest, ClosedAmbushUnitLandsReadyAtItsBattlefield) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();
    // [Ambush] needs a battlefield where you already have units — give P1 one
    // at BF1 only, so the single offer names BF1 and the assertion is sharp.
    addUnitIn(s, P1, 1, "Anchor");

    auto opener  = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto nidalee = addToZoneIn(s, P1, kNidaleeCatForm, ZoneType::Hand);
    for (int i = 0; i < 6; ++i) addReadyRune(s, P1, Domain::Body);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == nidalee;
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: the closed-state generator must offer the Ambush play";
    EXPECT_EQ(countIn(s.player(P1).trash, nidalee), 0)
        << "an [Ambush] unit must not be disposed as if it were a spell";

    const auto& obj = s.getObject(nidalee);
    ASSERT_TRUE(obj.location.has_value());
    ASSERT_TRUE(std::holds_alternative<BattlefieldLocation>(*obj.location));
    EXPECT_EQ(std::get<BattlefieldLocation>(*obj.location).id,
              s.battlefields[1].id);
    EXPECT_FALSE(obj.is_exhausted)
        << "an Ambush unit entering a battlefield enters ready";
}

TEST_F(ClosedStatePlaysTest, ClosedQuickDrawGearAttachesToTheNamedUnit) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();
    auto bearer = addUnitIn(s, P1, 0, "Bearer");

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto gear   = addToZoneIn(s, P1, kClothArmor, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Mind);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == gear &&
               !i.targets.empty() && i.targets[0] == bearer;
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: the closed-state generator must offer the Quick-Draw play";
    EXPECT_EQ(countIn(s.player(P1).trash, gear), 0)
        << "gear must not be disposed as if it were a spell";

    const auto& g = s.getObject(gear);
    ASSERT_TRUE(g.attached_to.has_value())
        << "[Quick-Draw] attaches the gear to the unit the play named (CR 819)";
    EXPECT_EQ(*g.attached_to, bearer);
    EXPECT_EQ(s.player(P1).gears_played_this_turn, 1)
        << "the gear play path owns the per-turn gear counter";
}

TEST_F(ClosedStatePlaysTest, ClosedFacedownPermanentRevealEntersPlayForFree) {
    GameEngine engine(card_db, events, card_registry);
    ClosedStateAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToZoneIn(s, P1, kOpener, ZoneType::Hand);
    auto unit   = addToZoneIn(s, P1, kHiddenUnit, ZoneType::Hand);
    {   // hidden LAST turn at BF0, so it has [Reaction] now
        auto& ps = s.player(P1);
        ps.hand.erase(std::remove(ps.hand.begin(), ps.hand.end(), unit),
                      ps.hand.end());
        auto& c = s.getObject(unit);
        c.zone = ZoneType::FacedownZone;
        c.location = std::nullopt;
        c.is_hidden = true;
        c.hidden_at = 0;
        c.hidden_on_turn = s.turn.turn_number - 1;
        s.battlefields[0].facedown.push_back(unit);
    }
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Fury);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == unit;
    };

    std::vector<PlayedFromFacedownEvent> facedown;
    auto fd = events.on_played_from_facedown.connect(
        [&](const PlayedFromFacedownEvent& e) { facedown.push_back(e); });
    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: a permanent hidden last turn must be offered as a "
           "closed-state play";

    // Reveal mechanics — the half that already worked, guarded.
    EXPECT_TRUE(s.battlefields[0].facedown.empty())
        << "the revealed permanent leaves the facedown zone";
    const auto& obj = s.getObject(unit);
    EXPECT_FALSE(obj.is_hidden);
    EXPECT_EQ(obj.hidden_at, kInvalidId);
    EXPECT_EQ(countExhausted(s, P1), 0)
        << "CR 811 — played IGNORING its base cost, so no rune is spent";

    ASSERT_EQ(played.size(), 2u);
    EXPECT_EQ(played[1].object, unit);
    EXPECT_EQ(played[1].play_source, Intent::PlaySource::Hidden);
    EXPECT_EQ(played[1].energy_spent, 0);

    // "When you play a card from face down" is card-type agnostic (Katarina,
    // Reckless 585), so a PERMANENT reveal must fire it too.
    ASSERT_EQ(facedown.size(), 1u)
        << "the permanent reveal must emit PlayedFromFacedownEvent exactly once";
    EXPECT_EQ(facedown[0].card, unit);
    EXPECT_EQ(facedown[0].player, P1);

    // ... and it must reach the board, at the battlefield it was hidden at.
    EXPECT_EQ(countIn(s.player(P1).trash, unit), 0)
        << "a revealed PERMANENT must not be disposed as if it were a spell";
    ASSERT_TRUE(obj.location.has_value());
    ASSERT_TRUE(std::holds_alternative<BattlefieldLocation>(*obj.location));
    EXPECT_EQ(std::get<BattlefieldLocation>(*obj.location).id,
              s.battlefields[0].id)
        << "CR 811 — the card was face down AT that battlefield; it is "
           "revealed and played there";
}
