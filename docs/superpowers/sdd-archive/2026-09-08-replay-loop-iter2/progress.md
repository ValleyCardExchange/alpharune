# SDD ledger — replay loop iteration 2 (option b: evaluator lever)

Spec: docs/superpowers/specs/2026-09-08-replay-loop-iter2.md (pre-registered). Tyler's ruling 2026-09-08: "Go with option b and run iteration 2."
## Task log
- Prior file in the vault edited in place: family weights back to default; evaluator unit 0.5, battlefield 0.6 (git history keeps the iteration-0/1 version). Timing: corpus+prior at 80 sims 52s per 4 games (score: 50s) — no cost.
- Frozen arm = iteration-1 80-sim baseline (reused, seed-matched). Launched New (corpus+prior) then Control (corpus default) detached: scratchpad/run_iter2.sh; logs /home/user/chorlick/batches/iter2/{new,control}/kennenP{1,2}; markers NEW_DONE, DONE.
- 05:15 New arm P1 done: Kennen 114/200 = 57.0% (frozen P1 102/200 = 51.0%), Validity OK. Pooled verdict waits on New P2 (running), then Control.
- 06:04 New arm DONE: Kennen 222/400 = 55.5% [50.6–60.3] vs frozen 49.2% → ACCEPT under the pre-registered rule (first acceptance). Validity OK both seats. Behaviour: zero-retreat games 35→16, zero-early-retreat 126→80, early-width<1 195→164, empty5 93→96. Attribution waits on the Control arm (corpus default weights), running.
- 06:55 Control arm P1 done: corpus at DEFAULT weights Kennen 119/200 = 59.5% (New P1 114, frozen 102), Validity OK — attribution likely goes to the evaluator change, not the learned weights; pooled attribution waits on Control P2.
- 07:43 Control arm DONE: corpus default Kennen 224/400 = 56.0% [51.1–60.8] — also beats frozen; New vs Control level (222 vs 224) → attribution: the EVALUATOR change did it, learned weights not confirmed on their own. Task 12's 20-sim 'not distinguishable' reversed at 80 sims (+7 points). Validity OK all four seats. Iteration 2 CLOSED (ACCEPT); records committed; ledger archived.
