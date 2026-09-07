/// @file test_closed_state_abilities.cpp
/// Closed-State ACTIVATED abilities (CR 376 / 398-406) driven through the REAL
/// FEPR loop — the third member of the "second executor" family that
/// tests/cards/test_closed_state_plays.cpp covers for spells and permanents.
///
/// `GameEngine::generateClosedStateActions` offers
/// `IntentType::ActivateReactionAbility` for every `[Reaction]` activated
/// ability its controller can pay for (Seal of Discord 204's
/// "[E]: [Reaction] — [Add] [P]" is the one the Kennen deck actually runs, 3
/// copies of it). `ChainManager::stepExecuteAndPass` is the ONLY answerer of a
/// closed-state offer — it never calls `GameEngine::executeIntent`. Before
/// this fix its switch handled `PassPriority` and `PlayReaction` and fell off
/// the end for everything else ("Other intent types (ActivateReactionAbility
/// etc.) are Phase 3+"), so a chosen activation:
///
///   • paid no cost (the source was never exhausted),
///   • ran no `onActivate` (no power, no energy, no draw),
///   • added no chain item, and
///   • stayed legal — so the same intent was offered and picked again, for
///     10 consecutive decisions per closed-state window (the
///     kMaxPriorityPasses cap) with completely frozen state. Real decision
///     logs on this branch show 10–93-long bursts of exactly that.
///
/// The fix mirrors the play half: a third injected executor
/// (`ChainManager::setActivateAbility`, wired in `GameEngine::initSubsystems`)
/// routes the intent back out to `GameEngine::executeIntent`, whose activation
/// path pays `exhaust` / `disempower_self` / energy / discard / XP and
/// dispatches `onActivate`. These tests drive the real loop:
///
///   (1) a Seal of Discord activation in the Closed State EXHAUSTS the gear
///       and adds exactly 1 Chaos power;
///   (2) the spent Seal is not offered again while it is exhausted — the
///       burst's "same intent stays legal" half;
///   (3) a `[Reaction]` ability that puts an item on the chain restarts FEPR
///       from Finalize (the item is finalized and resolved, and its effect
///       lands). NOTE: every `[Reaction]` activated ability in the shipped
///       registry today is an "[Add]" ability (Seals 40/81/120/163/204/245 +
///       their second printings, Energy Conduit 98, Lux 312, Dragonsoul Sage
///       655, Gold 326/564, Ancient Henge 438, and the four Legends 294/296/
///       506/786) — there is no registry card whose reaction ability has a
///       non-Add on-resolve effect, so this test registers a test-local gear
///       with one rather than skipping the coverage;
///   (4) regression: a scripted agent that ALWAYS takes an activation when
///       offered no longer bursts. With 3 ready Seals on the board the
///       closed-state window produces at most 3 activations, not the
///       ten-deep loop of frozen repeats.

#include "tests/cards/card_test_fixture.h"

#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/events.h"
#include "core/game_state.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace riftbound;
using namespace riftbound::test;

namespace {

// Test-local ids, well above the shipped registry (787 cards) and clear of
// the 90x block test_flow.cpp and the 91x block test_closed_state_plays.cpp
// use.
constexpr CardDefId kOpener      = 920;  // [Action], free — opens the chain
constexpr CardDefId kChainingGgr = 921;  // gear, [E]: [Reaction] — draw 1
constexpr CardDefId kCostlyGear  = 922;  // gear, [3][E]: [Reaction] — draw 1

// The real card this whole fix exists for: 3 copies ride in
// decks/kennen_tyler.txt.
constexpr CardDefId kSealOfDiscord = 204;

/// Free [Action] spell with no effect. Its only job is to put an item on the
/// chain so the game enters the Closed State with a real FEPR loop running.
class OpenerSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kOpener;
        d.name = "Closed State Ability Opener";
        d.card_type = CardType::Spell;
        d.domains = {Domain::Fury};
        d.energy_cost = 0;
        d.keywords.set(Keyword::Action);
        return d;
    }();
};

/// A `[Reaction]` activated ability that is NOT an "[Add]" ability: it draws a
/// card on resolution. Unlike every Add ability in the registry it has a
/// visible effect that can only happen if the chain item it creates is
/// finalized and resolved, which is exactly what test (3) asserts.
class ChainingReactionGear : public GearCard {
public:
    const CardDef& def() const override { return def_; }
    TriggerType triggerType() const override { return TriggerType::Activated; }
    bool hasActivatedAbility() const override { return true; }
    bool isReactionAbility() const override { return true; }
    ActivationCost getActivationCost() const override { return {.exhaust = true}; }
    void onActivate(CardContext& ctx,
                    const std::vector<GameObjectId>& /*targets*/) override {
        ctx.executor.drawCards(ctx.controller, 1);
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kChainingGgr;
        d.name = "Chaining Reaction Test Gear";
        d.card_type = CardType::Gear;
        d.domains = {Domain::Mind};
        d.energy_cost = 1;
        d.keywords.set(Keyword::Reaction);
        d.ability_text = "[E]: [Reaction] - Draw 1.";
        return d;
    }();
};

/// The same shape as ChainingReactionGear with a NON-ZERO energy cost. No
/// shipped [Reaction] ability costs energy, so the "energy cost cannot be
/// paid" path is only reachable through a card like this one.
class CostlyReactionGear : public GearCard {
public:
    const CardDef& def() const override { return def_; }
    TriggerType triggerType() const override { return TriggerType::Activated; }
    bool hasActivatedAbility() const override { return true; }
    bool isReactionAbility() const override { return true; }
    ActivationCost getActivationCost() const override {
        return {.exhaust = true, .energy = 3};
    }
    void onActivate(CardContext& ctx,
                    const std::vector<GameObjectId>& /*targets*/) override {
        ctx.executor.drawCards(ctx.controller, 1);
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kCostlyGear;
        d.name = "Costly Reaction Test Gear";
        d.card_type = CardType::Gear;
        d.domains = {Domain::Mind};
        d.energy_cost = 1;
        d.keywords.set(Keyword::Reaction);
        d.ability_text = "[3], [E]: [Reaction] - Draw 1.";
        return d;
    }();
};

/// Plays a scripted intent on its first call (whatever the offer set says —
/// the point is a HAND-BUILT intent reaching the executor), passes priority
/// afterwards, and records for every call whether P1 was in the
/// priority-pass set at the time. That set is what a false "it executed"
/// destroys: stepExecuteAndPass clears it on entry, so a spurious FEPR
/// restart wipes the passes already accumulated.
class PassSetWatchingAgent : public AgentInterface {
public:
    std::vector<Intent> script;
    size_t cursor = 0;
    int call_count = 0;
    std::vector<bool> p1_had_passed;

    Intent selectAction(const GameState& s,
                        const std::vector<Intent>& legal) override {
        ++call_count;
        const auto& passed = s.turn.players_passed_priority;
        p1_had_passed.push_back(
            passed.find(PlayerId::Player1) != passed.end());
        if (cursor < script.size()) return script[cursor++];
        for (const auto& i : legal)
            if (i.type == IntentType::PassPriority) return i;
        return legal.empty() ? Intent{} : legal.front();
    }
};

/// Agent for the closed-state activation tests.
///
///   • `want` selects which offered intent to take. With `repeat = false` it
///     fires once and the agent passes forever after (tests 1-3); with
///     `repeat = true` it takes a match EVERY time one is offered, which is
///     the burst-reproducing agent of test (4).
///   • Every legal-action set the agent is shown is recorded in `offers`, so a
///     test can assert what was (and was not) still on the menu after an
///     activation.
class ActivationAgent : public AgentInterface {
public:
    std::function<bool(const Intent&)> want;
    bool repeat = false;
    bool taken = false;
    int takes = 0;
    int call_count = 0;
    std::vector<std::vector<Intent>> offers;

    Intent selectAction(const GameState&,
                        const std::vector<Intent>& legal) override {
        ++call_count;
        if (legal.empty()) return Intent{};
        offers.push_back(legal);

        if (want && (repeat || !taken)) {
            for (const auto& i : legal) {
                if (!want(i)) continue;
                taken = true;
                ++takes;
                return i;
            }
        }
        for (const auto& i : legal)
            if (i.type == IntentType::PassPriority) return i;
        return legal.front();
    }
};

bool isActivation(const Intent& i) {
    return i.type == IntentType::ActivateReactionAbility ||
           i.type == IntentType::ActivateAbility ||
           i.type == IntentType::ActivateActionAbility;
}

}  // namespace

// ─── Fixture ───────────────────────────────────────────────────────────────

class ClosedStateAbilitiesTest : public CardTestFixture {
protected:
    void SetUp() override {
        CardTestFixture::SetUp();
        card_registry.registerCard(kOpener, std::make_unique<OpenerSpell>());
        card_registry.registerCard(kChainingGgr,
                                    std::make_unique<ChainingReactionGear>());
        card_registry.registerCard(kCostlyGear,
                                    std::make_unique<CostlyReactionGear>());
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
        obj.zone = ZoneType::Hand;
        s.player(owner).hand.push_back(id);
        return id;
    }

    GameObjectId addToDeckIn(GameState& s, PlayerId owner, CardDefId def_id) {
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_def_id = def_id;
        const auto& def = card_db.get(def_id);
        obj.name = def.name;
        obj.card_type = def.card_type;
        obj.zone = ZoneType::MainDeck;
        s.player(owner).main_deck.push_back(id);
        return id;
    }

    /// Put a GEAR card object on the board in `owner`'s base, ready. That is
    /// where an unattached Seal lives, and `location.has_value()` is what the
    /// closed-state activation generator gates on.
    GameObjectId addGearInBase(GameState& s, PlayerId owner, CardDefId def_id) {
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_def_id = def_id;
        const auto& def = card_db.get(def_id);
        obj.name = def.name;
        obj.card_type = def.card_type;
        obj.domains = def.domains;
        obj.keywords = def.keywords;
        obj.zone = ZoneType::Base;
        obj.location = BaseLocation{owner};
        obj.is_exhausted = false;
        return id;
    }

    /// Play the free opener from P1's hand. executePlaySpell puts it on the
    /// chain and runs the FEPR loop, so everything after this call happens in
    /// the Closed State with `agent1` holding priority first.
    void openTheChain(GameEngine& engine, GameObjectId opener) {
        Intent play;
        play.type = IntentType::PlayActionCard;
        play.player = P1;
        play.card = opener;
        play.play_source = Intent::PlaySource::Hand;
        engine.testHook_executeIntent(play);
    }

    static int chaosPower(const GameState& s, PlayerId p) {
        return s.player(p).rune_pool.power[static_cast<int>(Domain::Chaos)];
    }

    GameObjectId addRuneIn(GameState& s, PlayerId owner, Domain domain) {
        auto id = s.createObject();
        auto& r = s.getObject(id);
        r.owner = owner; r.controller = owner;
        r.card_type = CardType::Rune;
        r.name = std::string(toString(domain)) + " Rune";
        r.domains = {domain};
        r.zone = ZoneType::Base;
        r.location = BaseLocation{owner};
        return id;
    }

    static int readyRunesIn(const GameState& s, PlayerId p) {
        int n = 0;
        auto base = BaseLocation{p};
        for (const auto& [id, obj] : s.objects) {
            if (!obj.isRune() || obj.controller != p || obj.is_exhausted) continue;
            if (!obj.location.has_value() || *obj.location != LocationId{base}) continue;
            ++n;
        }
        return n;
    }
};

// ─── (1) The activation actually executes ──────────────────────────────────

TEST_F(ClosedStateAbilitiesTest, ClosedStateSealActivationExhaustsAndAddsPower) {
    GameEngine engine(card_db, events, card_registry);
    ActivationAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToHandIn(s, P1, kOpener);
    auto seal   = addGearInBase(s, P1, kSealOfDiscord);

    const int power_before = chaosPower(s, P1);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::ActivateReactionAbility &&
               i.ability_source == seal;
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: generateClosedStateActions must offer Seal of Discord's "
           "[Reaction] activation for the agent to take.";

    EXPECT_TRUE(s.getObject(seal).is_exhausted)
        << "The [E] component of the activation cost must be PAID. Before the "
           "fix stepExecuteAndPass dropped the intent on the floor, so the "
           "Seal stayed ready and the same activation stayed legal forever.";
    EXPECT_EQ(chaosPower(s, P1), power_before + 1)
        << "Seal of Discord's onActivate must run: [Add] [P] puts exactly 1 "
           "Chaos power in the controller's pool.";
}

// ─── (2) A spent Seal leaves the menu ──────────────────────────────────────

TEST_F(ClosedStateAbilitiesTest, ExhaustedSealIsNotOfferedAgain) {
    GameEngine engine(card_db, events, card_registry);
    ActivationAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToHandIn(s, P1, kOpener);
    auto seal   = addGearInBase(s, P1, kSealOfDiscord);

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::ActivateReactionAbility &&
               i.ability_source == seal;
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken);
    ASSERT_FALSE(agent1.offers.empty());

    // Find the offer set the activation was taken from, then check every
    // later offer to P1.
    size_t taken_at = agent1.offers.size();
    for (size_t k = 0; k < agent1.offers.size(); ++k) {
        bool has = std::any_of(agent1.offers[k].begin(), agent1.offers[k].end(),
                                [&](const Intent& i) {
                                    return isActivation(i) &&
                                           i.ability_source == seal;
                                });
        if (has) { taken_at = k; break; }
    }
    ASSERT_LT(taken_at, agent1.offers.size())
        << "sanity: the activation must have been on at least one menu.";

    for (size_t k = taken_at + 1; k < agent1.offers.size(); ++k) {
        for (const auto& i : agent1.offers[k]) {
            EXPECT_FALSE(isActivation(i) && i.ability_source == seal)
                << "Offer #" << k << " still lists an activation for the "
                   "EXHAUSTED Seal. generateClosedStateActions skips an "
                   "[E]-cost ability whose source is exhausted, so this can "
                   "only happen if the exhaust cost was never paid.";
        }
    }
}

// ─── (3) A chain-adding reaction ability restarts FEPR from Finalize ───────

TEST_F(ClosedStateAbilitiesTest, ChainAddingReactionAbilityRestartsFeprFromFinalize) {
    GameEngine engine(card_db, events, card_registry);
    ActivationAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToHandIn(s, P1, kOpener);
    auto gear   = addGearInBase(s, P1, kChainingGgr);
    addToDeckIn(s, P1, kOpener);  // one card to draw

    const size_t hand_before = s.player(P1).hand.size();  // includes the opener
    const size_t deck_before = s.player(P1).main_deck.size();

    std::vector<GameObjectId> finalized, resolved;
    auto c1 = events.on_chain_item_finalized.connect(
        [&](const ChainItemFinalizedEvent& e) { finalized.push_back(e.source); });
    auto c2 = events.on_chain_item_resolved.connect(
        [&](const ChainItemResolvedEvent& e) { resolved.push_back(e.source); });

    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::ActivateReactionAbility &&
               i.ability_source == gear;
    };

    openTheChain(engine, opener);

    ASSERT_TRUE(agent1.taken)
        << "sanity: the test gear's [Reaction] activation must be offered.";

    EXPECT_EQ(std::count(finalized.begin(), finalized.end(), gear), 1)
        << "The ability's chain item must be FINALIZED — that only happens if "
           "stepExecuteAndPass reported 'item added' and processFEPR restarted "
           "the loop at Finalize.";
    EXPECT_EQ(std::count(resolved.begin(), resolved.end(), gear), 1)
        << "…and then resolved by stepResolve.";
    EXPECT_TRUE(s.getObject(gear).is_exhausted);
    // The opener leaves the hand on its way to the chain, the draw puts one
    // card in: net hand size is unchanged, deck is one shorter.
    EXPECT_EQ(s.player(P1).main_deck.size(), deck_before - 1)
        << "onActivate must have run at resolution — the ability draws 1.";
    EXPECT_EQ(s.player(P1).hand.size(), hand_before)
        << "opener out (-1), draw in (+1).";
}

// ─── (4) Regression: no activation burst ───────────────────────────────────

TEST_F(ClosedStateAbilitiesTest, RepeatedActivationChoiceDoesNotBurst) {
    GameEngine engine(card_db, events, card_registry);
    ActivationAgent agent1;
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToHandIn(s, P1, kOpener);
    std::vector<GameObjectId> seals;
    for (int i = 0; i < 3; ++i)
        seals.push_back(addGearInBase(s, P1, kSealOfDiscord));

    const int power_before = chaosPower(s, P1);

    // The burst-reproducing agent: take an activation EVERY time one is on
    // the menu. Against a working engine each pick spends a distinct Seal, so
    // the supply of activations is exactly 3 and then it has to pass.
    agent1.repeat = true;
    agent1.want = [&](const Intent& i) { return isActivation(i); };

    openTheChain(engine, opener);

    EXPECT_LE(agent1.takes, 3)
        << "Bounded by the number of READY Seals. A higher count means an "
           "activation was picked without being executed and stayed legal — "
           "the 10-93-decision frozen-state burst from the real logs.";
    EXPECT_EQ(agent1.takes, 3)
        << "All three ready Seals should be activatable exactly once each.";
    EXPECT_EQ(chaosPower(s, P1), power_before + 3);
    for (auto id : seals)
        EXPECT_TRUE(s.getObject(id).is_exhausted);
}

// ─── (5) The SHOWDOWN half of the same hole ────────────────────────────────

TEST_F(ClosedStateAbilitiesTest, ShowdownReactionActivationExecutes) {
    // Found while fixing the closed-state path, and fixed by the same line.
    // `resolveShowdownDecision` already listed ActivateReactionAbility in its
    // dispatch switch and forwarded it to executeIntent — but executeIntent's
    // activation case covered only ActivateAbility / ActivateActionAbility, so
    // a [Reaction] activation fell to `default: break` there too: focus passed
    // and the pass-set cleared while nothing at all had happened. Same shape
    // as tests/cards/test_combat_showdown_dispatch.cpp's PlayReaction finding.
    GameEngine engine(card_db, events, card_registry);
    ActivationAgent agent1;  // want unset → always passes priority
    FirstChoiceAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto seal = addGearInBase(s, P1, kSealOfDiscord);
    const int power_before = chaosPower(s, P1);

    Intent act;
    act.type = IntentType::ActivateReactionAbility;
    act.player = P1;
    act.ability_source = seal;
    act.ability_index = 0;

    int action_count = 0;
    auto next = engine.testHook_resolveShowdownDecision(0, act, P1, action_count);

    EXPECT_TRUE(next.has_value());
    EXPECT_TRUE(s.getObject(seal).is_exhausted)
        << "The [E] cost must be paid on the showdown path too.";
    EXPECT_EQ(chaosPower(s, P1), power_before + 1)
        << "Seal of Discord's onActivate must run.";
}

// ─── (6) An UNDERPAID activation is rejected, and the pass set survives ────
//
// `generateClosedStateActions` gates affordability, so this only arrives as a
// hand-built intent (agent, OpenSpiel bridge, replay) — which is exactly what
// the ChainManager now hands to the engine. Before the fix executeIntent paid
// what it could off the ready runes and executed in full, and the closed-state
// callback inferred success from a resource fingerprint, so the FEPR loop
// restarted: `players_passed_priority` was cleared and P1's pass discarded for
// an activation whose cost was never paid.

TEST_F(ClosedStateAbilitiesTest, UnderpaidActivationIsRejectedAndKeepsThePassSet) {
    GameEngine engine(card_db, events, card_registry);
    PassSetWatchingAgent agent1;  // P1: opens the chain, then passes
    PassSetWatchingAgent agent2;  // P2: tries the unpayable activation
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToHandIn(s, P1, kOpener);
    auto gear   = addGearInBase(s, P2, kCostlyGear);
    addRuneIn(s, P2, Domain::Mind);  // ONE ready rune against a [3] cost
    addToDeckIn(s, P2, kOpener);     // a card the draw could take

    Intent underpaid;
    underpaid.type = IntentType::ActivateReactionAbility;
    underpaid.player = P2;
    underpaid.ability_source = gear;
    underpaid.ability_index = 0;
    agent2.script = {underpaid};

    std::vector<std::string> warnings;
    auto conn = events.on_log.connect([&](const LogEvent& e) {
        if (e.level == LogLevel::Warning) warnings.push_back(e.message);
    });

    const size_t p2_deck_before = s.player(P2).main_deck.size();
    openTheChain(engine, opener);
    conn.disconnect();

    bool warned = false;
    for (const auto& w : warnings)
        if (w.find("ACTIVATE_COST") != std::string::npos &&
            w.find("Costly Reaction Test Gear") != std::string::npos) warned = true;
    EXPECT_TRUE(warned)
        << "an activation whose energy cost cannot be paid must be rejected "
           "LOUDLY before anything is spent.";

    EXPECT_FALSE(s.getObject(gear).is_exhausted)
        << "the [E] must not be paid for a cost that cannot be completed";
    EXPECT_EQ(readyRunesIn(s, P2), 1)
        << "nothing paid — the one ready rune is still ready";
    EXPECT_EQ(s.player(P2).main_deck.size(), p2_deck_before)
        << "onActivate must not have run";

    ASSERT_GE(agent2.p1_had_passed.size(), 2u)
        << "P2 is asked again after its intent is rejected";
    EXPECT_TRUE(agent2.p1_had_passed[1])
        << "P1's priority pass must SURVIVE the rejection. A false 'it "
           "executed' restarts FEPR, and stepExecuteAndPass clears "
           "players_passed_priority on entry.";
    EXPECT_EQ(agent1.call_count, 1)
        << "P1 passed once and was never re-queried — a spurious FEPR restart "
           "would hand it priority all over again.";
}

// ─── (7) …and a LEGITIMATE activation still resets it ──────────────────────
//
// The control for (6): the same board with the energy actually available. The
// activation executes, FEPR restarts from Finalize, and the pass set is
// cleared — the CR 337.1.b.3 bookkeeping a play gets.

TEST_F(ClosedStateAbilitiesTest, PayableActivationExecutesAndResetsThePassSet) {
    GameEngine engine(card_db, events, card_registry);
    PassSetWatchingAgent agent1;
    PassSetWatchingAgent agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto opener = addToHandIn(s, P1, kOpener);
    auto gear   = addGearInBase(s, P2, kCostlyGear);
    for (int i = 0; i < 3; ++i) addRuneIn(s, P2, Domain::Mind);
    addToDeckIn(s, P2, kOpener);

    Intent act;
    act.type = IntentType::ActivateReactionAbility;
    act.player = P2;
    act.ability_source = gear;
    act.ability_index = 0;
    agent2.script = {act};

    const size_t p2_deck_before = s.player(P2).main_deck.size();
    openTheChain(engine, opener);

    EXPECT_TRUE(s.getObject(gear).is_exhausted) << "the [E] was paid";
    EXPECT_EQ(readyRunesIn(s, P2), 0) << "all three runes paid the [3]";
    EXPECT_EQ(s.player(P2).main_deck.size(), p2_deck_before - 1)
        << "onActivate ran at resolution — the ability draws 1";

    ASSERT_GE(agent1.call_count, 2)
        << "the activation restarts FEPR, so P1 is granted priority again";
    ASSERT_GE(agent1.p1_had_passed.size(), 2u);
    EXPECT_FALSE(agent1.p1_had_passed[1])
        << "…with players_passed_priority cleared — P1's earlier pass is "
           "stale once P2 has acted (CR 337.1.b.3).";
}
