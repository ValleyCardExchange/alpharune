# Iteration 1 acceptance — option (a), 80 simulations

Pre-registration: `docs/superpowers/specs/2026-09-08-replay-loop-iter1.md`
(written before any game ran). Frozen = fresh 80-sim baseline, no prior.
New = the unchanged iteration-0 prior file (`move_to_base` 0.6 → 1.2,
`move_to_battlefield` 3.0 → 3.5), 80 sims. Same binary (`192e9e1`), same
seeds (2000 / 3000), 200 games per seat per arm, `--threads 4`. Analyzer
Validity OK in all four seats (longest identical run 2).

## Result

| Seat | Frozen | New | New Wilson 95% | lower bound > frozen? |
|---|---|---|---|---|
| P1 | 102/200 = 51.0% | 96/200 = 48.0% | 41.2–54.9 | no |
| P2 | 95/200 = 47.5% | 86/200 = 43.0% | 36.3–49.9 | no |
| both | 197/400 = 49.2% | 182/400 = 45.5% | 40.7–50.4 | no |


| Behaviour | Frozen | New |
|---|---|---|
| zero_retreats | 35 | 44 |
| empty5 | 93 | 125 |
| zero_early_retreats | 126 | 140 |
| early_width_lt1 | 195 | 203 |

Frozen arm Wilson: 44.4–54.1

**VERDICT: REJECT — "not confirmed", for the second time.** The prior arm
is BELOW the frozen arm in both seats (48.0 vs 51.0; 43.0 vs 47.5) and
pooled (45.5% vs 49.2%). The baseline's point estimate sits inside the
prior arm's interval, so "worse" is not established either — but the
direction is consistent across seats and the behaviour counts moved
AGAINST the prior's intent: more zero-retreat games (35 → 44), more
games with five or more empty-board turns (93 → 125), more zero-early-
retreat games (126 → 140). Raising the search prior on retreats and
battlefield moves made the 80-sim agent retreat less and empty its board
more. A prior reorders which children the search expands first; with a
score-only evaluator that spends simulations on lines the evaluator
cannot distinguish, and the deeper the search, the more it costs.

## What replicated (the loop's other direction)

All four iteration-0 ★ signals hold in the 80-sim baseline (margin at
turn 8: 13% / 84%; scoring first 66% / 30%; early board < 1: 37%, ≈2:
85%; zero retreats 11%). Two iteration-0 lines did NOT replicate and
were downgraded in the playbook: the Seal-count signal (3+ Seals: 39% →
63%) and the Hard Bargain / Harrowing "played → losing" lines (36% →
48–49%). Depth alone cut zero-retreat games 51 → 35 and 5+-empty-board
games 142 → 93: a third of the iteration-0 "mistakes" were search-depth
artifacts, which is evidence FOR the lessons (better search converges on
them) and AGAINST family priors as the way to teach them.

## Pre-registered next step

Iteration 2 defaults to option (b): the lever moves into the EVALUATOR
(`unit` and `battlefield` weights, already in the prior file) with
`eval=corpus`, family weights back at default, 80 sims, same seeds and
rule — unless Tyler rules otherwise. Two rejections in a row also earn a
standing note in the playbook: the search prior is not a teaching lever
in this engine.
