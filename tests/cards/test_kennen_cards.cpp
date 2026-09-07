/// @file test_kennen_cards.cpp
/// Kennen (Heart of the Tempest) deck support — Tasks 6-8.
///
/// Task 6: Burn N (CR 440) + shared burn-out (CR 431.2/431.3), driven
/// directly through EffectExecutor::burnCards.
/// Task 7: revealAndChoose's rest-destination parameter (Recycle | Trash),
/// which Lightning Rush (card tests land in Task 8) needs set to Trash.
///
/// Task 8: the four cards (788-791) themselves, wired end-to-end. Tests
/// #15/#16 (Kennen's conquer grant), #17 (Heart of the Tempest empower +
/// action), #18-#20 (Lightning Rush), #21/#28 (Up from the Deep) — spec
/// Section 8 + the audit addendum.

#include "tests/cards/card_test_fixture.h"

#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/events.h"
#include "core/game_state.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"

#include <gtest/gtest.h>

namespace riftbound::test {
namespace {

constexpr CardDefId kHeartOfTheTempest = 788;
constexpr CardDefId kKennenStormOfShuriken = 789;
constexpr CardDefId kLightningRush = 790;
constexpr CardDefId kUpFromTheDeep = 791;

// Two real spells already in the registry, used as trash fodder for
// Kennen's conquer grant — their printed costs are the assertion target.
constexpr CardDefId kTheHarrowingSpell = 198;   // 6E + 2 Chaos P
constexpr CardDefId kRideTheWindSpell = 173;    // 2E + 1 Chaos P

using KennenCardsTest = CardTestFixture;

// ─── Task 6, test #13 — burnCards moves the top N to trash, order preserved ──

TEST_F(KennenCardsTest, BurnCards_TopTwo_GoToTrash_OrderPreserved) {
    // addToDeck pushes to back(); back() is the top of the deck. Push
    // bottom-to-top so the deck reads [bottom, middle, top].
    auto bottom = addToDeck(P1, 1);
    auto middle = addToDeck(P1, 1);
    auto top = addToDeck(P1, 1);
    ASSERT_EQ(state.player(P1).main_deck.back(), top);

    EffectExecutor exec(state, events, card_db);
    exec.burnCards(P1, 2);

    // Top burns first, then the next-from-top — trash records that order.
    ASSERT_EQ(state.player(P1).trash.size(), 2u);
    EXPECT_EQ(state.player(P1).trash[0], top);
    EXPECT_EQ(state.player(P1).trash[1], middle);
    EXPECT_TRUE(inTrash(P1, top));
    EXPECT_TRUE(inTrash(P1, middle));
    EXPECT_FALSE(inTrash(P1, bottom));

    // Bottom card is untouched, still in the deck.
    EXPECT_TRUE(inDeck(P1, bottom));
    EXPECT_EQ(deckSize(P1), 1);

    // Zone/location bookkeeping matches discardCards/recycleCards precedent.
    EXPECT_EQ(state.getObject(top).zone, ZoneType::Trash);
    EXPECT_FALSE(state.getObject(top).location.has_value());
    EXPECT_EQ(state.getObject(middle).zone, ZoneType::Trash);
    EXPECT_FALSE(state.getObject(middle).location.has_value());
}

// ─── Task 6, test #14 — burn-out mid-burn, then continue from reshuffled deck ─

TEST_F(KennenCardsTest, BurnCards_EmptyDeckMidBurn_BurnsOutThenContinues) {
    // Deck has exactly 1 card; trash already has 2. Burning 2 forces a
    // burn-out after the first card, then the second burn comes from the
    // reshuffled deck (CR 440.4).
    auto trash1 = addToDeck(P1, 1);
    auto trash2 = addToDeck(P1, 1);
    state.player(P1).main_deck.clear();
    for (auto id : {trash1, trash2}) {
        state.getObject(id).zone = ZoneType::Trash;
        state.player(P1).trash.push_back(id);
    }
    auto top = addToDeck(P1, 1);
    ASSERT_EQ(state.player(P1).main_deck.size(), 1u);

    state.player(P1).score = 3;
    state.player(P2).score = 2;

    EffectExecutor exec(state, events, card_db);
    // No RNG installed -> burnOut's shuffle is skipped, so the reshuffled
    // deck keeps the append order deterministically ([trash1, trash2, top],
    // back() == top) for this test to assert against.
    exec.burnCards(P1, 2);

    // First burn came straight from the deck (the only card there).
    // Deck went empty -> burn-out fired (CR 431.2): opponent +1, burned_out set.
    EXPECT_TRUE(state.player(P1).burned_out);
    EXPECT_EQ(state.player(P1).score, 3);       // self score unaffected
    EXPECT_EQ(state.player(P2).score, 3);       // opponent +1 (CR 431.2.c)
    EXPECT_FALSE(state.game_over);

    // Second burn came from the reshuffled deck (deterministic: no rng_ set,
    // so main_deck == [trash1, trash2, top] after recycling trash1/trash2/top,
    // and burning pops back() == top).
    ASSERT_EQ(state.player(P1).trash.size(), 1u);
    EXPECT_EQ(state.player(P1).trash[0], top);
    EXPECT_EQ(state.player(P1).main_deck.size(), 2u);
    EXPECT_TRUE(inDeck(P1, trash1));
    EXPECT_TRUE(inDeck(P1, trash2));
}

// ─── Task 6 — burnCards stops consuming `count` once burn-out ends the game ──

TEST_F(KennenCardsTest, BurnCards_BurnOutEndsGame_StopsWithoutBurningMore) {
    // Deck has 1 card, trash has 1. Burning 3: the first burn empties the
    // deck; the second burn's burn-out recycles both cards and pushes P2
    // to (>=) victory score with more points than P1 -> game over. The
    // third (and the rest of the second) burn must never happen.
    auto trash_card = addToDeck(P1, 1);
    state.player(P1).main_deck.clear();
    state.getObject(trash_card).zone = ZoneType::Trash;
    state.player(P1).trash.push_back(trash_card);
    auto deck_card = addToDeck(P1, 1);
    ASSERT_EQ(state.player(P1).main_deck.size(), 1u);

    state.player(P1).score = 3;
    state.player(P2).score = state.mode.victory_score - 1;  // one short

    EffectExecutor exec(state, events, card_db);
    exec.burnCards(P1, 3);

    EXPECT_TRUE(state.game_over);
    EXPECT_EQ(state.winner, P2);
    EXPECT_EQ(state.player(P2).score, state.mode.victory_score);

    // Burn-out recycled BOTH cards (the first burn's card + the pre-existing
    // trash card) into the deck; the loop returned on game_over before
    // burning anything from that reshuffled deck.
    EXPECT_TRUE(state.player(P1).trash.empty());
    EXPECT_EQ(state.player(P1).main_deck.size(), 2u);
    EXPECT_TRUE(inDeck(P1, deck_card));
    EXPECT_TRUE(inDeck(P1, trash_card));
}

// ─── Task 7, Step 1 — revealAndChoose(rest=Trash): non-chosen go to trash ────

TEST_F(KennenCardsTest, RevealAndChoose_TrashRest_NonChosenGoToTrashInRevealedOrder) {
    // Deck reads [bottom=A, middle=B, top=C]. Revealed order (top to
    // bottom, as revealAndChoose pops back()) is [C, B, A].
    auto a = addToDeck(P1, 1);
    auto b = addToDeck(P1, 1);
    auto c = addToDeck(P1, 1);
    ASSERT_EQ(state.player(P1).main_deck.back(), c);

    EffectExecutor exec(state, events, card_db);
    int cards_drawn_events = 0;
    int last_drawn_count = 0;
    events.on_cards_drawn.connect([&](const CardsDrawnEvent& e) {
        ++cards_drawn_events;
        last_drawn_count = e.count;
    });

    // revealAndChoose queries the agent once PER revealed card (draw vs
    // skip), in revealed order [C, B, A]. Choosing "index 1" means: skip
    // C (call 0), draw B (call 1), skip A (call 2).
    int call = 0;
    exec.setAgentQuery([&](PlayerId, const std::vector<Intent>& choices) {
        // choices = {draw_it, skip_it}.
        Intent picked = (call == 1) ? choices[0] : choices[1];
        ++call;
        return picked;
    });

    auto chosen = exec.revealAndChoose(P1, 3, EffectExecutor::RestDestination::Trash);

    ASSERT_EQ(chosen.size(), 1u);
    EXPECT_EQ(chosen[0], b);
    EXPECT_TRUE(inHand(P1, b));

    // Non-chosen cards go to trash in their REVEALED order: [C, A].
    ASSERT_EQ(state.player(P1).trash.size(), 2u);
    EXPECT_EQ(state.player(P1).trash[0], c);
    EXPECT_EQ(state.player(P1).trash[1], a);
    EXPECT_EQ(state.getObject(c).zone, ZoneType::Trash);
    EXPECT_FALSE(state.getObject(c).location.has_value());
    EXPECT_EQ(state.getObject(a).zone, ZoneType::Trash);
    EXPECT_FALSE(state.getObject(a).location.has_value());

    // Deck is fully consumed by the reveal.
    EXPECT_EQ(deckSize(P1), 0);

    // The chosen card is a DRAW: draws_this_turn bumped, one CardsDrawnEvent.
    EXPECT_EQ(state.player(P1).draws_this_turn, 1);
    EXPECT_EQ(cards_drawn_events, 1);
    EXPECT_EQ(last_drawn_count, 1);
}

// ─── Task 7 regression — default Recycle behaviour is untouched ─────────────

TEST_F(KennenCardsTest, RevealAndChoose_DefaultRest_StillRecyclesToBottom) {
    auto a = addToDeck(P1, 1);
    auto b = addToDeck(P1, 1);
    ASSERT_EQ(state.player(P1).main_deck.back(), b);

    EffectExecutor exec(state, events, card_db);
    // Always skip -> both cards recycled to the bottom, none chosen.
    exec.setAgentQuery([](PlayerId, const std::vector<Intent>& choices) {
        return choices[1];  // skip_it
    });

    auto chosen = exec.revealAndChoose(P1, 2);  // default RestDestination::Recycle

    EXPECT_TRUE(chosen.empty());
    EXPECT_TRUE(state.player(P1).trash.empty());
    EXPECT_EQ(deckSize(P1), 2);
    EXPECT_TRUE(inDeck(P1, a));
    EXPECT_TRUE(inDeck(P1, b));
}

// ═══════════════════════════════════════════════════════════════════════════
// Task 8 — 789 Kennen, Storm of Shuriken
// ═══════════════════════════════════════════════════════════════════════════

// ─── Card-level wiring: WhenYouPlayMe -> burnCards(controller, 2) ──────────
// (The helper itself is fully covered by #13/#14 above; this is the "one
// small case the CARD wires it" the task calls for.)

TEST_F(KennenCardsTest, Kennen_PlayTrigger_WiresBurnTwo) {
    auto bottom = addToDeck(P1, 1);
    auto top = addToDeck(P1, 1);
    auto kennen_id = addUnit(P1, kKennenStormOfShuriken, 4);

    EffectExecutor exec(state, events, card_db, &card_registry);
    fireTriggerAs(kKennenStormOfShuriken, P1, kennen_id,
                   TriggerType::WhenYouPlayMe, exec);

    EXPECT_EQ(trashSize(P1), 2);
    EXPECT_TRUE(inTrash(P1, top));
    EXPECT_TRUE(inTrash(P1, bottom));
    EXPECT_EQ(deckSize(P1), 0);
}

// ─── Test #15 — conquer with two spells in trash, agent chooses the second ─

TEST_F(KennenCardsTest, Kennen_Conquer_TwoSpellsInTrash_AgentChoosesSecond_GrantsItsFlow) {
    auto spell_a = state.createObject();
    {
        auto& o = state.getObject(spell_a);
        o.owner = P1; o.controller = P1;
        o.card_type = CardType::Spell;
        o.card_def_id = kTheHarrowingSpell;
        o.name = card_db.get(kTheHarrowingSpell).name;
        o.domains = card_db.get(kTheHarrowingSpell).domains;
        o.zone = ZoneType::Trash;
    }
    state.player(P1).trash.push_back(spell_a);

    auto spell_b = state.createObject();
    {
        auto& o = state.getObject(spell_b);
        o.owner = P1; o.controller = P1;
        o.card_type = CardType::Spell;
        o.card_def_id = kRideTheWindSpell;
        o.name = card_db.get(kRideTheWindSpell).name;
        o.domains = card_db.get(kRideTheWindSpell).domains;
        o.zone = ZoneType::Trash;
    }
    state.player(P1).trash.push_back(spell_b);

    auto kennen_id = addUnit(P1, kKennenStormOfShuriken, 4);
    state.turn.turn_number = 3;

    EffectExecutor exec(state, events, card_db, &card_registry);
    driveResumableTrigger(
        kKennenStormOfShuriken, P1, kennen_id,
        [&](const std::vector<Intent>& legal) {
            for (auto& i : legal)
                if (!i.chosen_objects.empty() && i.chosen_objects[0] == spell_b)
                    return i;
            ADD_FAILURE() << "spell_b (Ride the Wind) must be one of the "
                             "offered choices";
            return legal.front();
        },
        exec, TriggerType::WhenIConquer);

    ASSERT_TRUE(state.objectExists(spell_b));
    ASSERT_TRUE(state.getObject(spell_b).granted_flow.has_value())
        << "the chosen spell must be granted Flow this turn";
    const auto& gf = *state.getObject(spell_b).granted_flow;
    const auto& def_b = card_db.get(kRideTheWindSpell);
    EXPECT_EQ(gf.energy, def_b.energy_cost);
    EXPECT_EQ(gf.power, def_b.power_cost);
    EXPECT_EQ(gf.power_domain, def_b.domains.front());
    EXPECT_FALSE(gf.any_domain);
    EXPECT_EQ(gf.valid_on_turn, 3);

    EXPECT_FALSE(state.getObject(spell_a).granted_flow.has_value())
        << "the NON-chosen spell must not be granted Flow";
}

// ─── Test #16 — conquer with no spell in trash changes nothing ────────────

TEST_F(KennenCardsTest, Kennen_Conquer_NoSpellInTrash_ChangesNothing) {
    // A non-spell in trash (a unit) must not be treated as an eligible
    // target — it stays untouched and no choice is offered.
    auto unit_in_trash = state.createObject();
    {
        auto& o = state.getObject(unit_in_trash);
        o.owner = P1; o.controller = P1;
        o.card_type = CardType::Unit;
        o.name = "Trashed Test Unit";
        o.zone = ZoneType::Trash;
    }
    state.player(P1).trash.push_back(unit_in_trash);

    auto kennen_id = addUnit(P1, kKennenStormOfShuriken, 4);

    EffectExecutor exec(state, events, card_db, &card_registry);
    fireTriggerAs(kKennenStormOfShuriken, P1, kennen_id,
                   TriggerType::WhenIConquer, exec);

    EXPECT_FALSE(state.getObject(unit_in_trash).granted_flow.has_value());
    ASSERT_EQ(trashSize(P1), 1);
    EXPECT_TRUE(inTrash(P1, unit_in_trash));
}

// ═══════════════════════════════════════════════════════════════════════════
// Task 8 — 788 Heart of the Tempest
// ═══════════════════════════════════════════════════════════════════════════

// ─── Test #17 — empowers on trash play; action gives Assault 2 that expires

TEST_F(KennenCardsTest, HeartOfTheTempest_EmpowersOnTrashPlay_ActionGivesAssaultThatExpires) {
    // Real trash-replay path (The Harrowing, id 198) driven end-to-end
    // through the engine — mirrors test_play_from_non_hand.cpp test #4 —
    // so the resulting WhenYouPlayFromNonHand trigger (queued onto the
    // chain by TriggerManager::onCardPlayed) is actually drained by the
    // same runChain() call executePlaySpell makes, instead of dangling
    // unresolved the way a bare EffectExecutor::playIgnoringCost call
    // (with no enclosing intent) would leave it.
    constexpr CardDefId kTheHarrowingSpell2 = 198;  // 6E + 2 Chaos P

    GameEngine engine(card_db, events, card_registry);
    FirstChoiceAgent agent1, agent2;
    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    auto& s = engine.mutableState();
    s.mode = ModeOfPlay{};
    s.players[0].id = P1;
    s.players[1].id = P2;
    s.turn.turn_player = P1;
    s.turn.turn_number = 5;
    s.turn.phase = TurnPhase::MainPhase;
    s.turn.ns_state = NeutralShowdownState::Neutral;
    s.turn.oc_state = OpenClosedState::Open;
    BattlefieldState b0; b0.id = 0; s.battlefields.push_back(b0);
    BattlefieldState b1; b1.id = 1; s.battlefields.push_back(b1);

    auto legend_id = s.createObject();
    {
        auto& leg = s.getObject(legend_id);
        leg.owner = P1; leg.controller = P1;
        leg.card_def_id = kHeartOfTheTempest;
        leg.name = "Heart of the Tempest";
        leg.card_type = CardType::Legend;
        leg.zone = ZoneType::LegendZone;
    }
    s.player(P1).legend_zone = legend_id;
    ASSERT_FALSE(s.getObject(legend_id).is_empowered);

    // A unit sitting in P1's trash — The Harrowing's target, replayed via
    // playIgnoringCost. That replay's CardPlayedEvent (play_source=Trash)
    // is what fires WhenYouPlayFromNonHand on the legend.
    auto unit_in_trash = s.createObject();
    {
        auto& u = s.getObject(unit_in_trash);
        u.owner = P1; u.controller = P1;
        u.card_type = CardType::Unit;
        u.name = "Trashed Test Unit";
        u.base_might = 1; u.current_might = 1;
        u.zone = ZoneType::Trash;
    }
    s.player(P1).trash.push_back(unit_in_trash);

    // 10 ready Chaos runes — affords The Harrowing (6E + 2 Chaos P). Built
    // directly on `s` (engine.mutableState()) — the fixture's addRune/
    // addToHand operate on the FIXTURE's separate `state` member, not the
    // engine's own state, so this mirrors this file's Kennen action test
    // above and test_play_from_non_hand.cpp's local helpers.
    for (int i = 0; i < 10; ++i) {
        auto rid = s.createObject();
        auto& r = s.getObject(rid);
        r.owner = P1; r.controller = P1;
        r.card_type = CardType::Rune;
        r.name = "Test Rune";
        r.domains = {Domain::Chaos};
        r.zone = ZoneType::Base;
        r.location = BaseLocation{P1};
        r.is_exhausted = false;
    }
    auto harrowing_id = s.createObject();
    {
        auto& h = s.getObject(harrowing_id);
        h.owner = P1; h.controller = P1;
        h.card_def_id = kTheHarrowingSpell2;
        const auto& hdef = card_db.get(kTheHarrowingSpell2);
        h.name = hdef.name;
        h.card_type = hdef.card_type;
        h.domains = hdef.domains;
        h.zone = ZoneType::Hand;
    }
    s.player(P1).hand.push_back(harrowing_id);

    auto actions = engine.generateLegalActions();
    Intent play;
    bool found_play = false;
    for (auto& a : actions) {
        if (a.type == IntentType::PlayCard && a.card == harrowing_id) {
            play = a; found_play = true; break;
        }
    }
    ASSERT_TRUE(found_play)
        << "The Harrowing must be a legal hand play with 10 ready Chaos runes.";
    engine.testHook_executeIntent(play);

    ASSERT_TRUE(s.objectExists(legend_id));
    EXPECT_TRUE(s.getObject(legend_id).is_empowered)
        << "Heart of the Tempest must become empowered when a card is "
           "played from anywhere other than hand.";

    // The single legal unit target for the action — FirstChoiceAgent will
    // pick it deterministically.
    auto target_unit = s.createObject();
    {
        auto& tu = s.getObject(target_unit);
        tu.owner = P1; tu.controller = P1;
        tu.card_type = CardType::Unit;
        tu.name = "Assault Target";
        tu.base_might = 2; tu.current_might = 2;
        tu.zone = ZoneType::Base;
        tu.location = BaseLocation{P1};
    }

    auto activate_actions = engine.generateLegalActions();
    Intent activate;
    bool found = false;
    for (auto& a : activate_actions) {
        if (a.type == IntentType::ActivateAbility && a.ability_source == legend_id) {
            activate = a;
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found)
        << "Heart of the Tempest's [Action] must be offered while empowered "
           "and ready, with a legal unit on the board.";

    engine.testHook_executeIntent(activate);

    EXPECT_FALSE(s.getObject(legend_id).is_empowered)
        << "Activating the action must disempower Heart of the Tempest.";
    EXPECT_TRUE(s.getObject(legend_id).is_exhausted)
        << "Activating the action must also exhaust Heart of the Tempest.";

    ASSERT_TRUE(s.objectExists(target_unit));
    auto& tu = s.getObject(target_unit);
    EXPECT_TRUE(tu.keywords.has(Keyword::Assault));
    EXPECT_EQ(tu.assault_value, 2);
    EXPECT_EQ(tu.temp_assault_value, 2);

    // Expiration: GameEngine::expireTemporaryKeywords is the exact pure
    // helper the real Expiration Step runs per object — exposed publicly
    // so card tests can verify the this-turn grant is revoked without
    // spinning up the full turn loop (per its doc comment).
    GameEngine::expireTemporaryKeywords(tu, card_db);
    EXPECT_FALSE(tu.keywords.has(Keyword::Assault))
        << "Assault 2 must expire at the turn's Expiration Step.";
}

}  // namespace
}  // namespace riftbound::test
