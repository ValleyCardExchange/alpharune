# Corpus heuristic evaluator for MCTS — design spec (Task 12)

Date: 2026-09-07 (overnight, on Tyler's ruling "Do option one after task
11" and "build this out and have it ready to go tomorrow"). Status: written
by the controller from the vault's EVIDENCED principles; Tyler reviews in
the morning. Nothing here is a learning system — it makes MCTS play the
book; the two-directional replay loop and RL are later steps of the
roadmap recorded in personal-ai (`riftbound/self-play-engine.md`).

## Goal

Replace the MCTS position evaluator — today `score difference only`
(`src/agents/mcts_agent.cpp` `RiftboundHeuristicEvaluator::Evaluate`; the
comment block above it still describes a three-term heuristic whose two
minor terms were removed) — with a selectable "corpus" evaluator whose
terms come only from principles the coaching corpus has EVIDENCE for, and
prove it beats the score-only evaluator in seat-balanced self-play.

## What the evaluator may know (engine state it can read)

`RiftboundState::engineState()` exposes the full `GameState`: per-player
`score`, `hand`, `trash`, `main_deck`, `legend_zone` (an object with
`is_empowered`), `runesInBase(player)`, `unitsAt(location, controller)`,
`state.battlefields[]` with `controller` / `is_contested`, `turn.turn_player`,
`mode.victory_score`, and per-object `keywords` / `granted_flow` / `zone`.
No hidden information is read beyond what the perfect-information MCTS
already sees (this is the existing evaluator's contract; ISMCTS is out of
scope).

## Terms (each cites its evidence; anything without evidence is excluded)

All terms are computed from P1's perspective and the result is
`clamp(sum, -1, +1)` with the score term dominant so the bot still chases
points (the prior comment's lesson: proxies must never swamp score).

1. **Score difference** — `(p1.score − p2.score) / victory_score`, weight
   1.0. Evidence: the win condition (CR 194.3); the existing evaluator's
   measured result that score is the signal at low sim budgets.
2. **Point tempo (battlefields controlled)** — for each battlefield,
   +w_bf if P1 controls it uncontested, −w_bf if P2 does; a contested
   battlefield counts half for the contester. Evidence: Moe's point-
   tempo/cadence framing and Daremx's "Explaining Point Tempo" (holding a
   battlefield at turn start is the next point); SouL's "score is final".
   w_bf = 0.5 / victory_score (one held battlefield ≈ half a point of
   expected tempo).
3. **Board width / retention** — +w_unit per friendly unit on the board up
   to a cap of 4 (diminishing: 1.0, 0.75, 0.5, 0.25), minus the same for
   the opponent. Evidence: the StapleGod deep-dive (wide, cheap,
   rebuildable board is the antidote to board evaporation; "never empty");
   the 24-game pool's board-to-zero losses. w_unit = 0.25 / victory_score.
4. **Held interaction** — +w_hold per Reaction spell in P1's hand, capped
   at 2; symmetric for P2. Evidence: StapleGod holding Hard Bargain / Gust
   / Switcheroo as insurance; Singapore GF ("Not So Fast" won the title);
   Zelonius "play to outs / don't tap out". w_hold = 0.15 / victory_score.
5. **Live trash resources** — +w_trash per spell in P1's trash that has a
   live Flow cost (printed keyword or a granted flow stamped this turn),
   capped at 3; symmetric. Evidence: the Kennen manual's "trash is a second
   hand" and the observed Ride-the-Wind re-conquer loop; deck-agnostic in
   form (any deck with Flow benefits; others read 0). w_trash = 0.1 /
   victory_score.
6. **Empowered legend** — +w_emp if P1's legend is empowered; symmetric.
   Evidence: Heart of the Tempest's action converts Empowered into Assault
   2 — a stored resource. w_emp = 0.1 / victory_score.

Explicitly EXCLUDED (no evidence or evidence against): ready runes on the
opponent's turn (the "rune-poor" leak was RETRACTED in the corpus — top
pilots run 0–2 ready runes too); hand size (the existing evaluator removed
it as noise; no corpus evidence); seat (not an evaluation term); anything
card-name specific (no `if name == …` in the evaluator — the engine's own
rule).

## Selection

`buildAgent`'s spec string gains `eval=score|corpus` (default `score`, so
every existing command line and test is unchanged): `mcts:sims=50,eval=corpus`.
`parseAgentSpec` (`src/main.cpp` ~:170-200) currently recognises only
`sims=`; add `eval=` with validation (unknown value → the same
`runtime_error` shape as a missing `sims`). `MctsAgent`'s constructor
takes the evaluator choice; the corpus evaluator is a sibling class of
`RiftboundHeuristicEvaluator` in `mcts_agent.cpp` sharing the terminal
short-circuit (`Returns()` at terminal).

## Tests (each must be watched failing first)

Unit tests on constructed `GameState`s (a new `tests/test_corpus_evaluator.cpp`
using the engine's state structs directly — the evaluator is a pure
function of `GameState` once exposed through a small free function
`corpusEvaluate(const GameState&) -> std::pair<double,double>` that both
the OpenSpiel `Evaluator` wrapper and the tests call):
1. Empty symmetric state → {0, 0}; terminal states are not the free
   function's business (the wrapper short-circuits to `Returns()`).
2. Score term: P1 at 4, P2 at 0, victory 8 → P1 value 0.5 (only term).
3. Battlefield term: P1 controls one battlefield uncontested → +0.5/8;
   contested by P2 → +0.25/8 for P1 and +0.25/8 for P2 nets 0.
4. Board term: 1, 2, 4, 6 friendly units → 0.25/8 × {1, 1.75, 2.5, 2.5}.
5. Held interaction: 3 Reaction spells in hand → capped at 2 × 0.15/8;
   Action-only spells count 0.
6. Trash resources: a Flow-keyword spell in trash counts; a granted flow
   stamped THIS turn counts; a granted flow stamped last turn does not;
   a non-Flow spell does not; cap 3.
7. Empowered legend: +0.1/8; not empowered 0.
8. Antisymmetry: for any constructed state, value(P1) == −value(P2).
9. Clamp: a wildly lopsided state stays within [−1, +1].
10. Spec parsing: `mcts:sims=50,eval=corpus` selects the corpus
    evaluator; `eval=bogus` throws; `mcts:sims=50` still selects `score`.

## Proof (the A/B, reported honestly)

Batch runner (`--games N --threads 4`), sims=20 (fast enough overnight),
seat-balanced, decks as in the smoke notes:
- Baseline: `score` vs `score`, Kennen vs Rengar, 100 games each seat.
- Treatment: `corpus` (Kennen) vs `score` (Rengar), 100 games each seat.
- Control for deck: `corpus` vs `score` in the Kennen MIRROR, 100 games
  each seat.
Report win counts with a Wilson 95% interval; the claim "the corpus
evaluator is stronger" is made only if the mirror control's interval
excludes 50%. Anything else is reported as "not distinguishable at this
sample". Batch results go in `docs/superpowers/smoke/2026-09-08-corpus-ab.md`.

## Known limits (stated, not hidden)

- The agent still runs through the OpenSpiel action vocabulary; Task 11
  must land first or the new cards remain aliased.
- The evaluator plays the book; it does not learn. Weights are hand-set
  from the corpus' qualitative evidence, not fitted — the A/B is the only
  calibration.
- Perfect-information MCTS sees both hands; "held interaction" for the
  opponent is therefore visible to the bot in a way it is not to a human.
