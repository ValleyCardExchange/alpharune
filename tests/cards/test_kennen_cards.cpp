/// @file test_kennen_cards.cpp
/// Kennen (Heart of the Tempest) deck support — Tasks 6-8.
///
/// Task 6: Burn N (CR 440) + shared burn-out (CR 431.2/431.3), driven
/// directly through EffectExecutor::burnCards.

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

}  // namespace
}  // namespace riftbound::test
