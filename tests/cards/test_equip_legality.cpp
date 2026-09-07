/// @file test_equip_legality.cpp
/// Equip legality (CR 818) — `Card::canEquip` is the ONE predicate the
/// main-phase generator gates equip offers on, and the first thing every
/// gear's `onEquip` consults.
///
/// Why this file exists (replay-loop iteration 0, spec addendum #16):
/// `generateMainPhaseActions` offered an equip `ActivateAbility` intent for
/// EVERY unattached friendly gear with no affordability check at all, and
/// `executeIntent` dropped `onEquip() == false` silently. The intent stayed
/// legal, so the agent re-picked it every decision: seed 2000 produced 497
/// consecutive Last Rites activations, consuming Kennen's entire main phase
/// against `kMainPhaseMaxActions = 500`.
///
/// Two card-level faithfulness bugs rode the same path and are covered here:
///   • Last Rites recycled its 2 trash cards BEFORE checking it could pay
///     the [P] — a partial payment on a failed equip.
///   • `card_helpers::payOnePower` required a READY rune. CR 164.2.b
///     ("Recycle this: [Reaction] — Add [C]") puts no readiness condition on
///     recycling a rune for power, and the engine's own canonical payer
///     agrees ("Each rune in base (exhausted or ready) of matching domain can
///     be recycled for 1 Power"). It now also PREFERS an exhausted rune, so
///     paying power never burns a ready one needlessly.
///
/// The registry guard is the load-bearing test: `Card::canEquip` defaults to
/// `true`, so any gear that declares `hasEquipAbility()` and forgets to
/// override it is NAMED by test (1) and caught half-paying by test (2).

#include "tests/cards/card_test_fixture.h"

#include "cards/card.h"
#include "cards/card_registry.h"
#include "cards/gear/equip_base.h"
#include "core/events.h"
#include "core/game_state.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace riftbound;
using namespace riftbound::test;

namespace {

constexpr CardDefId kLastRites = 471;  // [Equip] — [P], Recycle 2 from trash
constexpr CardDefId kSoulSword = 601;  // [Equip] [G]
constexpr CardDefId kBoneshiver = 439;  // [Equip] [1][D] — energy AND power
constexpr CardDefId kHextechGauntlets = 748;  // [Equip] [3][A] − target Might
constexpr CardDefId kBladeOfRuinedKing = 498;  // [Equip] — [Y], kill a friendly

// Test-local gear: the UniversalEquipGear shape with a NON-ZERO energy cost.
// All three shipped [A] gears (Spinning Axe 504, Forgefire Cape 507,
// Shurelya's Requiem 509) cost energy 0, so the "one rune cannot pay both the
// [1] and the [A]" collision is unreachable through the registry.
constexpr CardDefId kUniversalEnergyOne = 930;

class UniversalEnergyOneGear : public UniversalEquipGear {
public:
    UniversalEnergyOneGear() : UniversalEquipGear(/*energy_cost=*/1) {}
    const CardDef& def() const override { return def_; }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = kUniversalEnergyOne;
        d.name = "Universal Energy Test Gear";
        d.card_type = CardType::Gear;
        d.domains = {Domain::Fury};
        d.energy_cost = 1;
        d.might_bonus = 1;
        d.keywords.set(Keyword::Equip);
        d.ability_text = "[Equip] [1][A].";
        return d;
    }();
};

// ═══════════════════════════════════════════════════════════════════════════
// Fixture
// ═══════════════════════════════════════════════════════════════════════════

class EquipLegalityTest : public CardTestFixture {
protected:
    /// Spawn a gear on the board, unattached (mirrors test_equipment.cpp).
    GameObjectId addGearOnBoard(PlayerId owner, CardDefId def_id, int at_bf = -1) {
        auto id = state.createObject();
        auto& obj = state.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_def_id = def_id;
        obj.card_type = CardType::Gear;
        if (def_id != kInvalidId) {
            const auto& def = card_db.get(def_id);
            obj.name = def.name;
            obj.super_type = def.super_type;
            obj.keywords = def.keywords;
            obj.domains = def.domains;
            obj.tags = def.tags;
            obj.might_bonus = def.might_bonus;
        } else {
            obj.name = "TestGear";
        }
        if (at_bf >= 0) {
            obj.zone = ZoneType::BattlefieldZone;
            obj.location = BattlefieldLocation{static_cast<BattlefieldId>(at_bf)};
        } else {
            obj.zone = ZoneType::Base;
            obj.location = BaseLocation{owner};
        }
        return id;
    }

    /// Put a card in a player's trash (Last Rites' recycle-2 fodder).
    GameObjectId addToTrash(PlayerId owner, const std::string& name = "Trashed") {
        auto id = state.createObject();
        auto& obj = state.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_type = CardType::Spell;
        obj.name = name;
        obj.zone = ZoneType::Trash;
        state.player(owner).trash.push_back(id);
        return id;
    }

    /// Small serialized fingerprint of everything an equip payment could
    /// touch: trash / main-deck / rune-deck contents, per-rune exhaustion,
    /// and XP. Compared before/after to prove "no state change on reject".
    static std::string fingerprintOf(const GameState& s, PlayerId p) {
        std::ostringstream os;
        const auto& ps = s.player(p);
        os << "trash[";
        for (auto id : ps.trash) os << id << ' ';
        os << "] deck[";
        for (auto id : ps.main_deck) os << id << ' ';
        os << "] runedeck[";
        for (auto id : ps.rune_deck) os << id << ' ';
        os << "] runes[";
        std::vector<std::pair<GameObjectId, int>> runes;
        for (const auto& [id, obj] : s.objects) {
            if (!obj.isRune() || obj.controller != p) continue;
            runes.emplace_back(id, obj.is_exhausted ? 1 : 0);
        }
        std::sort(runes.begin(), runes.end());
        for (const auto& [id, ex] : runes) os << id << ':' << ex << ' ';
        os << "] xp=" << ps.xp;
        return os.str();
    }

    std::string fingerprint(PlayerId p) const { return fingerprintOf(state, p); }

    /// Every registered card that declares an [Equip] ability.
    std::vector<CardDefId> equipGearIds() {
        std::vector<CardDefId> out;
        for (auto id : card_registry.classDefIds()) {
            Card* c = card_registry.get(id);
            if (c && c->hasEquipAbility()) out.push_back(id);
        }
        return out;
    }

    std::string gearName(CardDefId id) {
        const auto& def = card_db.get(id);
        return "#" + std::to_string(id) + " " + def.name;
    }

    /// Reset to a bare two-battlefield board (for multi-scenario tests).
    void resetBoard() {
        state = GameState{};
        state.mode = ModeOfPlay{};
        state.players[0].id = P1;
        state.players[1].id = P2;
        addBattlefield();
        addBattlefield();
    }

    /// Stand up a GameEngine in P1's neutral-open main phase over a
    /// hand-built board. Returns nothing — callers use engine.mutableState().
    static void initMainPhase(GameEngine& engine) {
        auto& s = engine.mutableState();
        s.mode = ModeOfPlay{};
        s.players[0].id = PlayerId::Player1;
        s.players[1].id = PlayerId::Player2;
        s.turn.turn_player = PlayerId::Player1;
        s.turn.turn_number = 5;
        s.turn.phase = TurnPhase::MainPhase;
        s.turn.ns_state = NeutralShowdownState::Neutral;
        s.turn.oc_state = OpenClosedState::Open;
        BattlefieldState b0; b0.id = 0; s.battlefields.push_back(b0);
        BattlefieldState b1; b1.id = 1; s.battlefields.push_back(b1);
    }

    // ── The same three builders, against an arbitrary GameState ──
    // The fixture's own addUnit / addGearOnBoard / addRune write into the
    // fixture's `state`; the generator tests need them in the ENGINE's state,
    // which is a different object.
    GameObjectId addUnitIn(GameState& s, PlayerId owner, int might, int at_bf) {
        auto id = s.createObject();
        auto& u = s.getObject(id);
        u.owner = owner; u.controller = owner;
        u.card_type = CardType::Unit;
        u.name = "Equip Target " + std::to_string(id);
        u.base_might = might; u.current_might = might;
        u.zone = ZoneType::BattlefieldZone;
        u.location = BattlefieldLocation{static_cast<BattlefieldId>(at_bf)};
        return id;
    }

    GameObjectId addGearIn(GameState& s, PlayerId owner, CardDefId def_id,
                           int at_bf) {
        auto id = s.createObject();
        auto& g = s.getObject(id);
        const auto& def = card_db.get(def_id);
        g.owner = owner; g.controller = owner;
        g.card_def_id = def_id;
        g.name = def.name;
        g.card_type = CardType::Gear;
        g.super_type = def.super_type;
        g.keywords = def.keywords;
        g.domains = def.domains;
        g.tags = def.tags;
        g.might_bonus = def.might_bonus;
        g.zone = ZoneType::BattlefieldZone;
        g.location = BattlefieldLocation{static_cast<BattlefieldId>(at_bf)};
        return id;
    }

    GameObjectId addRuneIn(GameState& s, PlayerId owner, Domain domain,
                           bool exhausted = false) {
        auto id = s.createObject();
        auto& r = s.getObject(id);
        r.owner = owner; r.controller = owner;
        r.card_type = CardType::Rune;
        r.name = std::string(toString(domain)) + " Rune";
        r.domains = {domain};
        r.zone = ZoneType::Base;
        r.location = BaseLocation{owner};
        r.is_exhausted = exhausted;
        return id;
    }

    /// Every equip intent the main-phase generator offers for `gear`.
    static std::vector<Intent> equipOffersFor(GameEngine& engine,
                                              GameObjectId gear) {
        std::vector<Intent> out;
        for (const auto& a : engine.generateLegalActions()) {
            if (a.type == IntentType::ActivateAbility && a.ability_source == gear)
                out.push_back(a);
        }
        return out;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// (1) Registry guard — every equip gear reports UNPAYABLE on a bare board
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, RegistryGuard_EveryEquipGearIsUnpayableOnABareBoard) {
    // No runes in base, empty trash, 0 XP, no units. NOTHING is payable here.
    ASSERT_EQ(readyRuneCount(P1), 0);
    ASSERT_EQ(trashSize(P1), 0);
    ASSERT_EQ(state.player(P1).xp, 0);

    auto gear_ids = equipGearIds();
    ASSERT_GE(gear_ids.size(), 30u)
        << "the registry should hold dozens of [Equip] gear — this guard is "
           "worthless if the enumeration silently found nothing";

    for (auto id : gear_ids) {
        Card* c = card_registry.get(id);
        EXPECT_FALSE(c->canEquip(state, P1))
            << gearName(id) << " declares hasEquipAbility() but reports "
            << "canEquip == true with no runes, no trash and 0 XP. Every "
            << "equip gear must override Card::canEquip with its real "
            << "affordability check (Card::canEquip defaults to true).";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// (2) Registry guard — a rejected onEquip pays NOTHING
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, RegistryGuard_RejectedOnEquipLeavesTheBoardUntouched) {
    EffectExecutor exec(state, events, card_db, &card_registry);
    auto unit = addUnit(P1, kInvalidId, /*might=*/1, /*at_bf=*/0);

    for (auto id : equipGearIds()) {
        Card* c = card_registry.get(id);
        auto gear = addGearOnBoard(P1, id, /*at_bf=*/0);

        const auto before = fingerprint(P1);
        CardContext ctx{state, events, exec, P1, gear};
        bool ok = c->onEquip(ctx, unit);

        EXPECT_FALSE(ok) << gearName(id)
            << " equipped on a board with no runes, no trash and 0 XP — its "
               "cost cannot have been paid.";
        EXPECT_EQ(fingerprint(P1), before) << gearName(id)
            << " mutated trash / deck / rune-deck / rune exhaustion / XP on a "
               "rejected equip. Payment must be all-or-nothing.";
        EXPECT_FALSE(state.getObject(gear).attached_to.has_value())
            << gearName(id) << " attached itself without paying.";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// (3) Last Rites — three scenarios (CR 164.2.b + atomic payment)
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, LastRites_LegalityAndAtomicPaymentAcrossThreeBoards) {
    Card* c = card_registry.get(kLastRites);
    ASSERT_NE(c, nullptr);

    // ── A: two trash cards + only an EXHAUSTED Chaos rune → legal, equips ──
    {
        SCOPED_TRACE("A: 2 trash + exhausted Chaos rune");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kLastRites, /*at_bf=*/0);
        addToTrash(P1, "fodder-1");
        addToTrash(P1, "fodder-2");
        auto rune = addRune(P1, Domain::Chaos, /*exhausted=*/true);

        EXPECT_TRUE(c->canEquip(state, P1))
            << "CR 164.2.b puts no readiness condition on recycling a rune "
               "for power — an EXHAUSTED Chaos rune pays Last Rites' [P].";

        CardContext ctx{state, events, exec, P1, gear};
        ASSERT_TRUE(c->onEquip(ctx, unit));
        EXPECT_EQ(trashSize(P1), 0) << "both trash cards recycled";
        EXPECT_EQ(deckSize(P1), 2) << "the recycled pair went to the main deck";
        EXPECT_FALSE(state.getObject(rune).location.has_value())
            << "the Chaos rune was recycled for power";
        EXPECT_EQ(state.player(P1).rune_deck.size(), 1u);
        ASSERT_TRUE(state.getObject(gear).attached_to.has_value());
        EXPECT_EQ(*state.getObject(gear).attached_to, unit);
    }

    // ── B: two trash cards + NO Chaos rune → illegal, nothing paid ──
    {
        SCOPED_TRACE("B: 2 trash + no Chaos rune");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kLastRites, /*at_bf=*/0);
        addToTrash(P1, "fodder-1");
        addToTrash(P1, "fodder-2");
        addRune(P1, Domain::Fury);

        EXPECT_FALSE(c->canEquip(state, P1));
        const auto before = fingerprint(P1);
        CardContext ctx{state, events, exec, P1, gear};
        EXPECT_FALSE(c->onEquip(ctx, unit));
        EXPECT_EQ(trashSize(P1), 2)
            << "Last Rites must not recycle its trash cards before it knows "
               "the [P] is payable — that is a partial payment.";
        EXPECT_EQ(fingerprint(P1), before);
        EXPECT_FALSE(state.getObject(gear).attached_to.has_value());
    }

    // ── C: one trash card + a Chaos rune → illegal, nothing paid ──
    {
        SCOPED_TRACE("C: 1 trash + Chaos rune");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kLastRites, /*at_bf=*/0);
        addToTrash(P1, "fodder-1");
        addRune(P1, Domain::Chaos);

        EXPECT_FALSE(c->canEquip(state, P1));
        const auto before = fingerprint(P1);
        CardContext ctx{state, events, exec, P1, gear};
        EXPECT_FALSE(c->onEquip(ctx, unit));
        EXPECT_EQ(trashSize(P1), 1);
        EXPECT_EQ(fingerprint(P1), before);
        EXPECT_FALSE(state.getObject(gear).attached_to.has_value());
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// (4) Generator — an unpayable equip is never offered
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, Generator_UnpayableEquipIsNotOfferedButPayableOneIs) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent a1, a2;
    engine.testHook_setAgents(&a1, &a2);
    engine.testHook_initSubsystems();
    initMainPhase(engine);
    auto& s = engine.mutableState();

    // One friendly unit at bf0 (the only legal equip target).
    auto unit = s.createObject();
    {
        auto& u = s.getObject(unit);
        u.owner = P1; u.controller = P1;
        u.card_type = CardType::Unit;
        u.name = "Equip Target";
        u.base_might = 2; u.current_might = 2;
        u.zone = ZoneType::BattlefieldZone;
        u.location = BattlefieldLocation{0};
    }
    // Last Rites on the board, unattached, with NOTHING to pay with.
    auto gear = s.createObject();
    {
        auto& g = s.getObject(gear);
        g.owner = P1; g.controller = P1;
        g.card_def_id = kLastRites;
        const auto& def = card_db.get(kLastRites);
        g.name = def.name;
        g.card_type = CardType::Gear;
        g.domains = def.domains;
        g.tags = def.tags;
        g.might_bonus = def.might_bonus;
        g.zone = ZoneType::BattlefieldZone;
        g.location = BattlefieldLocation{0};
    }

    auto equipOffers = [&]() {
        int n = 0;
        for (const auto& a : engine.generateLegalActions()) {
            if (a.type == IntentType::ActivateAbility && a.ability_source == gear) ++n;
        }
        return n;
    };

    EXPECT_EQ(equipOffers(), 0)
        << "an unpayable Last Rites must not be offered — the agent re-picks "
           "a legal-but-inert intent until the main-phase action cap (497 "
           "repeats on seed 2000 before this fix).";

    // Make it payable: 2 cards in trash + one EXHAUSTED Chaos rune.
    for (int i = 0; i < 2; ++i) {
        auto t = s.createObject();
        auto& o = s.getObject(t);
        o.owner = P1; o.controller = P1;
        o.card_type = CardType::Spell;
        o.name = "fodder";
        o.zone = ZoneType::Trash;
        s.player(P1).trash.push_back(t);
    }
    {
        auto r = s.createObject();
        auto& o = s.getObject(r);
        o.owner = P1; o.controller = P1;
        o.card_type = CardType::Rune;
        o.name = "Chaos Rune";
        o.domains = {Domain::Chaos};
        o.zone = ZoneType::Base;
        o.location = BaseLocation{P1};
        o.is_exhausted = true;
    }

    EXPECT_EQ(equipOffers(), 1)
        << "a payable equip must still be offered exactly once per friendly "
           "unit (one unit on this board).";
}

// ═══════════════════════════════════════════════════════════════════════════
// (5) Faithfulness — CR 164.2.b: an exhausted rune pays power
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, SoulSword_EquipsOffAnExhaustedCalmRuneAndPrefersIt) {
    Card* c = card_registry.get(kSoulSword);
    ASSERT_NE(c, nullptr);

    // ── every Calm rune exhausted → still payable (CR 164.2.b) ──
    {
        SCOPED_TRACE("only exhausted Calm runes");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/3, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kSoulSword, /*at_bf=*/0);
        auto rune = addRune(P1, Domain::Calm, /*exhausted=*/true);

        EXPECT_TRUE(c->canEquip(state, P1));
        CardContext ctx{state, events, exec, P1, gear};
        ASSERT_TRUE(c->onEquip(ctx, unit))
            << "CR 164.2.b's power recycle has no readiness condition, and "
               "the engine's canonical payer already allows exhausted runes.";
        EXPECT_FALSE(state.getObject(rune).location.has_value());
        ASSERT_TRUE(state.getObject(gear).attached_to.has_value());
        EXPECT_EQ(*state.getObject(gear).attached_to, unit);
    }

    // ── one ready + one exhausted → the EXHAUSTED one is spent ──
    {
        SCOPED_TRACE("prefers the exhausted rune");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/3, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kSoulSword, /*at_bf=*/0);
        auto ready = addRune(P1, Domain::Calm, /*exhausted=*/false);
        auto spent = addRune(P1, Domain::Calm, /*exhausted=*/true);

        CardContext ctx{state, events, exec, P1, gear};
        ASSERT_TRUE(c->onEquip(ctx, unit));
        EXPECT_FALSE(state.getObject(spent).location.has_value())
            << "paying power must burn an exhausted rune before a ready one";
        EXPECT_TRUE(state.getObject(ready).location.has_value());
        EXPECT_EQ(readyRuneCount(P1, Domain::Calm), 1);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// (6) Executor — a rejected equip WARNS and changes nothing
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, Executor_RejectedEquipIntentLogsAWarningAndPaysNothing) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent a1, a2;
    engine.testHook_setAgents(&a1, &a2);
    engine.testHook_initSubsystems();
    initMainPhase(engine);
    auto& s = engine.mutableState();

    auto unit = s.createObject();
    {
        auto& u = s.getObject(unit);
        u.owner = P1; u.controller = P1;
        u.card_type = CardType::Unit;
        u.name = "Equip Target";
        u.base_might = 2; u.current_might = 2;
        u.zone = ZoneType::BattlefieldZone;
        u.location = BattlefieldLocation{0};
    }
    auto gear = s.createObject();
    {
        auto& g = s.getObject(gear);
        g.owner = P1; g.controller = P1;
        g.card_def_id = kLastRites;
        const auto& def = card_db.get(kLastRites);
        g.name = def.name;
        g.card_type = CardType::Gear;
        g.domains = def.domains;
        g.tags = def.tags;
        g.might_bonus = def.might_bonus;
        g.zone = ZoneType::BattlefieldZone;
        g.location = BattlefieldLocation{0};
    }
    // One card in trash and no Chaos rune: the intent is hand-built, so the
    // generator's new gate does not protect the executor here.
    {
        auto t = s.createObject();
        auto& o = s.getObject(t);
        o.owner = P1; o.controller = P1;
        o.card_type = CardType::Spell;
        o.name = "fodder";
        o.zone = ZoneType::Trash;
        s.player(P1).trash.push_back(t);
    }

    std::vector<std::string> warnings;
    auto conn = events.on_log.connect([&](const LogEvent& e) {
        if (e.level == LogLevel::Warning) warnings.push_back(e.message);
    });

    Intent equip;
    equip.type = IntentType::ActivateAbility;
    equip.player = P1;
    equip.ability_source = gear;
    equip.targets = {unit};

    const auto before = fingerprintOf(s, P1);
    engine.testHook_executeIntent(equip);
    conn.disconnect();

    bool warned = false;
    for (const auto& w : warnings) {
        if (w.find("EQUIP:") != std::string::npos &&
            w.find("Last Rites") != std::string::npos) { warned = true; break; }
    }
    EXPECT_TRUE(warned)
        << "executeIntent must LOG a warning naming the gear when onEquip "
           "returns false — a silently dropped equip is what let the agent "
           "re-pick the same inert intent 497 times.";
    EXPECT_EQ(fingerprintOf(s, P1), before);
    EXPECT_FALSE(s.getObject(gear).attached_to.has_value());
}


// ═══════════════════════════════════════════════════════════════════════════
// (7) Boneshiver — ONE rune cannot pay both the [1] energy and the [D] power
//
// The engine's canonical additional-cost payer (GameEngine::payAdditionalCost
// / canPayAdditionalCost) puts every rune it recycles for power into a
// `recycled` set and skips it when exhausting for energy: one physical rune
// never pays both halves of one cost. The equip payer must agree, or the two
// disagree on the identical cost shape — canPayAdditionalCost(energy=1,
// power=1, Body) is FALSE on a single ready Body rune while the equip path
// used to say true and pay the whole cost off it.
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, Boneshiver_OneRuneCannotPayBothEnergyAndPower) {
    Card* c = card_registry.get(kBoneshiver);
    ASSERT_NE(c, nullptr);

    // ── A: exactly one ready Body rune and nothing else → unpayable ──
    {
        SCOPED_TRACE("A: one ready Body rune, nothing else");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kBoneshiver, /*at_bf=*/0);
        auto body = addRune(P1, Domain::Body);

        EXPECT_FALSE(c->canEquip(state, P1))
            << "the [1] exhausts a ready rune and the [D] recycles a Body "
               "rune — with only ONE rune in base those are the same rune, "
               "which the engine's own payer forbids.";

        const auto before = fingerprint(P1);
        CardContext ctx{state, events, exec, P1, gear};
        EXPECT_FALSE(c->onEquip(ctx, unit));
        EXPECT_EQ(fingerprint(P1), before);
        EXPECT_TRUE(state.getObject(body).location.has_value())
            << "the single rune must still be in base — nothing was payable";
        EXPECT_FALSE(state.getObject(gear).attached_to.has_value());
    }

    // ── B: one ready Body rune + one other ready rune → equips, and the two
    //      halves come off DIFFERENT runes ──
    {
        SCOPED_TRACE("B: one ready Body rune + one ready Fury rune");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kBoneshiver, /*at_bf=*/0);
        auto body = addRune(P1, Domain::Body);   // the only [D] source
        auto other = addRune(P1, Domain::Fury);  // can only pay the [1]

        EXPECT_TRUE(c->canEquip(state, P1));
        CardContext ctx{state, events, exec, P1, gear};
        ASSERT_TRUE(c->onEquip(ctx, unit));

        EXPECT_FALSE(state.getObject(body).location.has_value())
            << "the Body rune is the only one that can pay the [D] — it must "
               "be the one recycled";
        ASSERT_TRUE(state.getObject(other).location.has_value())
            << "the Fury rune pays the [1] by EXHAUSTING; it stays in base";
        EXPECT_TRUE(state.getObject(other).is_exhausted)
            << "exactly one rune exhausted for the [1] energy, and it is not "
               "the one recycled for the [D]";
        EXPECT_EQ(state.player(P1).rune_deck.size(), 1u);
        EXPECT_EQ(readyRuneCount(P1), 0);
        ASSERT_TRUE(state.getObject(gear).attached_to.has_value());
        EXPECT_EQ(*state.getObject(gear).attached_to, unit);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// (8) Generator — the same single-rune Boneshiver is never offered
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, Generator_BoneshiverWithASingleRuneIsNotOffered) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent a1, a2;
    engine.testHook_setAgents(&a1, &a2);
    engine.testHook_initSubsystems();
    initMainPhase(engine);
    auto& s = engine.mutableState();

    addUnitIn(s, P1, /*might=*/2, /*at_bf=*/0);
    auto gear = addGearIn(s, P1, kBoneshiver, /*at_bf=*/0);
    addRuneIn(s, P1, Domain::Body);

    EXPECT_EQ(equipOffersFor(engine, gear).size(), 0u)
        << "one rune cannot pay both the [1] and the [D]; offering it is the "
           "shape that let an agent re-pick an inert intent all main phase.";

    addRuneIn(s, P1, Domain::Fury);
    EXPECT_EQ(equipOffersFor(engine, gear).size(), 1u)
        << "with a second ready rune the cost is payable off two distinct "
           "runes — one offer per friendly unit (one unit on this board).";
}

// ═══════════════════════════════════════════════════════════════════════════
// (9) Universal ([A]) equip — same collision, same rule
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, UniversalEquip_EnergyOneWithASingleRuneIsUnpayable) {
    card_registry.registerCard(kUniversalEnergyOne,
                               std::make_unique<UniversalEnergyOneGear>());
    card_db.buildFromClasses(card_registry);

    Card* c = card_registry.get(kUniversalEnergyOne);
    ASSERT_NE(c, nullptr);

    // ── A: one ready rune → the [1] and the [A] collide → unpayable ──
    {
        SCOPED_TRACE("A: a single ready rune");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kUniversalEnergyOne, /*at_bf=*/0);
        auto rune = addRune(P1, Domain::Fury);

        EXPECT_FALSE(c->canEquip(state, P1))
            << "[A] recycles a rune and [1] exhausts one — with a single rune "
               "in base those are the same rune.";
        const auto before = fingerprint(P1);
        CardContext ctx{state, events, exec, P1, gear};
        EXPECT_FALSE(c->onEquip(ctx, unit));
        EXPECT_EQ(fingerprint(P1), before);
        EXPECT_TRUE(state.getObject(rune).location.has_value());
        EXPECT_FALSE(state.getObject(gear).attached_to.has_value());
    }

    // ── B: two ready runes → equips off two distinct runes ──
    {
        SCOPED_TRACE("B: two ready runes");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearOnBoard(P1, kUniversalEnergyOne, /*at_bf=*/0);
        addRune(P1, Domain::Fury);
        addRune(P1, Domain::Calm);

        EXPECT_TRUE(c->canEquip(state, P1));
        CardContext ctx{state, events, exec, P1, gear};
        ASSERT_TRUE(c->onEquip(ctx, unit));
        EXPECT_EQ(state.player(P1).rune_deck.size(), 1u)
            << "exactly one rune recycled for the [A]";
        EXPECT_EQ(exhaustedRuneCount(P1), 1)
            << "exactly one OTHER rune exhausted for the [1]";
        EXPECT_EQ(readyRuneCount(P1), 0);
        ASSERT_TRUE(state.getObject(gear).attached_to.has_value());
    }

    // ── C: the generator never offers the single-rune case ──
    {
        SCOPED_TRACE("C: generator");
        GameEngine engine(card_db, events, card_registry);
        FirstChoiceAgent a1, a2;
        engine.testHook_setAgents(&a1, &a2);
        engine.testHook_initSubsystems();
        initMainPhase(engine);
        auto& s = engine.mutableState();
        addUnitIn(s, P1, /*might=*/2, /*at_bf=*/0);
        auto gear = addGearIn(s, P1, kUniversalEnergyOne, /*at_bf=*/0);
        addRuneIn(s, P1, Domain::Fury);
        EXPECT_EQ(equipOffersFor(engine, gear).size(), 0u);
        addRuneIn(s, P1, Domain::Calm);
        EXPECT_EQ(equipOffersFor(engine, gear).size(), 1u);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// (10) Hextech Gauntlets — legality is PER TARGET when the cost is
//
// "[Equip] [3][A]. This ability's Energy cost is reduced by the Might of the
// unit you choose." A target-agnostic `canEquip` answers for the CHEAPEST
// legal target, so the gear was offered against every friendly unit including
// ones it could not afford — the executor rejected each one and the intent
// stayed legal, which is the offered-then-rejected-forever burst again, just
// audible. `canEquipTarget` is the per-(gear, unit) predicate the generator's
// per-unit branch now gates on.
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, HextechGauntlets_OnlyTheAffordableTargetIsOffered) {
    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent a1, a2;
    engine.testHook_setAgents(&a1, &a2);
    engine.testHook_initSubsystems();
    initMainPhase(engine);
    auto& s = engine.mutableState();

    auto mighty = addUnitIn(s, P1, /*might=*/3, /*at_bf=*/0);  // energy 3-3 = 0
    auto weak   = addUnitIn(s, P1, /*might=*/0, /*at_bf=*/0);  // energy 3-0 = 3
    auto gear   = addGearIn(s, P1, kHextechGauntlets, /*at_bf=*/0);
    auto rune   = addRuneIn(s, P1, Domain::Fury);  // exactly one ready rune

    auto offers = equipOffersFor(engine, gear);
    ASSERT_EQ(offers.size(), 1u)
        << "one ready rune pays the [A] for the Might-3 unit (energy 0) and "
           "nothing else — the Might-0 unit needs 3 more energy, so its "
           "(gear, unit) intent is illegal and must not be generated.";
    ASSERT_EQ(offers[0].targets.size(), 1u);
    EXPECT_EQ(offers[0].targets[0], mighty);

    // The unaffordable target, hand-built: rejected LOUDLY, nothing paid.
    std::vector<std::string> warnings;
    auto conn = events.on_log.connect([&](const LogEvent& e) {
        if (e.level == LogLevel::Warning) warnings.push_back(e.message);
    });
    Intent bad;
    bad.type = IntentType::ActivateAbility;
    bad.player = P1;
    bad.ability_source = gear;
    bad.targets = {weak};
    const auto before = fingerprintOf(s, P1);
    engine.testHook_executeIntent(bad);
    conn.disconnect();

    bool warned = false;
    for (const auto& w : warnings)
        if (w.find("EQUIP:") != std::string::npos &&
            w.find("Hextech Gauntlets") != std::string::npos) warned = true;
    EXPECT_TRUE(warned);
    EXPECT_EQ(fingerprintOf(s, P1), before);
    EXPECT_FALSE(s.getObject(gear).attached_to.has_value());

    // …and the OFFERED one really is payable.
    engine.testHook_executeIntent(offers[0]);
    ASSERT_TRUE(s.getObject(gear).attached_to.has_value());
    EXPECT_EQ(*s.getObject(gear).attached_to, mighty);
    EXPECT_FALSE(s.getObject(rune).location.has_value())
        << "the single rune was recycled for the [A]";
}

// ═══════════════════════════════════════════════════════════════════════════
// (11) Blade of the Ruined King — pay the [Y], THEN kill
//
// `killObject` fires death events, and CR 164.2.b's rune recycle is itself a
// [Reaction]: something opened by the kill can spend the very Order rune
// `canEquip` counted. With the kill first, that leaves a friendly unit dead,
// the [Y] unpaid and the gear unattached. The irreversible action goes LAST.
// The UnitDied handler below is the test's stand-in for that reaction.
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(EquipLegalityTest, BladeOfTheRuinedKing_PaysThePowerBeforeItKills) {
    Card* c = card_registry.get(kBladeOfRuinedKing);
    ASSERT_NE(c, nullptr);

    // ── A: a reaction on the death spends the Order rune ──
    {
        SCOPED_TRACE("A: the death fires a rune-spending reaction");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit   = addUnit(P1, kInvalidId, /*might=*/3, /*at_bf=*/0);
        auto victim = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear   = addGearOnBoard(P1, kBladeOfRuinedKing, /*at_bf=*/0);
        auto order  = addRune(P1, Domain::Order);

        ASSERT_TRUE(c->canEquip(state, P1));

        bool rune_still_in_base_at_kill = true;
        auto conn = events.on_unit_died.connect([&](const UnitDiedEvent&) {
            auto& r = state.getObject(order);
            rune_still_in_base_at_kill = r.location.has_value();
            if (!r.location.has_value()) return;
            r.location = std::nullopt;
            r.zone = ZoneType::RuneDeck;
            state.player(P1).rune_deck.push_back(order);
        });

        CardContext ctx{state, events, exec, P1, gear};
        const bool ok = c->onEquip(ctx, unit);
        conn.disconnect();

        EXPECT_FALSE(rune_still_in_base_at_kill)
            << "the [Y] must already be paid when the unit dies — otherwise a "
               "reaction opened by the death can spend the rune canEquip "
               "counted, and the kill is unrecoverable.";
        EXPECT_TRUE(ok) << "the whole cost is payable, so the equip succeeds";
        EXPECT_FALSE(state.getObject(victim).location.has_value())
            << "the friendly unit is still killed as the other half of the cost";
        ASSERT_TRUE(state.getObject(gear).attached_to.has_value());
        EXPECT_EQ(*state.getObject(gear).attached_to, unit);
    }

    // ── B: no Order rune at all → nothing is killed ──
    {
        SCOPED_TRACE("B: [Y] unpayable");
        resetBoard();
        EffectExecutor exec(state, events, card_db, &card_registry);
        auto unit   = addUnit(P1, kInvalidId, /*might=*/3, /*at_bf=*/0);
        auto victim = addUnit(P1, kInvalidId, /*might=*/2, /*at_bf=*/0);
        auto gear   = addGearOnBoard(P1, kBladeOfRuinedKing, /*at_bf=*/0);
        addRune(P1, Domain::Fury);  // wrong domain

        const auto before = fingerprint(P1);
        CardContext ctx{state, events, exec, P1, gear};
        EXPECT_FALSE(c->onEquip(ctx, unit));
        EXPECT_TRUE(state.getObject(victim).location.has_value())
            << "if the power cannot be paid, nothing is killed";
        EXPECT_EQ(fingerprint(P1), before);
        EXPECT_FALSE(state.getObject(gear).attached_to.has_value());
    }
}

}  // namespace
