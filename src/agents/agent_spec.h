#pragma once
/// @file agent_spec.h
/// Parsing for the `--agent1` / `--agent2` spec strings.
///
///   random | human | mcts:sims=N[,eval=score|corpus]
///                  | ismcts:sims=N[,eval=score|corpus]
///
/// This lived inline in `src/main.cpp` until Task 12.2. main.cpp is not
/// linkable from `riftbound_tests`, so the parser was unreachable by
/// unit tests and the `eval=` option could not be proven; moving it into
/// riftbound_core makes it testable without changing any behaviour. The
/// header stays free of OpenSpiel and of anything in `agents/` that
/// needs it, so riftbound_core keeps its dependency shape.

#include <cstdint>
#include <string>

namespace riftbound {

/// Which position evaluator an MCTS-family agent installs.
///
/// `Score`  — the historical score-difference-only heuristic.
/// `Corpus` — the six-term corpus heuristic (see corpus_evaluator.h).
///
/// `Score` is the default so every command line and test that predates
/// Task 12 behaves exactly as it did.
enum class EvaluatorKind : uint8_t { Score, Corpus };

/// The spec token for `k` — the same string the command line accepts.
const char* toString(EvaluatorKind k);

struct AgentSpec {
    std::string raw;                             // original spec string
    std::string kind;                            // random | human | mcts | ismcts
    int sims = 0;                                // populated for mcts/ismcts
    EvaluatorKind eval = EvaluatorKind::Score;   // mcts/ismcts evaluator choice
};

/// Parse an agent spec string.
/// @throws std::runtime_error on an unknown kind, an mcts/ismcts spec
///         carrying options but no positive `sims`, or an unrecognised
///         `eval` value.
AgentSpec parseAgentSpec(const std::string& s);

}  // namespace riftbound
