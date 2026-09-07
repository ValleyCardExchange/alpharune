#pragma once
/// @file decision_log_writer.h
/// Machine-readable per-decision JSON log — the L1 component of the
/// replay-analysis loop (docs/superpowers/specs/2026-09-07-replay-loop-iter0-design.md).
///
/// Unlike ReplayWriter (human-readable HTML, buffered in memory and
/// written once at game end), DecisionLogWriter STREAMS one JSON record
/// per decision directly to disk as the game plays, so a crashed game
/// still leaves a parseable prefix on disk (see `finish()` / destructor
/// below). It has no CardDB dependency — card/battlefield names come
/// straight off the live GameState's GameObjects.
///
/// File shape (top-level JSON object):
///   {
///     "schema": 1,
///     "decks": {"p1": {path, legend, champion}, "p2": {...}},
///     "agents": {"p1": "<spec>", "p2": "<spec>"},
///     "seed": <uint64>,
///     "seats": {"<deck1 basename>": "P1", "<deck2 basename>": "P2"},
///     "engine_version": "<kVersionTag>",
///     "log": [ <record>, <record>, ... ],
///     "winner": {"deck": "<legend> / <champion>", "seat": "P1"|"P2"|"None"},
///     "reason": "...",
///     "turns": <int>,
///     "final_scores": [p1, p2],
///     "decisions": <int>            // total decision count (footer)
///   }
/// NOTE: the streamed per-decision array is keyed "log", not "decisions" —
/// the spec's footer field list names a "decisions" key too (the total
/// decision COUNT, matching GameResult::total_decisions), and a JSON
/// object cannot use the same key twice. See l1-report.md "concerns" for
/// why this name was picked.
///
/// Each record in "log":
///   {
///     "idx": <int>,                 // GameState::decision_index
///     "turn": <int>,                // TurnState::turn_number
///     "phase": {"phase": "MainPhase", "ns_state": "Neutral"|"Showdown",
///               "oc_state": "Open"|"Closed"},
///     "actor": "P1"|"P2",           // the Intent's own `player` field
///     "scores": [p1, p2],
///     "players": [ {seat, hand, deck, trash, banishment, runes_ready,
///                   runes_exhausted, legend_empowered}, ... ],   // P1, P2
///     "battlefields": [ {id, name, controller, contested,
///                        units: {p1: {count, might}, p2: {count, might}}}, ... ],
///     "legal_count": <int>,
///     "chosen": {type, card (name or null), source_zone, flow_source,
///                restricted_bf (bf id or null), destination (or null)},
///     "root_value": <double or null>   // MCTS root-value estimate, when
///                                       // the deciding agent supplied one
///                                       // via setNextRootValue(). No
///                                       // current call site feeds this —
///                                       // MctsAgent exposes no accessor
///                                       // and is out of scope for L1 (see
///                                       // l1-report.md) — so it is
///                                       // always null today.
///   }

#include "core/game_state.h"
#include "core/intent.h"

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace riftbound {

struct DecisionLogHeader {
    struct Deck {
        std::string path;
        std::string legend;
        std::string champion;
    };
    Deck deck1;
    Deck deck2;
    std::string agent1_spec;
    std::string agent2_spec;
    uint64_t seed = 0;
    std::string engine_version;
};

class DecisionLogWriter {
public:
    /// Opens `output_path` and writes the header + opens the "log" array
    /// immediately. Throws std::runtime_error if the file can't be opened.
    DecisionLogWriter(const std::string& output_path, DecisionLogHeader header);

    /// Best-effort close: if `finish()` was never called (the game crashed
    /// or threw), writes a `"truncated": true` marker so the file still
    /// parses as valid JSON (minus the footer fields).
    ~DecisionLogWriter();

    DecisionLogWriter(const DecisionLogWriter&) = delete;
    DecisionLogWriter& operator=(const DecisionLogWriter&) = delete;

    /// Record one decision point. Call once per `GameEngine::on_decision`
    /// invocation, in order.
    void recordDecision(const GameState& state,
                         const std::vector<Intent>& legal_actions,
                         const Intent& chosen_action);

    /// Attach an MCTS root-value estimate to the NEXT recordDecision call
    /// (mirrors ReplayWriter::setNextWinProb's one-shot-pending pattern).
    /// Cleared after consumption; a decision with no call before it
    /// records `root_value: null`.
    void setNextRootValue(double value);

    /// Close the "log" array and write the footer. Call once at game end.
    /// Safe to call at most once; a second call is a no-op.
    void finish(PlayerId winner,
                const std::string& reason,
                int turns,
                const int final_scores[2],
                int decision_count);

private:
    std::ofstream file_;
    DecisionLogHeader header_;
    bool wrote_first_ = false;
    bool finished_ = false;
    std::optional<double> pending_root_value_;

    std::string winnerDeckName(PlayerId winner) const;
};

} // namespace riftbound
