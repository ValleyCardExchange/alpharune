# Replay loop — iteration 1 (pre-registered 2026-09-08, before any game ran)

Tyler's ruling: "Go with option a, run more sims, and iteration 1."
Option (a) from iteration 0's acceptance record: make the prior's lever
bite by giving the search more simulations, changing NOTHING else.

## Configuration (one variable changed from iteration 0)

| | Iteration 0 | Iteration 1 |
|---|---|---|
| MCTS simulations, both sides | 20 | **80** (4×; measured 2.3× wall time per game) |
| Evaluator | score-only | score-only |
| Prior under test | `kennen-vs-rengar.prior.json` (move_to_base 0.6→1.2, move_to_battlefield 3.0→3.5) | the SAME file, unchanged |
| Frozen baseline | 20-sim, no prior (iteration-0 L3) | **fresh 80-sim, no prior** — a 20-sim baseline is not comparable |
| Games | 200 per seat per arm | 200 per seat per arm (400 per arm) |
| Seeds | 2000 (Kennen P1) / 3000 (Kennen P2) | same |
| Binary | `b1f2a89` build | `192e9e1` build (L0c; behaviour-neutral for these decks — 8-game smoke identical) |
| Validity | analyzer Validity must be OK | same; a burst invalidates the arm |

Order: baseline P1, baseline P2, prior P1, prior P2 — the baseline logs
are analyzed while the prior arm runs.

## Pre-registered questions and rules

1. **H1 (acceptance):** at 80 sims the prior raises Kennen's win rate over
   the 80-sim frozen baseline. Rule unchanged from the iteration-0 spec:
   ACCEPT only if the pooled 400-game Wilson 95% interval of the prior
   arm has a lower bound above the frozen arm's pooled point estimate.
   Anything else is REJECT / "not confirmed".
2. **Replication (the loop's other direction):** do iteration 0's signals
   hold at 80 sims in the baseline arm — early board (turns 3–6), retreats,
   margin at turn 8, scoring first? Each is reported with its interval and
   the playbook gains an 80-sim column; a signal that fails to replicate is
   downgraded in the playbook, not deleted.
3. **Secondary (not an acceptance criterion):** does Kennen's baseline win
   rate move with depth (46.5% at 20 sims)? Reported, not acted on.
4. **Behaviour shift:** the same four behaviour counts as iteration 0's
   acceptance record (zero-retreat games, ≥5 empty-board turns, zero early
   retreats, early board < 1), baseline vs prior at 80 sims.

## Pre-registered next step

If REJECT again, iteration 2 defaults to option (b) — the lever moves into
the evaluator (`unit` / `battlefield` weights already in the prior file)
with `eval=corpus` — unless Tyler rules otherwise. If ACCEPT, the prior
becomes the new frozen agent and the playbook records the confirmed
lesson.
