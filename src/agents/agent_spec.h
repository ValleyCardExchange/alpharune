#pragma once
/// @file agent_spec.h
/// Parsing for the `--agent1` / `--agent2` spec strings.
///
///   random | human | mcts:sims=N[,eval=score|corpus][,prior=<path>]
///                  | ismcts:sims=N[,eval=score|corpus][,prior=<path>]
///
/// This lived inline in `src/main.cpp` until Task 12.2. main.cpp is not
/// linkable from `riftbound_tests`, so the parser was unreachable by
/// unit tests and the `eval=` option could not be proven; moving it into
/// riftbound_core makes it testable without changing any behaviour. The
/// header stays free of OpenSpiel and of anything in `agents/` that
/// needs it, so riftbound_core keeps its dependency shape.
///
/// `prior=<path>` (L4 — prior injection, see
/// docs/superpowers/specs/2026-09-07-replay-loop-iter0-design.md) points
/// at a schema-v1 JSON file overriding some or all of the MCTS search
/// prior's family weights and/or the corpus evaluator's term weights for
/// one matchup. `PriorConfig`/`FamilyWeights` live here (rather than in
/// `mcts_agent.h`, which is not linkable from `riftbound_tests`) so both
/// the parser and the family-weight lookup it enables are unit-testable.

#include "corpus_evaluator.h"  // CorpusWeights

#include "core/types.h"        // IntentType

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

/// Unnormalized MCTS search-prior weights, one per action "family" —
/// the same grouping `MctsAgent`'s `Prior()` has always used. Every
/// field default-initializes to the literal `Prior()` has hard-coded
/// since Task 12, so `PriorConfig{}` reproduces today's behaviour
/// exactly. Field names match the `action_family_weights` JSON keys in
/// schema v1.
struct FamilyWeights {
    double play                = 4.0;
    double combat_damage       = 4.0;
    double move_to_battlefield = 3.0;
    double move_to_base        = 0.6;
    double activate            = 2.5;
    double choice              = 1.5;
    double setup               = 1.0;  // mulligan / choose-battlefield / play-first
    double pass                = 0.3;  // end turn / pass priority / pass focus
    double concede             = 0.01;
};

/// The pure lookup `Prior()` delegates to: which family does an action
/// of `type` belong to, and what unnormalized weight does that family
/// get from `family`. `moves_to_battlefield` disambiguates the one
/// intent type (`StandardMove`) whose family depends on more than its
/// `IntentType` — a move onto a battlefield is the `move_to_battlefield`
/// family, a move to base is `move_to_base`. Anything not classified
/// into a named family (a handful of triggered-ability response types
/// with no evidence either way) returns the same neutral 1.0 every
/// unclassified legal action got before this refactor — that fallback
/// is not part of schema v1 and is not overridable.
double intentFamilyWeight(IntentType type, bool moves_to_battlefield,
                          const FamilyWeights& family);

/// Everything a `prior=<path>` file can override: the MCTS search
/// prior's family weights and the corpus evaluator's term weights.
/// Default-constructs to today's hard-coded values for both, so an
/// `AgentSpec` with no `prior=` key behaves byte-identically to before
/// this option existed.
struct PriorConfig {
    FamilyWeights family;
    CorpusWeights evaluator;
};

/// Load and validate a schema-v1 prior file at `path`.
///
/// Every key is optional; an absent key keeps its `PriorConfig{}`
/// default. `schema`, `matchup`, and `written` are accepted (any
/// metadata a vault file wants to carry for humans) but not otherwise
/// consumed. Any key outside {schema, matchup, written,
/// action_family_weights, evaluator_weights} at the top level, or
/// outside the named weight keys inside either weights object, is a
/// hard error naming the offending key — a typo in a prior file must
/// not silently do nothing.
///
/// @throws std::runtime_error if `path` does not exist or is not
///         readable, is not valid JSON, or contains an unknown key.
PriorConfig loadPriorConfig(const std::string& path);

struct AgentSpec {
    std::string raw;                             // original spec string
    std::string kind;                            // random | human | mcts | ismcts
    int sims = 0;                                // populated for mcts/ismcts
    EvaluatorKind eval = EvaluatorKind::Score;   // mcts/ismcts evaluator choice
    PriorConfig prior;                           // mcts/ismcts prior override
                                                  // (default = today's hard-coded
                                                  // weights when no prior= is given)
};

/// Parse an agent spec string.
/// @throws std::runtime_error on an unknown kind, an mcts/ismcts spec
///         carrying options but no positive `sims`, an unrecognised
///         `eval` value, or a `prior=` path that `loadPriorConfig`
///         rejects (validated eagerly, at parse time).
AgentSpec parseAgentSpec(const std::string& s);

}  // namespace riftbound
