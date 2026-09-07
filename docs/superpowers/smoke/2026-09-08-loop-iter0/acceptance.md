# L5 acceptance — iteration 0 priors (Kennen vs Rengar)

Frozen agent: `mcts:sims=20` (score evaluator), the L3 batch itself.
New agent: `mcts:sims=20,prior=<personal-ai vault>/riftbound/playbooks/kennen-vs-rengar.prior.json`
(changes exactly two action-family weights: `move_to_base` 0.6 → 1.2,
`move_to_battlefield` 3.0 → 3.5; the evaluator weights in the file are
inert with the score evaluator). Same binary as L3 (the copied `b1f2a89`
build), same seeds (2000 / 3000), 200 games per seat, `--threads 4`.
Analyzer Validity: OK in both seats (longest identical run 2).

## Result

| Seat | New Kennen wins | New rate | Wilson 95% | Frozen (L3) | lower bound > frozen? |
|---|---|---|---|---|---|
| P1 | 100/200 | 50.0% | 43.1–56.9 | 48.0% | no |
| P2 | 93/200 | 46.5% | 39.7–53.4 | 45.0% | no |
| both | 193/400 | 48.2% | 43.4–53.1 | 46.5% | no |

**VERDICT: REJECT — "not confirmed".** Rule (spec L5): the new interval's
lower bound must exceed the frozen point estimate on the pooled games.
It does not. The +1.7-point move is inside noise.

## What the priors actually did to behaviour (knowledge about the agent)

Games in each bucket, L3 (frozen) → L5 (prior), 400 games each:

| Behaviour the prior targeted | L3 | L5 |
|---|---|---|
| games with ZERO moves to base | 51 | 35 |
| games with ≥5 own turns ending with no unit on a battlefield | 142 | 122 |
| games with zero retreats before turn 9 | 170 | 176 |
| games with mean early board (turns 3–6) below 1 unit | 215 | 223 |

The retreat lever moved a little in the intended direction (12.8% → 8.8%
of games with no retreat at all; 35% → 30% of games with five or more
empty-board turns); the early-board lever did not move. The within-batch
correlations replicate almost exactly (0 retreats → 11%; early board < 1 →
39%; scoring first → 63%), so the SIGNALS are stable — what is weak is
the LEVER: at 20 simulations a prior weight only reorders expansion, and
the score-only evaluator decides the rest. A prior is not a strategy.

## Ruling for iteration 1 (recorded, not enacted)

Three ways to make the lever bite, to be chosen deliberately, one per
iteration: (a) more simulations, so the prior's expansion order survives
into the value estimates; (b) move the lever into the evaluator (the
`unit` and `battlefield` weights, which already reflect the same
evidence) and run with `eval=corpus`; (c) a larger n per seat so a
two-point effect can be resolved at all (400 games resolves ~±5). The
rejected lesson stays in the playbook marked "not confirmed".
