# Replay loop — iteration 0 batch (Kennen vs Rengar)

Spec: `docs/superpowers/specs/2026-09-07-replay-loop-iter0-design.md`
(L3). Plan: `docs/superpowers/plans/2026-09-07-replay-loop-iter0.md`.

## Configuration

| | |
|---|---|
| Decks | `decks/kennen_tyler.txt` vs `decks/rengar_test.txt` |
| Agents | `mcts:sims=20` both seats, score evaluator (the spec's baseline; `eval=corpus` is NOT used here) |
| Games | 200 with Kennen in seat P1 (seed 2000), 200 with Kennen in seat P2 (seed 3000) |
| Threads | 4 |
| Logs | `--decision-log` JSON per game, kept OUTSIDE the repo (~300–500 KB each); only `summary.json` / `summary.md` from `scripts/analyze_batch.py` are committed here |
| Binary | built from the tree of commit `b1f2a89` (the version string inside the logs reads `cae7532-dirty` because the build ran while L0b was still uncommitted; the code is L0b's — verified by the controller's 8-game smoke on the same binary: Validity OK, longest identical run 2) |

## Why this batch is the FIRST valid one for Kennen

Two earlier attempts at this batch were killed as invalid. Both were
engine holes that the loop itself surfaced, and both sat inside Tyler's
deck:

1. **Closed-state `[Reaction]` ability activations were never executed**
   (`8b26d29`, spec addendum #15). Seal of Discord's `[E]: [Reaction] —
   Add [P]` was offered, chosen, and silently dropped, up to 93 times per
   priority window. Kennen's power engine was dead in the closed state.
2. **Main-phase equip offers had no affordability check and a rejected
   equip was silent** (`b1f2a89`, spec addendum #16). One unpayable Last
   Rites was re-chosen 497 times in a row and, against the 500-action
   turn cap, consumed Kennen's ENTIRE turn — five turns in seed 2000.
   The registry-wide guard test written for the fix caught a third bug
   on the way: the three `UniversalEquipGear` cards attached with an
   empty base, paying nothing.

`scripts/analyze_batch.py` now leads every summary with a **Validity**
section that flags frozen-state bursts (`cae7532`), so the next hole of
this kind shows up in the summary instead of in a manual dig.

Every earlier Kennen batch in this repo (the 24-game pool in
`2026-09-07-kennen-vs-rengar.md`, the corpus A/B in
`2026-09-08-corpus-ab.md`) is contaminated wherever either hole was
live. Their numbers are engine evidence, not deck evidence.

## Results

| | Games | Kennen wins | Wilson 95% |
|---|---|---|---|
| Kennen seat P1 (seed 2000) | 200 | 96 | 48.0% [41.2–54.9] |
| Kennen seat P2 (seed 3000) | 200 | 90 | 45.0% [38.3–51.9] |
| Both | 400 | 186 | 46.5% [41.7–51.4] |

Analyzer Validity: OK in both seats (longest identical decision run 2,
threshold 10). Mean game length 16.0 turns. Files: `summary-kennenP1.*`,
`summary-kennenP2.*` (from `scripts/analyze_batch.py`), `deep-analysis.md`
(from `deep_kennen.py` here — a Kennen-centred read of the same logs).

The playbook derived from them lives in personal-ai's vault
(`docs/knowledge/riftbound/playbooks/kennen-vs-rengar.md` with its
`.prior.json` twin), which is the single source of truth; this directory
holds only the batch record. The L5 acceptance run (prior-injected Kennen,
same seeds, same binary) is recorded in `acceptance.md` when it lands.
