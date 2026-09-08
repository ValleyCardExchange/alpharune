# SDD ledger — replay loop iteration 1 (option a: more sims)

Spec: docs/superpowers/specs/2026-09-08-replay-loop-iter1.md (pre-registered before any game). Tyler's ruling 2026-09-08: "Go with option a, run more sims, and iteration 1."
## Task log
- Timing on the L0c binary (4 games, 4 threads): 20 sims 22s, 50 sims 30s, 80 sims 50s. Chose 80 sims (4× iteration 0; ~85 min per 400-game arm). Binary snapshot scratchpad/riftbound-192e9e1 so any rebuild cannot disturb the batches.
- Launched 4 sequential batches detached (scratchpad/run_iter1.sh): base P1 seed 2000, base P2 seed 3000, prior P1, prior P2; logs /home/user/chorlick/batches/iter1/{base,prior}/kennenP{1,2}; markers BASE_DONE, DONE.
- 02:10 harness worker restarted mid-batch; the detached batch script survived (setsid). base P1 done 02:05 (Kennen 102/200 = 51.0%, Validity OK; all iteration-0 signals replicate at 80 sims); base P2 continuing. Monitor re-armed.
- 02:46 baseline arm DONE: Kennen 197/400 = 49.2% [44–54] (P1 102, P2 95), Validity OK both. Replication: tempo/first-score/early-board/retreats hold at 80 sims; Seal-count and Hard Bargain/Harrowing lines do NOT (downgraded in the playbook); depth alone cut zero-retreat games 51→35 and 5+-empty-board games 142→93. Prior arm running.
- 03:25 prior P1 done: Kennen 96/200 = 48.0% vs baseline P1 102/200 = 51.0% — below baseline in this seat; pooled verdict waits on prior P2 (running).
- 04:01 prior arm DONE: Kennen 182/400 = 45.5% [40.7–50.4] vs baseline 49.2% → REJECT (second time); prior arm below baseline in both seats; behaviour counts moved AGAINST intent (zero-retreat 35→44, empty5 93→125). Validity OK all four seats. Record + acceptance committed; iteration 2 defaults to option (b) per pre-registration. Iteration 1 CLOSED; ledger archived.
