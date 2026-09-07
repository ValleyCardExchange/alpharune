#pragma once
/// @file corpus_evaluator.h
/// Corpus heuristic position evaluator — a pure function of GameState.
///
/// Spec: docs/superpowers/specs/2026-09-07-corpus-evaluator-design.md
///
/// This is the "play the book" evaluator: every term below cites a
/// principle the Riftbound coaching corpus has EVIDENCE for, and any
/// principle without evidence is deliberately absent. It does not learn
/// — the weights are hand-set from the corpus' qualitative evidence and
/// the seat-balanced A/B is the only calibration.
///
/// Why a free function in riftbound_core rather than a method on the
/// OpenSpiel Evaluator: `src/agents/mcts_agent.cpp` is compiled into the
/// `riftbound` executable (it needs the OpenSpiel game/state TUs whose
/// static registrars must not be stripped), so nothing in it is linkable
/// from `riftbound_tests`. Keeping the arithmetic here — dependent only
/// on `core/` headers — makes every term unit-testable; the OpenSpiel
/// `Evaluator` wrapper in mcts_agent.cpp is then a thin delegation with
/// the terminal short-circuit.
///
/// Contract: perfect-information, same as the existing score-only
/// evaluator. Nothing here reads more than that evaluator's contract
/// allows, and nothing here is card-name specific (engine house rule).

#include <utility>

namespace riftbound {

struct GameState;

// ── Weights (each cites the spec section that ratified it) ──────────────
//
// All terms are computed from P1's perspective, summed, and clamped to
// [-1, +1]. Every term except the score term is scaled by
// 1 / victory_score so the score term stays dominant — the prior
// evaluator's hard-won lesson was that proxies which swamp score make
// the bot stop chasing points.

/// Term 1 — score difference, `(p1.score - p2.score) / victory_score`.
/// Evidence: the win condition (CR 194.3); the existing evaluator's
/// measured result that score is the signal at low sim budgets.
inline constexpr double kCorpusWeightScore = 1.0;

/// Term 2 — point tempo. Per battlefield held uncontested; a contested
/// battlefield is split half to the holder and half to the contester.
/// Evidence: Moe's point-tempo/cadence framing and Daremx's "Explaining
/// Point Tempo" (holding a battlefield at turn start is the next point);
/// SouL's "score is final". One held battlefield ~ half a point.
inline constexpr double kCorpusWeightBattlefield = 0.5;

/// Term 3 — board width / retention, diminishing (1.0, 0.75, 0.5, 0.25)
/// and capped at four units per side.
/// Evidence: the StapleGod deep-dive (wide, cheap, rebuildable board is
/// the antidote to board evaporation; "never empty"); the 24-game pool's
/// board-to-zero losses.
inline constexpr double kCorpusWeightUnit = 0.25;
inline constexpr int    kCorpusUnitCap = 4;

/// Term 4 — held interaction: Reaction spells in hand, capped at two.
/// Evidence: StapleGod holding Hard Bargain / Gust / Switcheroo as
/// insurance; Singapore GF ("Not So Fast" won the title); Zelonius
/// "play to outs / don't tap out".
inline constexpr double kCorpusWeightHeldInteraction = 0.15;
inline constexpr int    kCorpusHeldInteractionCap = 2;

/// Term 5 — live trash resources: spells in trash with a LIVE Flow cost
/// (printed Keyword::Flow, or a granted flow stamped this turn), cap 3.
/// Evidence: the Kennen manual's "trash is a second hand" and the
/// observed Ride-the-Wind re-conquer loop. Deck-agnostic in form — any
/// deck with Flow benefits, every other deck reads 0.
inline constexpr double kCorpusWeightTrashResource = 0.1;
inline constexpr int    kCorpusTrashResourceCap = 3;

/// Term 6 — empowered legend.
/// Evidence: Heart of the Tempest's action converts Empowered into
/// Assault 2 — a stored resource.
inline constexpr double kCorpusWeightEmpoweredLegend = 0.1;

// Explicitly EXCLUDED (no evidence, or evidence against): ready runes on
// the opponent's turn (the "rune-poor" leak was RETRACTED in the corpus
// — top pilots run 0-2 ready runes too); hand size (the previous
// evaluator removed it as noise); seat; anything card-name specific.

/// Evaluate `state` from both players' perspectives.
/// Returns `{value_for_player1, value_for_player2}`, each in [-1, +1] and
/// exactly antisymmetric (`second == -first`).
///
/// Terminal states are NOT this function's business — the OpenSpiel
/// wrapper short-circuits to `State::Returns()` before calling in.
std::pair<double, double> corpusEvaluate(const GameState& state);

}  // namespace riftbound
