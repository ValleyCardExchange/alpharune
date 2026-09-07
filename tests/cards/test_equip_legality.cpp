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
#include "core/events.h"
#include "core/game_state.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

using namespace riftbound;
using namespace riftbound::test;

namespace {

constexpr CardDefId kLastRites = 471;  // [Equip] — [P], Recycle 2 from trash
constexpr CardDefId kSoulSword = 601;  // [Equip] [G]

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

}  // namespace
