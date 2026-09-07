/// @file test_agent_spec.cpp
/// Unit tests for agent spec parsing (Task 12.2, spec test 10).
///
/// Spec: docs/superpowers/specs/2026-09-07-corpus-evaluator-design.md
///
/// `AgentSpec` / `parseAgentSpec` were extracted out of `src/main.cpp`
/// (which cannot be linked from the test binary) into
/// `src/agents/agent_spec.{h,cpp}` in riftbound_core so the `eval=`
/// option is testable. The tests below therefore fall in two groups:
///
///   • CHARACTERIZATION of the behaviour that existed in main.cpp before
///     the extraction — these must not change, or the extraction was not
///     behaviour-preserving.
///   • The new `eval=score|corpus` option.
///
/// What is NOT covered here: that `buildAgent` hands the parsed
/// EvaluatorKind to MctsAgent and that MctsAgent installs the matching
/// OpenSpiel Evaluator. `buildAgent` lives in main.cpp and MctsAgent is
/// compiled into the `riftbound` executable (it needs the OpenSpiel
/// registrar translation units), so neither is linkable from
/// riftbound_tests. That half of spec test 10 is covered by the A/B run
/// in docs/superpowers/smoke/2026-09-08-corpus-ab.md, where the two
/// evaluators produce visibly different play.

#include "agents/agent_spec.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace riftbound {
namespace {

// ── Characterization: kinds ────────────────────────────────────────────────

TEST(AgentSpecParse, BareKindsParse) {
    for (const std::string kind : {"random", "human", "mcts", "ismcts"}) {
        auto spec = parseAgentSpec(kind);
        EXPECT_EQ(spec.kind, kind);
        EXPECT_EQ(spec.raw, kind);
    }
}

TEST(AgentSpecParse, UnknownKindThrows) {
    EXPECT_THROW(parseAgentSpec("greedy"), std::runtime_error);
    EXPECT_THROW(parseAgentSpec("greedy:sims=50"), std::runtime_error);
    EXPECT_THROW(parseAgentSpec(""), std::runtime_error);
}

// ── Characterization: sims ─────────────────────────────────────────────────

TEST(AgentSpecParse, SimsIsParsedForMctsFamily) {
    EXPECT_EQ(parseAgentSpec("mcts:sims=50").sims, 50);
    EXPECT_EQ(parseAgentSpec("ismcts:sims=20").sims, 20);
}

TEST(AgentSpecParse, MctsWithAnEmptyOrZeroSimsThrows) {
    EXPECT_THROW(parseAgentSpec("mcts:sims=0"), std::runtime_error);
    EXPECT_THROW(parseAgentSpec("mcts:sims=-1"), std::runtime_error);
    EXPECT_THROW(parseAgentSpec("ismcts:sims=0"), std::runtime_error);
    // A colon with no recognised sims key leaves sims at its 0 default,
    // which trips the same guard.
    EXPECT_THROW(parseAgentSpec("mcts:"), std::runtime_error);
}

TEST(AgentSpecParse, BareMctsWithoutAColonIsAcceptedWithZeroSims) {
    // PRE-EXISTING behaviour carried over verbatim from main.cpp: the
    // sims>0 guard sits inside the "has a colon" branch, so `mcts` with
    // no options parses to sims=0 instead of throwing. Pinned here so
    // the extraction is provably behaviour-preserving; tightening it is
    // a separate change with its own blast radius (it would reject
    // command lines that exist today).
    auto spec = parseAgentSpec("mcts");
    EXPECT_EQ(spec.kind, "mcts");
    EXPECT_EQ(spec.sims, 0);
}

TEST(AgentSpecParse, UnknownKeysAreIgnored) {
    // Also pre-existing: unrecognised key=value pairs are skipped rather
    // than rejected.
    auto spec = parseAgentSpec("mcts:sims=50,rollouts=3");
    EXPECT_EQ(spec.sims, 50);
}

TEST(AgentSpecParse, RawIsThePristineInputString) {
    EXPECT_EQ(parseAgentSpec("mcts:sims=50,eval=corpus").raw,
              "mcts:sims=50,eval=corpus");
}

// ── New: eval=score|corpus (spec test 10) ──────────────────────────────────

TEST(AgentSpecParse, EvalDefaultsToScore) {
    EXPECT_EQ(parseAgentSpec("mcts:sims=50").eval, EvaluatorKind::Score);
    EXPECT_EQ(parseAgentSpec("ismcts:sims=50").eval, EvaluatorKind::Score);
    EXPECT_EQ(parseAgentSpec("random").eval, EvaluatorKind::Score);
}

TEST(AgentSpecParse, EvalCorpusSelectsTheCorpusEvaluator) {
    auto spec = parseAgentSpec("mcts:sims=50,eval=corpus");
    EXPECT_EQ(spec.kind, "mcts");
    EXPECT_EQ(spec.sims, 50);
    EXPECT_EQ(spec.eval, EvaluatorKind::Corpus);
}

TEST(AgentSpecParse, EvalScoreIsAcceptedExplicitly) {
    EXPECT_EQ(parseAgentSpec("mcts:sims=50,eval=score").eval,
              EvaluatorKind::Score);
}

TEST(AgentSpecParse, EvalOptionOrderDoesNotMatter) {
    auto spec = parseAgentSpec("mcts:eval=corpus,sims=50");
    EXPECT_EQ(spec.sims, 50);
    EXPECT_EQ(spec.eval, EvaluatorKind::Corpus);
}

TEST(AgentSpecParse, EvalWorksForIsmctsToo) {
    EXPECT_EQ(parseAgentSpec("ismcts:sims=20,eval=corpus").eval,
              EvaluatorKind::Corpus);
}

TEST(AgentSpecParse, UnknownEvalValueThrows) {
    EXPECT_THROW(parseAgentSpec("mcts:sims=50,eval=bogus"),
                 std::runtime_error);
    EXPECT_THROW(parseAgentSpec("mcts:sims=50,eval="), std::runtime_error);
    EXPECT_THROW(parseAgentSpec("mcts:sims=50,eval=CORPUS"),
                 std::runtime_error);
}

TEST(AgentSpecParse, UnknownEvalValueThrowsRegardlessOfAgentKind) {
    // The value is validated wherever it appears — an `eval=` typo on a
    // random seat is still a typo, and silently ignoring it would make
    // an A/B batch quietly measure the wrong thing.
    EXPECT_THROW(parseAgentSpec("random:eval=bogus"), std::runtime_error);
}

TEST(AgentSpecParse, EvalErrorMentionsTheOfferedValues) {
    try {
        parseAgentSpec("mcts:sims=50,eval=bogus");
        FAIL() << "expected parseAgentSpec to throw";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("eval"), std::string::npos) << msg;
        EXPECT_NE(msg.find("score"), std::string::npos) << msg;
        EXPECT_NE(msg.find("corpus"), std::string::npos) << msg;
    }
}

TEST(EvaluatorKindToString, RoundTripsTheSpecTokens) {
    EXPECT_STREQ(toString(EvaluatorKind::Score), "score");
    EXPECT_STREQ(toString(EvaluatorKind::Corpus), "corpus");
}

}  // namespace
}  // namespace riftbound
