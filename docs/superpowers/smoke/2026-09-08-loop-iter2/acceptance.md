# Iteration 2 acceptance — option (b), the evaluator lever

Pre-registration: `docs/superpowers/specs/2026-09-08-replay-loop-iter2.md`
(written before any game ran). 80 simulations both sides, binary
`192e9e1`, seeds 2000 / 3000, 200 games per seat per arm, `--threads 4`.
Analyzer Validity OK in all four new seats (longest identical run 2).

| Arm | Kennen agent | P1 | P2 | Pooled | Wilson 95% |
|---|---|---|---|---|---|
| Frozen | `mcts:sims=80` (score evaluator; iteration-1 baseline) | 102/200 | 95/200 | 197/400 = 49.2% | 44.4–54.1 |
| New | `mcts:sims=80,eval=corpus,prior=<vault file>` (unit 0.5, battlefield 0.6, family default) | 114/200 | 108/200 | **222/400 = 55.5%** | **50.6–60.3** |
| Control | `mcts:sims=80,eval=corpus` (default weights) | 119/200 | 105/200 | **224/400 = 56.0%** | **51.1–60.8** |

## Verdicts (rules as pre-registered)

1. **H1 — ACCEPT.** New's pooled lower bound (50.6) exceeds Frozen's point
   estimate (49.2). The first acceptance in three iterations. Per the
   pre-registration the New agent becomes the frozen agent for
   iteration 3.
2. **Attribution — the EVALUATOR change did it; the learned weights are
   "not confirmed on their own".** New's lower bound (50.6) does not
   exceed Control's point estimate (56.0); New and Control are level
   (222 vs 224). The corpus evaluator at its default weights beats the
   frozen score agent by the same rule (51.1 > 49.2). Dropping the weight
   overrides would lose nothing measurable.
3. **A Task 12 finding is reversed by depth.** At 20 simulations the
   corpus evaluator was "not distinguishable" from score-only (mirror
   46%, interval including 50%). At 80 simulations it is worth about +7
   points against the same opponent. The book-derived terms (battlefield
   tempo, board width, held interaction, live trash resources, empowered
   legend) need enough search to be cashed in.

## Behaviour (games of 400)

| Behaviour | Frozen | New | Control |
|---|---|---|---|
| zero moves to base | 35 | 16 | 22 |
| ≥5 own turns ending with no unit on a battlefield | 93 | 96 | 108 |
| zero retreats before turn 9 | 126 | 80 | 80 |
| mean early board (turns 3–6) below 1 unit | 195 | 164 | 155 |
| games where Kennen scores first | 209 | 263 | — |

The evaluator agents retreat early far more (zero-early-retreat games
126 → 80 in both), keep a thicker early board, and score first more
often — the iteration-0 lessons, produced by valuing them rather than
by biasing the search toward them. The empty-board count did not move:
that lesson is not yet expressed by any evaluator term.

## What the two-directional loop has now done, once

Claude read the agents' games (iteration 0) → wrote the lesson (board on
turns 3–6, retreat to keep units, tempo by turn 8) → the lesson went
back into the agents; twice through the search prior it did not take,
once through the evaluator it did → the agents beat their frozen
previous selves on it (55.5% vs 49.2%) → and the accepted agent's games
are now the next thing to read.

## Pre-registered next step

Iteration 3 is Tyler's call. Honest candidates: (i) read the ACCEPTED
agent's games and re-derive the playbook — the loop's other direction on
the stronger agent; (ii) fit the evaluator weights from the logs instead
of hand-setting them (the roadmap's "agents learn" step) — the weights
did nothing by hand, which is exactly what fitting is for; (iii) an
evaluator term for "no unit on a battlefield at end of turn", the one
lesson the current terms do not express.
