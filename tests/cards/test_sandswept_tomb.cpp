/// @file test_sandswept_tomb.cpp
/// Sandswept Tomb (792, VEN) + the rune-discount mechanism behind it —
/// Kennen/Tyler deck spec §4 and addenda #5 / #9.
///
/// Card text: "Each spell that chooses one or more units here that are
/// friendly to it costs [A] less."  A POWER (rune) discount of 1 in ANY
/// domain. "Friendly to it" is relative to the SPELL'S CONTROLLER and the
/// flag lives on the battlefield, so BOTH players benefit from the Tomb.
///
/// Because most unit-choosing spells pick their target at RESOLVE time
/// (`needsPlayTimeTarget()`), the offer cannot know whether the discount's
/// condition will be met. The ratified mechanism (spec §4, addendum #5) is a
/// second, RESTRICTED intent: identical to the normal one but carrying
/// `Intent::target_battlefield_restriction`. Taking it commits the play to
/// choosing at the Tomb (Card::pickTarget filters the resolve-time legal list
/// to that battlefield) and prices it with the discount.
///
/// Tests here, numbered per the implementation plan:
///   #22 — the aura sets `friendly_spell_power_discount` on the Tomb's OWN
///         battlefield and on no other.
///   #23 — Ride the Wind (173: 2E + 1 Chaos power, resolve-time target) with
///         a friendly unit at the Tomb is offered TWICE (normal + restricted),
///         and the restricted intent is affordable with one fewer matching
///         rune than the normal one needs.
///   #24 — executing the restricted intent recycles one fewer rune for power,
///         stamps the restriction on the chain item, and the resolve-time
///         picker offers ONLY units at the Tomb.
///   #25 — no friendly unit at the Tomb → offered exactly once (no restricted
///         intent, no discount).
///   flow — a [Flow] spell with power in the trash + a Tomb yields a
///         restricted FLOW intent that pays one less power.
///   opp  — the OPPONENT of the Tomb's controller gets the same discount for
///         THEIR friendly unit there ("friendly to it" is per-spell).

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

constexpr CardDefId kSandsweptTomb = 792;
constexpr CardDefId kRideTheWind   = 173;   // 2E + 1 [Chaos] power, resolve-time target

}  // namespace

class SandsweptTombTest : public CardTestFixture {
protected:
    /// Main Phase / Neutral Open, P1 to act, two battlefields in the ENGINE's
    /// state (the base fixture's `state` is a different object the engine
    /// never sees).
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

    /// Put the Tomb card object on battlefield `bf` and run the aura pass the
    /// engine runs during cleanup, so the flag is live.
    GameObjectId placeTomb(GameEngine& engine, BattlefieldId bf) {
        auto& s = engine.mutableState();
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        const auto& def = card_db.get(kSandsweptTomb);
        obj.card_def_id = kSandsweptTomb;
        obj.name = def.name;
        obj.card_type = CardType::Battlefield;
        obj.domains = def.domains;
        s.battlefields[bf].card_object_id = id;
        engine.testHook_cleanup();   // runs recalculateAuras (step 1c: BF cards)
        return id;
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

    GameObjectId addUnitIn(GameState& s, PlayerId owner, int at_bf,
                            const char* name = "Test Unit") {
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_type = CardType::Unit;
        obj.name = name;
        obj.base_might = 2;
        obj.current_might = 2;
        if (at_bf >= 0) {
            obj.zone = ZoneType::BattlefieldZone;
            obj.location = BattlefieldLocation{static_cast<BattlefieldId>(at_bf)};
        } else {
            obj.zone = ZoneType::Base;
            obj.location = BaseLocation{owner};
        }
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
        obj.zone = zone;
        if (zone == ZoneType::Trash) s.player(owner).trash.push_back(id);
        else if (zone == ZoneType::Hand) s.player(owner).hand.push_back(id);
        return id;
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

    static int countReady(const GameState& s, PlayerId owner) {
        int n = 0;
        for (const auto& [id, obj] : s.objects) {
            if (obj.controller != owner || !obj.isRune()) continue;
            if (obj.zone != ZoneType::Base) continue;
            if (!obj.is_exhausted) ++n;
        }
        return n;
    }

    static int countExhausted(const GameState& s, PlayerId owner) {
        int n = 0;
        for (const auto& [id, obj] : s.objects) {
            if (obj.controller != owner || !obj.isRune()) continue;
            if (obj.zone != ZoneType::Base) continue;
            if (obj.is_exhausted) ++n;
        }
        return n;
    }
};

// ─── Test #22: the aura ────────────────────────────────────────────────────

TEST_F(SandsweptTombTest, AuraSetsDiscountFlagOnItsOwnBattlefieldOnly) {
    auto tomb = addUnit(P1, kSandsweptTomb, 0, /*at_bf=*/-1);
    state.getObject(tomb).card_type = CardType::Battlefield;
    state.battlefields[0].card_object_id = tomb;

    EXPECT_EQ(state.battlefields[0].friendly_spell_power_discount, 0)
        << "sanity: the flag starts clear";

    ASSERT_NE(card_registry.get(kSandsweptTomb), nullptr)
        << "Sandswept Tomb (792) must be registered.";
    card_registry.get(kSandsweptTomb)->applyPassiveAura(state, PlayerId::None);

    EXPECT_EQ(state.battlefields[0].friendly_spell_power_discount, 1)
        << "The Tomb discounts spells that choose friendly units HERE — the "
           "flag belongs on the battlefield the Tomb itself is.";
    EXPECT_EQ(state.battlefields[1].friendly_spell_power_discount, 0)
        << "No other battlefield may pick up the discount.";
}

// ─── Test #23: the offer ───────────────────────────────────────────────────

TEST_F(SandsweptTombTest, RideTheWindOfferedTwiceWithFriendlyUnitAtTheTomb) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    ASSERT_EQ(s.battlefields[0].friendly_spell_power_discount, 1)
        << "sanity: the Tomb's aura is live";

    addUnitIn(s, P1, /*at_bf=*/0, "Tomb Unit");
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);

    // 3 Chaos runes: the FULL printed cost (2E + 1 [Chaos]) is payable —
    // exhaust 2 for energy, recycle 1 of them for the power.
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Chaos);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 2u)
        << "With a friendly unit at the Tomb, a resolve-time-target spell "
           "must be offered twice: the ordinary play (target free, full "
           "price) and the Tomb-restricted play (target committed to the "
           "Tomb, [A] cheaper). Spec §4 / addendum #5.";

    int plain = 0, restricted = 0;
    for (const auto& o : offers) {
        if (o.target_battlefield_restriction.has_value()) {
            ++restricted;
            EXPECT_EQ(*o.target_battlefield_restriction, 0u)
                << "The restriction must name the Tomb's battlefield.";
        } else {
            ++plain;
        }
        EXPECT_TRUE(o.targets.empty())
            << "Ride the Wind picks at resolve time — neither offer carries "
               "a play-time target.";
        EXPECT_EQ(o.flow_source, Intent::FlowSource::None);
    }
    EXPECT_EQ(plain, 1);
    EXPECT_EQ(restricted, 1);
}

TEST_F(SandsweptTombTest, RestrictedIntentIsTheOnlyOfferWhenOnlyTheDiscountedCostIsPayable) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    addUnitIn(s, P1, /*at_bf=*/0, "Tomb Unit");
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);

    // 2 ORDER runes: enough energy for the [2], but Ride the Wind is a Chaos
    // card and there is no Chaos rune to recycle for its [1] power. The full
    // cost is therefore NOT payable; the Tomb's [A] discount removes the
    // power component entirely and makes the restricted play payable.
    addReadyRune(s, P1, Domain::Order);
    addReadyRune(s, P1, Domain::Order);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "The full cost is unaffordable (no Chaos rune for the [1] power), "
           "so only the Tomb-restricted, discounted play may be offered — and "
           "it MUST be offered: its affordability is checked against the "
           "discounted power, not the printed one.";
    ASSERT_TRUE(offers[0].target_battlefield_restriction.has_value())
        << "The single surviving offer is the restricted one.";
    EXPECT_EQ(*offers[0].target_battlefield_restriction, 0u);
}

// ─── Test #24: execution ───────────────────────────────────────────────────

namespace {

/// Records every MakeChoice prompt whose options are all UNITS — i.e. the
/// resolve-time target picks, as opposed to the cost-payment rune picks that
/// share the MakeChoice shape. Always takes the first option.
class TargetPromptRecorder : public AgentInterface {
public:
    std::vector<std::vector<GameObjectId>> unit_prompts;
    /// Objects to pick when they appear among the options (used to drive a
    /// pair pick down a specific branch). Anything else takes option 0.
    std::vector<GameObjectId> prefer;

    Intent selectAction(const GameState& s,
                        const std::vector<Intent>& legal) override {
        if (!legal.empty() && legal.front().type == IntentType::MakeChoice) {
            std::vector<GameObjectId> units;
            bool all_units = true;
            for (const auto& i : legal) {
                if (i.chosen_objects.empty()) { all_units = false; break; }
                auto id = i.chosen_objects.front();
                if (!s.objectExists(id) || !s.getObject(id).isUnit()) {
                    all_units = false;
                    break;
                }
                units.push_back(id);
            }
            if (all_units && !units.empty()) unit_prompts.push_back(units);
        }
        for (const auto& i : legal) {
            if (i.chosen_objects.empty()) continue;
            if (std::find(prefer.begin(), prefer.end(),
                          i.chosen_objects.front()) != prefer.end())
                return i;
        }
        return legal.empty() ? Intent{} : legal.front();
    }
};

}  // namespace

TEST_F(SandsweptTombTest, RestrictedPlayRecyclesOneFewerRuneAndPicksOnlyAtTheTomb) {
    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    auto tomb_unit = addUnitIn(s, P1, /*at_bf=*/0, "Tomb Unit");
    auto far_unit  = addUnitIn(s, P1, /*at_bf=*/1, "Far Unit");
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Chaos);

    std::optional<BattlefieldId> chain_restriction;
    bool saw_item = false;
    auto fin = events.on_chain_item_finalized.connect(
        [&](const ChainItemFinalizedEvent& e) {
            if (e.source != spell) return;
            for (const auto& it : s.chain.items) {
                if (it.id != e.item) continue;
                saw_item = true;
                chain_restriction = it.target_battlefield_restriction;
            }
        });

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 2u) << "sanity: normal + restricted";
    Intent restricted;
    bool found = false;
    for (const auto& o : offers) {
        if (o.target_battlefield_restriction.has_value()) { restricted = o; found = true; }
    }
    ASSERT_TRUE(found);

    engine.testHook_executeIntent(restricted);

    // ── Cost: the [1] power is discounted away, so NOTHING is recycled ──
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "The Tomb pays the spell's [1] power for it — no rune may be "
           "recycled. A recycled rune here means the discount never reached "
           "the payment path.";
    EXPECT_EQ(countExhausted(s, P1), 2)
        << "The [2] energy still costs two exhausted runes; only the power "
           "component is discounted.";
    EXPECT_EQ(countReady(s, P1), 1)
        << "3 runes - 2 exhausted for energy = 1 still ready, none recycled.";

    // ── The chain item carries the commitment ──
    ASSERT_TRUE(saw_item) << "the play must put the spell on the chain";
    ASSERT_TRUE(chain_restriction.has_value())
        << "executePlaySpell must stamp the restriction onto the chain item — "
           "it is what the resolve-time picker reads.";
    EXPECT_EQ(*chain_restriction, 0u);

    // ── The picker only offers units at the Tomb ──
    ASSERT_EQ(agent1.unit_prompts.size(), 1u)
        << "Ride the Wind must publish exactly one resolve-time target pick.";
    EXPECT_EQ(agent1.unit_prompts[0], std::vector<GameObjectId>{tomb_unit})
        << "The restricted play committed to choosing at the Tomb, so the "
           "resolve-time legal list must be filtered to units there — the "
           "unit at the other battlefield may not be offered.";
    EXPECT_TRUE(s.getObject(tomb_unit).isAtBase())
        << "Ride the Wind still does its job: the chosen unit is moved to base.";
    EXPECT_TRUE(s.getObject(far_unit).isAtBattlefield())
        << "The unit that could not be chosen is untouched.";
}

TEST_F(SandsweptTombTest, UnrestrictedPlayStillPaysThePowerAndSeesEveryTarget) {
    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    auto tomb_unit = addUnitIn(s, P1, /*at_bf=*/0, "Tomb Unit");
    auto far_unit  = addUnitIn(s, P1, /*at_bf=*/1, "Far Unit");
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Chaos);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 2u);
    Intent plain;
    bool found = false;
    for (const auto& o : offers) {
        if (!o.target_battlefield_restriction.has_value()) { plain = o; found = true; }
    }
    ASSERT_TRUE(found);

    engine.testHook_executeIntent(plain);

    // The control for the test above: taking the UNRESTRICTED offer keeps the
    // full price and the full choice.
    EXPECT_EQ(s.player(P1).rune_deck.size(), 1u)
        << "Without the commitment there is no discount — the [1] power "
           "recycles exactly one Chaos rune.";
    ASSERT_EQ(agent1.unit_prompts.size(), 1u);
    EXPECT_EQ(agent1.unit_prompts[0].size(), 2u)
        << "An unrestricted play may choose either friendly unit.";
    EXPECT_NE(std::find(agent1.unit_prompts[0].begin(),
                        agent1.unit_prompts[0].end(), tomb_unit),
              agent1.unit_prompts[0].end());
    EXPECT_NE(std::find(agent1.unit_prompts[0].begin(),
                        agent1.unit_prompts[0].end(), far_unit),
              agent1.unit_prompts[0].end());
}

// ─── Test #25: no eligible unit at the Tomb ────────────────────────────────

TEST_F(SandsweptTombTest, OfferedOnceWhenNoFriendlyUnitIsAtTheTomb) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    // The only friendly unit is at the OTHER battlefield, and the unit AT the
    // Tomb belongs to the opponent — "friendly to it" means friendly to the
    // spell's controller, so neither earns P1 a discount.
    addUnitIn(s, P1, /*at_bf=*/1, "Far Unit");
    addUnitIn(s, P2, /*at_bf=*/0, "Enemy At Tomb");
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Chaos);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "With no legal friendly-unit target at the Tomb there is nothing "
           "to commit to, so no restricted intent may be emitted — the spell "
           "is offered exactly once, at full price.";
    EXPECT_FALSE(offers[0].target_battlefield_restriction.has_value());

    engine.testHook_executeIntent(offers[0]);
    EXPECT_EQ(s.player(P1).rune_deck.size(), 1u)
        << "and it pays its full [1] power: one Chaos rune recycled.";
}

TEST_F(SandsweptTombTest, NoRestrictedOfferWithoutATomb) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    // Same board as test #23, minus the Tomb.
    addUnitIn(s, P1, /*at_bf=*/0, "Unit At BF0");
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Chaos);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "No Tomb, no discount, no second offer — the mechanism must be "
           "invisible on an ordinary board.";
    EXPECT_FALSE(offers[0].target_battlefield_restriction.has_value());
}

// ─── The flow path: Flow REPLACES the base cost, so the discount comes off
//     the FLOW cost's power component ─────────────────────────────────────

namespace {

constexpr CardDefId kTombFlowSpell = 903;   // test-local, above the shipped registry

/// A [Flow] spell that chooses a friendly unit at RESOLVE time and whose FLOW
/// cost has a power component — the exact shape the Tomb's restricted offer
/// exists for. Printed cost is deliberately out of reach so only the flow
/// path can be exercised.
class TombFlowSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    TargetRequirements getTargetRequirements() const override {
        return TargetRequirements{.count = 1, .must_be_unit = true,
                                   .must_be_friendly = true};
    }
    bool needsPlayTimeTarget() const override { return true; }
    void onResolve(CardContext& ctx, const std::vector<GameObjectId>& targets) override {
        GameObjectId picked = targets.empty()
            ? pickTarget(ctx, "Tomb Flow Spell",
                          enumerateLegalTargets(ctx.state, ctx.controller))
            : targets[0];
        if (picked == kInvalidId || !ctx.state.objectExists(picked)) return;
        ctx.executor.readyObject(picked);
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kTombFlowSpell;
        d.name = "Tomb Flow Test Spell";
        d.card_type = CardType::Spell;
        d.domains = {Domain::Fury};
        d.energy_cost = 6;              // unaffordable from hand in these tests
        d.keywords.set(Keyword::Flow);
        d.keywords.set(Keyword::Action);
        d.flow_energy = 1;
        d.flow_power = 1;
        d.flow_any_domain = true;
        return d;
    }();
};

}  // namespace

class SandsweptTombFlowTest : public SandsweptTombTest {
protected:
    void SetUp() override {
        SandsweptTombTest::SetUp();
        card_registry.registerCard(kTombFlowSpell,
                                    std::make_unique<TombFlowSpell>());
        card_db.buildFromClasses(card_registry);
    }
};

TEST_F(SandsweptTombFlowTest, RestrictedFlowIntentIsOfferedAndPaysOneLessPower) {
    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    auto tomb_unit = addUnitIn(s, P1, /*at_bf=*/0, "Tomb Unit");
    s.getObject(tomb_unit).is_exhausted = true;      // so readyObject is visible
    addUnitIn(s, P1, /*at_bf=*/1, "Far Unit");
    auto spell = addToZoneIn(s, P1, kTombFlowSpell, ZoneType::Trash);

    // ONE ready rune. The flow cost is [1] energy + [1] power in any domain:
    // that single rune cannot both be exhausted for the energy and recycled
    // for the power, so the undiscounted flow play is unaffordable. The
    // Tomb's [A] removes the power component and makes it payable.
    addReadyRune(s, P1, Domain::Fury);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "Only the Tomb-restricted flow play is affordable — and it MUST be "
           "offered: the flow generator prices its offers against the FLOW "
           "cost's power, discount included.";
    ASSERT_TRUE(offers[0].target_battlefield_restriction.has_value());
    EXPECT_EQ(*offers[0].target_battlefield_restriction, 0u);
    EXPECT_EQ(offers[0].flow_source, Intent::FlowSource::Printed);
    EXPECT_EQ(offers[0].play_source, Intent::PlaySource::Trash);

    engine.testHook_executeIntent(offers[0]);

    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "The flow cost's [1] power is discounted away — nothing recycled.";
    EXPECT_EQ(countExhausted(s, P1), 1)
        << "Only the flow cost's [1] energy is charged.";
    ASSERT_EQ(agent1.unit_prompts.size(), 1u);
    EXPECT_EQ(agent1.unit_prompts[0], std::vector<GameObjectId>{tomb_unit})
        << "A restricted FLOW play commits to the Tomb exactly as a hand play "
           "does — the picker may only offer units there.";
    EXPECT_FALSE(s.getObject(tomb_unit).is_exhausted)
        << "sanity: the spell still resolved and did its work";
}

// ─── "Friendly to IT": the Tomb's controller has no monopoly on the discount ──

TEST_F(SandsweptTombTest, OpponentOfTheTombsControllerGetsTheDiscountToo) {
    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent2;
    FirstChoiceAgent agent1;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();
    s.turn.turn_player = P2;               // it is the OPPONENT's turn to act

    placeTomb(engine, 0);
    auto p2_unit = addUnitIn(s, P2, /*at_bf=*/0, "P2 Unit At Tomb");
    auto spell = addToZoneIn(s, P2, kRideTheWind, ZoneType::Hand);
    // 2 Order runes: energy only. Without the discount the [1] Chaos power is
    // unpayable, so the restricted offer is the only one that can appear.
    addReadyRune(s, P2, Domain::Order);
    addReadyRune(s, P2, Domain::Order);
    // P1 — the player who is NOT playing the spell — holds the battlefield.
    // Stamped after the aura pass so control is not recomputed away from it.
    s.battlefields[0].controller = P1;

    ASSERT_EQ(s.battlefields[0].friendly_spell_power_discount, 1)
        << "sanity: the flag is on the battlefield, not on its controller";

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "The Tomb says 'units here that are friendly to IT' — friendly to "
           "the SPELL, i.e. to its controller. The flag lives on the "
           "battlefield, so the player who does NOT control the Tomb gets the "
           "discount for their own unit standing there.";
    ASSERT_TRUE(offers[0].target_battlefield_restriction.has_value());
    EXPECT_EQ(offers[0].player, P2);

    engine.testHook_executeIntent(offers[0]);
    EXPECT_EQ(s.player(P2).rune_deck.size(), 0u)
        << "and the discount is actually charged: no rune recycled for power.";
    ASSERT_EQ(agent2.unit_prompts.size(), 1u);
    EXPECT_EQ(agent2.unit_prompts[0], std::vector<GameObjectId>{p2_unit});
}

// ─── Play-time targets: no restricted intent needed, the discount applies
//     directly to the offer that chooses the unit at the Tomb ─────────────

TEST_F(SandsweptTombTest, PlayTimeTargetOfferIsPricedWithTheDiscountItEarns) {
    // Last Stand (69): 3 energy + 1 [Calm] power, and it chooses its friendly
    // unit AT PLAY TIME (one intent per legal target). Per spec §4 such a
    // spell needs no restricted variant — the chosen target is already known,
    // so the discount is applied to that offer directly.
    constexpr CardDefId kLastStand = 69;

    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    auto tomb_unit = addUnitIn(s, P1, /*at_bf=*/0, "Tomb Unit");
    addUnitIn(s, P1, /*at_bf=*/1, "Far Unit");
    auto spell = addToZoneIn(s, P1, kLastStand, ZoneType::Hand);

    // 3 ORDER runes: the [3] energy is covered, the [1] Calm power is not.
    // Only the offer that chooses the unit standing at the Tomb is payable.
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Order);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "Each play-time-target offer must be priced with the discount THAT "
           "offer earns: choosing the unit at the Tomb costs [A] less and is "
           "affordable; choosing the unit elsewhere is not.";
    ASSERT_EQ(offers[0].targets.size(), 1u);
    EXPECT_EQ(offers[0].targets[0], tomb_unit);
    EXPECT_FALSE(offers[0].target_battlefield_restriction.has_value())
        << "A play-time target needs no commitment — it is already chosen.";

    engine.testHook_executeIntent(offers[0]);
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "and the payment path honours the same discount: nothing recycled.";
    EXPECT_EQ(countExhausted(s, P1), 3);
}

// ─── A restriction is a CLAIM, not a fact (review fix round 1, Important #1) ──

TEST_F(SandsweptTombTest, HandBuiltRestrictionWithNoEligibleUnitIsRejected) {
    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    // The only friendly unit is at the OTHER battlefield, so no restricted
    // offer is generated — but an agent, a replay or the OpenSpiel bridge can
    // still hand executePlaySpell an intent that claims one.
    addUnitIn(s, P1, /*at_bf=*/1, "Far Unit");
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Chaos);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u) << "sanity: no restricted offer is generated";
    Intent forged = offers[0];
    forged.target_battlefield_restriction = 0;   // the Tomb — but nothing to choose there

    std::vector<CardPlayedEvent> played;
    auto conn = events.on_card_played.connect(
        [&](const CardPlayedEvent& e) { played.push_back(e); });

    engine.testHook_executeIntent(forged);

    EXPECT_TRUE(played.empty())
        << "The claim must be re-checked against live state and REJECTED, the "
           "way the Flow claim is forty lines later — the battlefield flag "
           "alone does not earn the discount, an eligible friendly unit to "
           "choose there does.";
    EXPECT_EQ(countReady(s, P1), 3);
    EXPECT_EQ(countExhausted(s, P1), 0);
    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "Nothing may be paid on the rejected path.";
    const auto& hand = s.player(P1).hand;
    EXPECT_NE(std::find(hand.begin(), hand.end(), spell), hand.end())
        << "and nothing may be mutated: the spell stays in hand.";
    EXPECT_TRUE(agent1.unit_prompts.empty());
}

TEST_F(SandsweptTombTest, RestrictedPlayKeepsItsDiscountWhenTheTombUnitDiesFirst) {
    // CR: a cost is locked in when the play is made. A LEGITIMATE restricted
    // play that loses its Tomb unit between payment and resolution therefore
    // keeps the discount and simply fizzles — the same outcome as any spell
    // whose only target vanishes. Documented here so the re-validation added
    // for the forged-intent case above is never "fixed" into a refund.
    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    auto tomb_unit = addUnitIn(s, P1, /*at_bf=*/0, "Tomb Unit");
    auto far_unit  = addUnitIn(s, P1, /*at_bf=*/1, "Far Unit");
    s.getObject(far_unit).is_exhausted = true;
    auto spell = addToZoneIn(s, P1, kRideTheWind, ZoneType::Hand);
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Chaos);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 2u);
    Intent restricted;
    for (const auto& o : offers)
        if (o.target_battlefield_restriction.has_value()) restricted = o;
    ASSERT_TRUE(restricted.target_battlefield_restriction.has_value());

    // Kill the Tomb unit the instant the spell is played — after the cost is
    // paid, before the chain resolves it.
    auto conn = events.on_card_played.connect([&](const CardPlayedEvent&) {
        auto& u = s.getObject(tomb_unit);
        u.location.reset();
        u.zone = ZoneType::Trash;
        s.player(P1).trash.push_back(tomb_unit);
    });

    engine.testHook_executeIntent(restricted);

    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "The discounted cost was locked in at play — no rune is recycled "
           "after the fact.";
    EXPECT_EQ(countExhausted(s, P1), 2);
    EXPECT_TRUE(agent1.unit_prompts.empty())
        << "Nothing is left to choose at the Tomb, so no target choice is "
           "published — the spell fizzles.";
    EXPECT_TRUE(s.getObject(far_unit).isAtBattlefield())
        << "and the commitment holds to the end: the unit at the OTHER "
           "battlefield is never offered as a fallback.";
    EXPECT_TRUE(s.getObject(far_unit).is_exhausted);
}

// ─── Pair-pick spells (review fix round 1, Important #2 — controller ruling) ──
//
// Star-Crossed (690) and Switcheroo (466) are both in Tyler's deck and both
// choose their two units at RESOLVE time through Card::pickTargetPair. The
// commitment is enforced caller-agnostically: at the A step the list is
// narrowed to {a : a is a friendly unit at the Tomb} ∪ {a : some b reachable
// from a is}, and at the B step B is narrowed to friendly-at-Tomb unless the
// chosen A already satisfies it. Neither card knows any of this.

TEST_F(SandsweptTombTest, StarCrossedRestrictedOfferNarrowsTheFriendlyList) {
    constexpr CardDefId kStarCrossed = 690;   // 3E + 1 [Chaos], A = friendly, B = enemy

    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    auto tomb_friendly = addUnitIn(s, P1, /*at_bf=*/0, "Friendly At Tomb");
    auto far_friendly  = addUnitIn(s, P1, /*at_bf=*/1, "Friendly Elsewhere");
    auto enemy         = addUnitIn(s, P2, /*at_bf=*/1, "Enemy");
    auto spell = addToZoneIn(s, P1, kStarCrossed, ZoneType::Hand);

    // 3 ORDER runes: the [3] energy is covered, the [1] Chaos power is not —
    // so only the discounted, restricted play can be offered at all.
    for (int i = 0; i < 3; ++i) addReadyRune(s, P1, Domain::Order);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "A pair-pick spell that can choose a friendly unit at the Tomb must "
           "get the restricted offer too — Star-Crossed and Switcheroo are in "
           "the deck this branch exists for.";
    ASSERT_TRUE(offers[0].target_battlefield_restriction.has_value());
    EXPECT_EQ(*offers[0].target_battlefield_restriction, 0u);

    engine.testHook_executeIntent(offers[0]);

    EXPECT_EQ(s.player(P1).rune_deck.size(), 0u)
        << "the discount is charged: nothing recycled for the [1] power";
    EXPECT_EQ(countExhausted(s, P1), 3);
    ASSERT_EQ(agent1.unit_prompts.size(), 2u)
        << "Star-Crossed publishes two picks: the friendly, then the enemy.";
    EXPECT_EQ(agent1.unit_prompts[0], std::vector<GameObjectId>{tomb_friendly})
        << "The A list is narrowed to friendly units at the Tomb — the "
           "friendly unit elsewhere may not be chosen under the commitment.";
    EXPECT_EQ(agent1.unit_prompts[1], std::vector<GameObjectId>{enemy})
        << "The B list is left alone once A already satisfies the commitment: "
           "the enemy need not stand at the Tomb.";
    EXPECT_EQ(s.getObject(tomb_friendly).zone, ZoneType::Hand);
    EXPECT_EQ(s.getObject(enemy).zone, ZoneType::Hand);
    EXPECT_TRUE(s.getObject(far_friendly).isAtBattlefield());
}

TEST_F(SandsweptTombTest, SwitcherooGetsNoRestrictedOfferWithOnlyEnemiesAtTheTomb) {
    constexpr CardDefId kSwitcheroo = 466;   // 2E + 2 [Chaos], A = any unit at a BF

    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    // Only ENEMY units stand at the Tomb; P1's own units are elsewhere. There
    // is no friendly unit "here" for the spell to choose, so nothing earns the
    // discount — Switcheroo's A list contains Tomb units, but they are the
    // wrong player's.
    addUnitIn(s, P2, /*at_bf=*/0, "Enemy At Tomb A");
    addUnitIn(s, P2, /*at_bf=*/0, "Enemy At Tomb B");
    addUnitIn(s, P1, /*at_bf=*/1, "Friendly Elsewhere A");
    addUnitIn(s, P1, /*at_bf=*/1, "Friendly Elsewhere B");
    auto spell = addToZoneIn(s, P1, kSwitcheroo, ZoneType::Hand);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Chaos);

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 1u)
        << "Eligibility is 'a FRIENDLY unit at the Tomb', not 'a unit at the "
           "Tomb' — with only enemies there, no restricted offer may appear.";
    EXPECT_FALSE(offers[0].target_battlefield_restriction.has_value());
}

TEST_F(SandsweptTombTest, SwitcherooRestrictedPlayNarrowsTheSecondPickWhenTheFirstIsEnemy) {
    constexpr CardDefId kSwitcheroo = 466;

    GameEngine engine(card_db, events, card_registry);
    TargetPromptRecorder agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    placeTomb(engine, 0);
    // At the Tomb: two enemies and one friendly. Switcheroo's B list is "the
    // other units at A's battlefield", so picking an enemy A leaves both the
    // other enemy AND the friendly reachable — exactly the case where B must
    // be narrowed for the commitment to mean anything.
    auto enemy_a  = addUnitIn(s, P2, /*at_bf=*/0, "Enemy At Tomb A");
    auto enemy_b  = addUnitIn(s, P2, /*at_bf=*/0, "Enemy At Tomb B");
    auto friendly = addUnitIn(s, P1, /*at_bf=*/0, "Friendly At Tomb");
    auto far_friendly = addUnitIn(s, P1, /*at_bf=*/1, "Friendly Elsewhere");
    auto far_enemy    = addUnitIn(s, P2, /*at_bf=*/1, "Enemy Elsewhere");
    auto spell = addToZoneIn(s, P1, kSwitcheroo, ZoneType::Hand);
    for (int i = 0; i < 4; ++i) addReadyRune(s, P1, Domain::Chaos);

    agent1.prefer = {enemy_a};   // drive the A pick down the enemy branch

    auto offers = intentsFor(engine.generateLegalActions(), spell);
    ASSERT_EQ(offers.size(), 2u) << "normal + restricted";
    Intent restricted;
    for (const auto& o : offers)
        if (o.target_battlefield_restriction.has_value()) restricted = o;
    ASSERT_TRUE(restricted.target_battlefield_restriction.has_value());

    engine.testHook_executeIntent(restricted);

    ASSERT_EQ(agent1.unit_prompts.size(), 2u);

    // A: kept if it IS a friendly-at-Tomb, or if one is reachable as its B.
    std::vector<GameObjectId> a_list = agent1.unit_prompts[0];
    std::sort(a_list.begin(), a_list.end());
    std::vector<GameObjectId> a_want = {enemy_a, enemy_b, friendly};
    std::sort(a_want.begin(), a_want.end());
    EXPECT_EQ(a_list, a_want)
        << "The A list keeps the friendly unit at the Tomb and the two enemies "
           "standing next to it (each can still reach that friendly as its "
           "second pick), and drops the units at the other battlefield, from "
           "which the commitment can never be met.";
    EXPECT_EQ(std::find(a_list.begin(), a_list.end(), far_friendly), a_list.end());
    EXPECT_EQ(std::find(a_list.begin(), a_list.end(), far_enemy), a_list.end());

    // B: A was an enemy, so the commitment is still unmet — B must be narrowed
    // to the friendly unit at the Tomb, not merely to "units at A's BF".
    EXPECT_EQ(agent1.unit_prompts[1], std::vector<GameObjectId>{friendly})
        << "With an enemy chosen first, the second pick is the only chance to "
           "meet the condition the discount was paid for: the other enemy at "
           "the Tomb may not be offered.";

    EXPECT_EQ(s.player(P1).rune_deck.size(), 1u)
        << "Switcheroo costs [2] power; the Tomb pays one, so exactly one "
           "Chaos rune is recycled.";
}
