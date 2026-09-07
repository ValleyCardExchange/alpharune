/// @file test_corpus_evaluator.cpp
/// Unit tests for the corpus heuristic evaluator (Task 12.1).
///
/// Spec: docs/superpowers/specs/2026-09-07-corpus-evaluator-design.md
/// ("Tests" section, cases 1–9). Each case pins a single term's numeric
/// contribution so a weight change cannot slip through unnoticed; the
/// expected values are written as the spec's literal arithmetic
/// (e.g. 0.5 / 8) rather than as the header's constants, so the tests
/// fail if the constants drift from the ratified design.
///
/// The evaluator is a pure function of GameState — no engine, no CardDB,
/// no OpenSpiel. States are built by hand here (the card fixture in
/// tests/cards/card_test_fixture.h is the precedent for the shape, but
/// it loads the whole card registry, which these tests do not need).

#include "agents/corpus_evaluator.h"

#include "core/game_object.h"
#include "core/game_state.h"
#include "core/types.h"

#include <gtest/gtest.h>

#include <utility>

namespace riftbound {
namespace {

constexpr PlayerId P1 = PlayerId::Player1;
constexpr PlayerId P2 = PlayerId::Player2;

/// Minimal hand-built GameState. Only the fields the evaluator reads are
/// populated; everything else keeps its struct default.
class CorpusStateBuilder {
public:
    explicit CorpusStateBuilder(int victory_score = 8) {
        s.mode.victory_score = victory_score;
        s.players[0].id = P1;
        s.players[1].id = P2;
        s.turn.turn_number = 3;   // a non-zero "this turn" for flow grants
        s.turn.turn_player = P1;
    }

    GameState& state() { return s; }

    void setScore(PlayerId p, int score) { s.player(p).score = score; }

    BattlefieldId addBattlefield() {
        BattlefieldState bf;
        bf.id = static_cast<BattlefieldId>(s.battlefields.size());
        s.battlefields.push_back(bf);
        return bf.id;
    }

    /// Controlled battlefield, optionally contested by the other player.
    BattlefieldId addBattlefield(PlayerId controller,
                                 PlayerId contested_by = PlayerId::None) {
        auto id = addBattlefield();
        auto& bf = s.battlefields.back();
        if (controller != PlayerId::None) bf.controller = controller;
        if (contested_by != PlayerId::None) {
            bf.is_contested = true;
            bf.contested_by = contested_by;
        }
        return id;
    }

    /// A unit on the board. `at_bf < 0` puts it in the controller's base.
    GameObjectId addUnit(PlayerId controller, int at_bf = -1) {
        auto id = s.createObject();
        auto& o = s.getObject(id);
        o.owner = controller;
        o.controller = controller;
        o.card_type = CardType::Unit;
        o.name = "TestUnit";
        o.base_might = 1;
        o.current_might = 1;
        if (at_bf >= 0) {
            o.zone = ZoneType::BattlefieldZone;
            o.location = BattlefieldLocation{static_cast<BattlefieldId>(at_bf)};
        } else {
            o.zone = ZoneType::Base;
            o.location = BaseLocation{controller};
        }
        return id;
    }

    /// A spell in hand. `kw` is the keyword to print on it (Count = none).
    GameObjectId addSpellInHand(PlayerId owner, Keyword kw = Keyword::Count) {
        auto id = makeSpell(owner);
        auto& o = s.getObject(id);
        if (kw != Keyword::Count) o.keywords.set(kw);
        o.zone = ZoneType::Hand;
        s.player(owner).hand.push_back(id);
        return id;
    }

    /// A unit (not a spell) in hand — the held-interaction term must skip it.
    GameObjectId addUnitInHand(PlayerId owner, Keyword kw = Keyword::Count) {
        auto id = s.createObject();
        auto& o = s.getObject(id);
        o.owner = owner;
        o.controller = owner;
        o.card_type = CardType::Unit;
        o.name = "TestUnitCard";
        if (kw != Keyword::Count) o.keywords.set(kw);
        o.zone = ZoneType::Hand;
        s.player(owner).hand.push_back(id);
        return id;
    }

    /// A spell in trash. `printed_flow` prints Keyword::Flow;
    /// `granted_flow_turn >= 0` stamps a granted flow valid on that turn.
    GameObjectId addSpellInTrash(PlayerId owner, bool printed_flow = false,
                                 int granted_flow_turn = -1) {
        auto id = makeSpell(owner);
        auto& o = s.getObject(id);
        if (printed_flow) o.keywords.set(Keyword::Flow);
        if (granted_flow_turn >= 0) {
            GameObject::GrantedFlow g;
            g.energy = 1;
            g.valid_on_turn = granted_flow_turn;
            o.granted_flow = g;
        }
        o.zone = ZoneType::Trash;
        s.player(owner).trash.push_back(id);
        return id;
    }

    GameObjectId setLegend(PlayerId owner, bool empowered) {
        auto id = s.createObject();
        auto& o = s.getObject(id);
        o.owner = owner;
        o.controller = owner;
        o.card_type = CardType::Legend;
        o.name = "TestLegend";
        o.zone = ZoneType::LegendZone;
        o.is_empowered = empowered;
        s.player(owner).legend_zone = id;
        return id;
    }

private:
    GameObjectId makeSpell(PlayerId owner) {
        auto id = s.createObject();
        auto& o = s.getObject(id);
        o.owner = owner;
        o.controller = owner;
        o.card_type = CardType::Spell;
        o.name = "TestSpell";
        return id;
    }

    GameState s;
};

// Every term but the score term is scaled by 1 / victory_score.
constexpr double kV = 8.0;

// ── Spec test 1: empty symmetric state ──────────────────────────────────────

TEST(CorpusEvaluator, EmptySymmetricStateIsZero) {
    CorpusStateBuilder b;
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
    EXPECT_DOUBLE_EQ(v.second, 0.0);
}

TEST(CorpusEvaluator, SymmetricNonEmptyStateIsZero) {
    CorpusStateBuilder b;
    for (auto p : {P1, P2}) {
        b.addUnit(p);
        b.addUnit(p);
        b.addSpellInHand(p, Keyword::Reaction);
        b.addSpellInTrash(p, /*printed_flow=*/true);
        b.setLegend(p, /*empowered=*/true);
    }
    b.addBattlefield(P1);
    b.addBattlefield(P2);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
    EXPECT_DOUBLE_EQ(v.second, 0.0);
}

// ── Spec test 2: score difference (weight 1.0) ──────────────────────────────

TEST(CorpusEvaluator, ScoreTermIsTheDifferenceOverVictoryScore) {
    CorpusStateBuilder b;
    b.setScore(P1, 4);
    b.setScore(P2, 0);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.5);
    EXPECT_DOUBLE_EQ(v.second, -0.5);
}

TEST(CorpusEvaluator, ScoreTermIsSignedTowardTheLeader) {
    CorpusStateBuilder b;
    b.setScore(P1, 1);
    b.setScore(P2, 3);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -0.25);
    EXPECT_DOUBLE_EQ(v.second, 0.25);
}

TEST(CorpusEvaluator, ScoreTermRespectsANonDefaultVictoryScore) {
    CorpusStateBuilder b(/*victory_score=*/4);
    b.setScore(P1, 1);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.25);
}

// ── Spec test 3: point tempo (battlefields controlled) ──────────────────────

TEST(CorpusEvaluator, UncontestedBattlefieldIsHalfAPointOfTempo) {
    CorpusStateBuilder b;
    b.addBattlefield(P1);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.5 / kV);
    EXPECT_DOUBLE_EQ(v.second, -0.5 / kV);
}

TEST(CorpusEvaluator, OpponentUncontestedBattlefieldIsNegative) {
    CorpusStateBuilder b;
    b.addBattlefield(P2);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -0.5 / kV);
}

TEST(CorpusEvaluator, ContestedBattlefieldSplitsTheTempoAndNetsZero) {
    CorpusStateBuilder b;
    b.addBattlefield(P1, /*contested_by=*/P2);
    auto v = corpusEvaluate(b.state());
    // +0.25/8 for P1 (half share as holder) and +0.25/8 for P2 (half
    // share as contester) — net zero from P1's perspective.
    EXPECT_DOUBLE_EQ(v.first, 0.0);
    EXPECT_DOUBLE_EQ(v.second, 0.0);
}

TEST(CorpusEvaluator, UncontrolledBattlefieldContributesNothing) {
    CorpusStateBuilder b;
    b.addBattlefield();
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
}

TEST(CorpusEvaluator, BattlefieldTermSumsAcrossBattlefields) {
    CorpusStateBuilder b;
    b.addBattlefield(P1);
    b.addBattlefield(P1);
    b.addBattlefield(P2);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.5 / kV);
}

// ── Spec test 4: board width / retention (diminishing, cap 4) ───────────────

TEST(CorpusEvaluator, BoardWidthDiminishesAndCapsAtFourUnits) {
    const double w = 0.25 / kV;
    const std::pair<int, double> cases[] = {
        {1, 1.0}, {2, 1.75}, {4, 2.5}, {6, 2.5},
    };
    for (const auto& [units, multiplier] : cases) {
        CorpusStateBuilder b;
        for (int i = 0; i < units; ++i) b.addUnit(P1);
        auto v = corpusEvaluate(b.state());
        EXPECT_DOUBLE_EQ(v.first, w * multiplier)
            << "with " << units << " friendly units";
    }
}

TEST(CorpusEvaluator, BoardWidthCountsUnitsAtBattlefieldsToo) {
    CorpusStateBuilder b;
    auto bf = b.addBattlefield();
    b.addUnit(P1, static_cast<int>(bf));
    b.addUnit(P1);  // base
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, (0.25 / kV) * 1.75);
}

TEST(CorpusEvaluator, BoardWidthIsSymmetricForTheOpponent) {
    CorpusStateBuilder b;
    b.addUnit(P1);
    b.addUnit(P2);
    b.addUnit(P2);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, (0.25 / kV) * (1.0 - 1.75));
}

// ── Spec test 5: held interaction (Reaction spells in hand, cap 2) ──────────

TEST(CorpusEvaluator, HeldReactionSpellsCapAtTwo) {
    CorpusStateBuilder b;
    for (int i = 0; i < 3; ++i) b.addSpellInHand(P1, Keyword::Reaction);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 2 * (0.15 / kV));
}

TEST(CorpusEvaluator, OneHeldReactionSpellCountsOnce) {
    CorpusStateBuilder b;
    b.addSpellInHand(P1, Keyword::Reaction);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.15 / kV);
}

TEST(CorpusEvaluator, ActionOnlySpellsInHandCountZero) {
    CorpusStateBuilder b;
    b.addSpellInHand(P1, Keyword::Action);
    b.addSpellInHand(P1);  // vanilla spell
    b.addUnitInHand(P1, Keyword::Reaction);  // not a spell
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
}

TEST(CorpusEvaluator, HeldInteractionIsSymmetric) {
    CorpusStateBuilder b;
    b.addSpellInHand(P2, Keyword::Reaction);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -(0.15 / kV));
}

// ── Spec test 6: live trash resources (Flow, cap 3) ─────────────────────────

TEST(CorpusEvaluator, PrintedFlowSpellInTrashCounts) {
    CorpusStateBuilder b;
    b.addSpellInTrash(P1, /*printed_flow=*/true);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.1 / kV);
}

TEST(CorpusEvaluator, GrantedFlowStampedThisTurnCounts) {
    CorpusStateBuilder b;  // turn_number == 3
    b.addSpellInTrash(P1, /*printed_flow=*/false, /*granted_flow_turn=*/3);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.1 / kV);
}

TEST(CorpusEvaluator, GrantedFlowStampedLastTurnDoesNotCount) {
    CorpusStateBuilder b;  // turn_number == 3
    b.addSpellInTrash(P1, /*printed_flow=*/false, /*granted_flow_turn=*/2);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
}

TEST(CorpusEvaluator, NonFlowSpellInTrashDoesNotCount) {
    CorpusStateBuilder b;
    b.addSpellInTrash(P1);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
}

TEST(CorpusEvaluator, TrashResourcesCapAtThree) {
    CorpusStateBuilder b;
    for (int i = 0; i < 5; ++i) b.addSpellInTrash(P1, /*printed_flow=*/true);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 3 * (0.1 / kV));
}

TEST(CorpusEvaluator, TrashResourcesAreSymmetric) {
    CorpusStateBuilder b;
    b.addSpellInTrash(P2, /*printed_flow=*/true);
    b.addSpellInTrash(P2, /*printed_flow=*/true);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -2 * (0.1 / kV));
}

// ── Spec test 7: empowered legend ───────────────────────────────────────────

TEST(CorpusEvaluator, EmpoweredLegendCounts) {
    CorpusStateBuilder b;
    b.setLegend(P1, /*empowered=*/true);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.1 / kV);
}

TEST(CorpusEvaluator, UnempoweredLegendCountsZero) {
    CorpusStateBuilder b;
    b.setLegend(P1, /*empowered=*/false);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
}

TEST(CorpusEvaluator, OpponentEmpoweredLegendIsNegative) {
    CorpusStateBuilder b;
    b.setLegend(P2, /*empowered=*/true);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -(0.1 / kV));
}

TEST(CorpusEvaluator, MissingLegendZoneIsSafe) {
    CorpusStateBuilder b;
    b.state().player(P1).legend_zone = 999999;  // dangling id
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 0.0);
}

// ── Spec test 8: antisymmetry ───────────────────────────────────────────────

TEST(CorpusEvaluator, ValueIsAntisymmetricAcrossTerms) {
    CorpusStateBuilder b;
    b.setScore(P1, 3);
    b.setScore(P2, 1);
    b.addBattlefield(P1);
    b.addBattlefield(P2, /*contested_by=*/P1);
    b.addUnit(P1);
    b.addUnit(P1);
    b.addUnit(P2);
    b.addSpellInHand(P1, Keyword::Reaction);
    b.addSpellInHand(P2, Keyword::Reaction);
    b.addSpellInHand(P2, Keyword::Reaction);
    b.addSpellInTrash(P1, /*printed_flow=*/true);
    b.addSpellInTrash(P2, /*printed_flow=*/false, /*granted_flow_turn=*/3);
    b.setLegend(P1, /*empowered=*/true);
    b.setLegend(P2, /*empowered=*/false);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -v.second);
    EXPECT_GT(v.first, 0.0);  // P1 is genuinely ahead in this position
}

TEST(CorpusEvaluator, MirroredStateFlipsTheSign) {
    auto build = [](PlayerId me, PlayerId them) {
        CorpusStateBuilder b;
        b.setScore(me, 5);
        b.setScore(them, 2);
        b.addBattlefield(me);
        b.addUnit(me);
        b.addUnit(me);
        b.addUnit(them);
        b.addSpellInHand(me, Keyword::Reaction);
        b.addSpellInTrash(them, /*printed_flow=*/true);
        b.setLegend(me, /*empowered=*/true);
        return corpusEvaluate(b.state());
    };
    auto forward = build(P1, P2);
    auto mirrored = build(P2, P1);
    EXPECT_DOUBLE_EQ(forward.first, -mirrored.first);
    EXPECT_DOUBLE_EQ(forward.second, -mirrored.second);
}

// ── Spec test 9: clamp ──────────────────────────────────────────────────────

TEST(CorpusEvaluator, WildlyLopsidedStateStaysInRange) {
    CorpusStateBuilder b(/*victory_score=*/1);
    b.setScore(P1, 40);
    b.addBattlefield(P1);
    b.addBattlefield(P1);
    b.addBattlefield(P1);
    for (int i = 0; i < 10; ++i) {
        b.addUnit(P1);
        b.addSpellInHand(P1, Keyword::Reaction);
        b.addSpellInTrash(P1, /*printed_flow=*/true);
    }
    b.setLegend(P1, /*empowered=*/true);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, 1.0);
    EXPECT_DOUBLE_EQ(v.second, -1.0);
}

TEST(CorpusEvaluator, WildlyLopsidedStateClampsTheOtherWayToo) {
    CorpusStateBuilder b(/*victory_score=*/1);
    b.setScore(P2, 40);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -1.0);
    EXPECT_DOUBLE_EQ(v.second, 1.0);
}

TEST(CorpusEvaluator, ZeroVictoryScoreDoesNotDivideByZero) {
    CorpusStateBuilder b(/*victory_score=*/0);
    b.setScore(P1, 1);
    auto v = corpusEvaluate(b.state());
    EXPECT_TRUE(v.first >= -1.0 && v.first <= 1.0);
}

// ── Score dominance: the headroom, MEASURED rather than assumed ─────────────
//
// The spec says the score term is "dominant so the bot still chases points".
// Against the ratified weights that is true term-by-term (weight 1.0 vs
// fractions) but NOT of the sum: on the standard 2-battlefield board, every
// proxy term maxed for one side totals
//     2*(0.5/8) + 2.5*(0.25/8) + 2*(0.15/8) + 3*(0.1/8) + (0.1/8)
//   = 0.290625
// which is 2.325 points of score-equivalent at victory_score 8. So a 1- or
// 2-point lead CAN be outvoted by a maximally lopsided board; a 3-point lead
// cannot. These tests pin that number so a later weight change cannot quietly
// widen the gap. Flagged as a concern in the Task 12 report — the spec's
// weights are the owner's to retune, not this session's.

/// Build "P1 has `score` points and nothing else; P2 has every proxy maxed".
CorpusStateBuilder maximallyLopsidedProxies(int p1_score) {
    CorpusStateBuilder b;
    b.setScore(P1, p1_score);
    b.addBattlefield(P2);
    b.addBattlefield(P2);
    for (int i = 0; i < 6; ++i) {
        b.addUnit(P2);
        b.addSpellInHand(P2, Keyword::Reaction);
        b.addSpellInTrash(P2, /*printed_flow=*/true);
    }
    b.setLegend(P2, /*empowered=*/true);
    return b;
}

TEST(CorpusEvaluator, MaxedProxyTermsAreWorthExactlyTwoPointThreeTwoFivePoints) {
    auto b = maximallyLopsidedProxies(/*p1_score=*/0);
    auto v = corpusEvaluate(b.state());
    EXPECT_DOUBLE_EQ(v.first, -0.290625);
    // …expressed in points of score at victory_score 8:
    EXPECT_DOUBLE_EQ(-v.first * kV, 2.325);
}

TEST(CorpusEvaluator, TwoPointLeadDoesNotOutweighAMaximalOpposingBoard) {
    auto b = maximallyLopsidedProxies(/*p1_score=*/2);
    EXPECT_LT(corpusEvaluate(b.state()).first, 0.0);
}

TEST(CorpusEvaluator, ThreePointLeadDoesOutweighAMaximalOpposingBoard) {
    auto b = maximallyLopsidedProxies(/*p1_score=*/3);
    EXPECT_GT(corpusEvaluate(b.state()).first, 0.0);
}

TEST(CorpusEvaluator, ScoreStillDominatesTermByTerm) {
    // One point of score beats any SINGLE proxy term maxed against it,
    // which is the sense in which the score weight is dominant.
    CorpusStateBuilder bf;
    bf.setScore(P1, 1);
    bf.addBattlefield(P2);
    EXPECT_GT(corpusEvaluate(bf.state()).first, 0.0);

    CorpusStateBuilder units;
    units.setScore(P1, 1);
    for (int i = 0; i < 6; ++i) units.addUnit(P2);
    EXPECT_GT(corpusEvaluate(units.state()).first, 0.0);

    CorpusStateBuilder held;
    held.setScore(P1, 1);
    for (int i = 0; i < 6; ++i) held.addSpellInHand(P2, Keyword::Reaction);
    EXPECT_GT(corpusEvaluate(held.state()).first, 0.0);

    CorpusStateBuilder trash;
    trash.setScore(P1, 1);
    for (int i = 0; i < 6; ++i) trash.addSpellInTrash(P2, /*printed_flow=*/true);
    EXPECT_GT(corpusEvaluate(trash.state()).first, 0.0);
}

TEST(CorpusEvaluator, TwoOpposingBattlefieldsExactlyTieOnePointOfScore) {
    // The single most load-bearing consequence of w_bf = 0.5/victory: on
    // the standard 2-battlefield board, "the opponent holds both" reads
    // as EXACTLY one point of score. Deliberate per the spec (a held
    // battlefield ~ half a point of expected tempo) — pinned so the
    // equality is a choice, not an accident.
    CorpusStateBuilder b;
    b.setScore(P1, 1);
    b.addBattlefield(P2);
    b.addBattlefield(P2);
    EXPECT_DOUBLE_EQ(corpusEvaluate(b.state()).first, 0.0);
}

}  // namespace
}  // namespace riftbound
