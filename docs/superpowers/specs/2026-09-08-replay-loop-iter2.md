# Replay loop — iteration 2 (pre-registered 2026-09-08, before any game ran)

Tyler's ruling: "Go with option b and run iteration 2." Option (b) from
iteration 1's acceptance record: the lever moves from the search prior
into the EVALUATOR.

## Arms (80 simulations both sides, seeds 2000 / 3000, 200 games per seat, `--threads 4`, binary `192e9e1`)

| Arm | Agent spec (Kennen side) | Purpose |
|---|---|---|
| Frozen | `mcts:sims=80` (score evaluator) — the iteration-1 baseline, ALREADY ON RECORD (197/400 = 49.2%), seed-matched | the previous agent |
| New | `mcts:sims=80,eval=corpus,prior=<vault prior file>` — corpus evaluator with the learned weights `unit` 0.25→0.5, `battlefield` 0.5→0.6; action-family weights at DEFAULT (the iteration-0/1 family changes are removed from the file in the same change) | the agent under test |
| Control | `mcts:sims=80,eval=corpus` — corpus evaluator at its default weights | attribution: evaluator vs weights |

Rengar always `mcts:sims=80` (score evaluator). Run order: New, then
Control. Each arm's analyzer Validity must be OK or that arm is invalid.

## Pre-registered rules

1. **H1 (acceptance):** ACCEPT only if the New arm's pooled 400-game
   Wilson 95% interval has a lower bound above the Frozen arm's pooled
   point estimate (49.2%). Otherwise REJECT / "not confirmed".
2. **Attribution (only if accepted):** if New's lower bound is also above
   Control's point estimate, the WEIGHTS did it; otherwise the evaluator
   change did it and the weights are "not confirmed" on their own.
3. **Replication:** the ★ signals re-measured on the New arm (they are
   about the matchup, not the agent, and should hold).
4. **Behaviour shift:** the same four counts as before, Frozen vs New vs
   Control.

## Stated risk (from Task 12's review, on the record)

With `unit` 0.5 and `battlefield` 0.6 the six proxy terms can sum to
~3.15 points of evaluator value against a victory score of 8, so a maxed
opposing board can outvote a small score lead. That is the hypothesis
being tested, not a reason to tune before measuring.

## Pre-registered next step

If REJECT again: three levers tried (family prior at 20 and 80 sims,
evaluator weights at 80 sims), none confirmed. Iteration 3 is then a
DESIGN decision, not a parameter change — the honest options are
(i) fit the evaluator weights from the logs instead of hand-setting them
(the "agents learn" step of the roadmap), or (ii) a larger n to resolve
a 2–3 point effect. Tyler chooses. If ACCEPT: the New agent becomes the
frozen agent, and the playbook records the confirmed lesson with its
attribution.
