#include <gtest/gtest.h>
#include "core/intent.h"
#include "core/types.h"

#include <cstdint>
#include <set>
#include <string>

using namespace riftbound;

TEST(TypesTest, PlayerIdOpponent) {
    EXPECT_EQ(opponent(PlayerId::Player1), PlayerId::Player2);
    EXPECT_EQ(opponent(PlayerId::Player2), PlayerId::Player1);
}

TEST(TypesTest, PlayerIndex) {
    EXPECT_EQ(playerIndex(PlayerId::Player1), 0);
    EXPECT_EQ(playerIndex(PlayerId::Player2), 1);
}

TEST(TypesTest, KeywordSetBasic) {
    KeywordSet kw;
    EXPECT_FALSE(kw.has(Keyword::Accelerate));
    EXPECT_FALSE(kw.has(Keyword::Ganking));

    kw.set(Keyword::Accelerate);
    EXPECT_TRUE(kw.has(Keyword::Accelerate));
    EXPECT_FALSE(kw.has(Keyword::Ganking));

    kw.set(Keyword::Ganking);
    EXPECT_TRUE(kw.has(Keyword::Accelerate));
    EXPECT_TRUE(kw.has(Keyword::Ganking));

    kw.clear(Keyword::Accelerate);
    EXPECT_FALSE(kw.has(Keyword::Accelerate));
    EXPECT_TRUE(kw.has(Keyword::Ganking));
}

TEST(TypesTest, KeywordSetBitOps) {
    KeywordSet a, b;
    a.set(Keyword::Assault);
    a.set(Keyword::Shield);
    b.set(Keyword::Shield);
    b.set(Keyword::Tank);

    auto combined = a | b;
    EXPECT_TRUE(combined.has(Keyword::Assault));
    EXPECT_TRUE(combined.has(Keyword::Shield));
    EXPECT_TRUE(combined.has(Keyword::Tank));

    auto intersect = a & b;
    EXPECT_FALSE(intersect.has(Keyword::Assault));
    EXPECT_TRUE(intersect.has(Keyword::Shield));
    EXPECT_FALSE(intersect.has(Keyword::Tank));
}

TEST(TypesTest, KeywordSetReset) {
    KeywordSet kw;
    kw.set(Keyword::Accelerate);
    kw.set(Keyword::Ganking);
    kw.set(Keyword::Shield);
    kw.reset();
    EXPECT_EQ(kw.bits, 0u);
}

TEST(TypesTest, LocationIdVariant) {
    LocationId base = BaseLocation{PlayerId::Player1};
    LocationId bf = BattlefieldLocation{42};

    EXPECT_TRUE(std::holds_alternative<BaseLocation>(base));
    EXPECT_TRUE(std::holds_alternative<BattlefieldLocation>(bf));

    EXPECT_EQ(std::get<BaseLocation>(base).player, PlayerId::Player1);
    EXPECT_EQ(std::get<BattlefieldLocation>(bf).id, 42u);
}

TEST(TypesTest, LocationIdEquality) {
    LocationId a = BaseLocation{PlayerId::Player1};
    LocationId b = BaseLocation{PlayerId::Player1};
    LocationId c = BaseLocation{PlayerId::Player2};
    LocationId d = BattlefieldLocation{1};

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
}

TEST(TypesTest, ToStringCoverage) {
    // Ensure all toString functions return non-null
    EXPECT_STREQ(toString(CardType::Unit), "Unit");
    EXPECT_STREQ(toString(CardType::Legend), "Legend");
    EXPECT_STREQ(toString(SuperType::Champion), "Champion");
    EXPECT_STREQ(toString(Domain::Fury), "Fury");
    EXPECT_STREQ(toString(Domain::Order), "Order");
    EXPECT_STREQ(toString(TurnPhase::MainPhase), "MainPhase");
    EXPECT_STREQ(toString(PlayerId::Player1), "P1");
    EXPECT_STREQ(toString(IntentType::StandardMove), "StandardMove");
    EXPECT_STREQ(toString(CombatDesignation::Attacker), "Attacker");
    EXPECT_STREQ(toString(ScoreMethod::Hold), "Hold");
}

TEST(TypesTest, ModeOfPlayDefaults) {
    ModeOfPlay mode;
    EXPECT_EQ(mode.num_players, 2);
    EXPECT_EQ(mode.victory_score, 8);
    EXPECT_EQ(mode.battlefield_count, 2);
    EXPECT_TRUE(mode.second_player_extra_channel);
}

// Test #27 (Kennen deck): every Keyword below Count has a non-empty, unique
// toString, and Flow is spelled "Flow".
TEST(TypesTest, KeywordToStringCompleteAndUnique) {
    std::set<std::string> seen;
    for (uint32_t i = 0; i < static_cast<uint32_t>(Keyword::Count); ++i) {
        const char* s = toString(static_cast<Keyword>(i));
        ASSERT_NE(s, nullptr) << "keyword bit " << i;
        std::string name(s);
        EXPECT_FALSE(name.empty()) << "keyword bit " << i;
        EXPECT_NE(name, "Unknown") << "keyword bit " << i << " has no toString case";
        EXPECT_TRUE(seen.insert(name).second)
            << "duplicate keyword toString \"" << name << "\" at bit " << i;
    }
    EXPECT_EQ(seen.size(), static_cast<size_t>(Keyword::Count));
    EXPECT_STREQ(toString(Keyword::Flow), "Flow");
}

// ─── Intent::operator== is a FULL structural comparison ────────────────────
//
// Serializers (and the OpenSpiel bridge) locate a chosen action by its index
// in the legal-action list, so any field that distinguishes two otherwise
// identical offers has to participate in equality — otherwise the lookup
// picks the earlier twin and the recorded decision is a different play from
// the one that was made. `flow_source` was fixed on this branch for exactly
// that reason; `use_alt_play_cost` and `granted_ability_def` carry the same
// hazard (an alt-cost play vs the printed-cost play of the same card; an
// aura-granted ability vs the bearer's own ability at the same index).
TEST(TypesTest, IntentEqualityDistinguishesAltCostAndGrantedAbility) {
    Intent base;
    base.type = IntentType::PlayCard;
    base.player = PlayerId::Player1;
    base.card = 7;

    Intent alt = base;
    alt.use_alt_play_cost = true;
    EXPECT_FALSE(base == alt)
        << "Two plays of the same card that differ only in whether they pay "
           "the alternate cost are different actions.";

    Intent granted;
    granted.type = IntentType::ActivateAbility;
    granted.player = PlayerId::Player1;
    granted.ability_source = 7;
    granted.ability_index = 0;
    Intent own = granted;
    granted.granted_ability_def = 462;
    EXPECT_FALSE(own == granted)
        << "An aura-granted ability and the bearer's own ability at the same "
           "index are different actions.";

    Intent same = base;
    EXPECT_TRUE(base == same) << "sanity: identical intents still compare equal";
}
