/// @file test_kennen_cards.cpp
/// Kennen (Heart of the Tempest) deck support — Tasks 6-8.
///
/// Task 6: Burn N (CR 440) + shared burn-out (CR 431.2/431.3), driven
/// directly through EffectExecutor::burnCards.
/// Task 7: revealAndChoose's rest-destination parameter (Recycle | Trash),
/// which Lightning Rush (card tests land in Task 8) needs set to Trash.

#include "tests/cards/card_test_fixture.h"

#include <gtest/gtest.h>

namespace riftbound::test {
namespace {

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

}  // namespace
}  // namespace riftbound::test
