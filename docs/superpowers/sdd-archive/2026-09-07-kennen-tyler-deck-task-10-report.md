# Task 10 report: Smoke self-play and reporting

## Status: DONE

## Setup verified

- Branch: `kennen-tyler-deck` (confirmed via `git branch --show-current`).
- `git status --short` clean before the run; source HEAD at start `b33a1ad`.
- One docs-only commit (`321c301`, spec addendum #12, `.md` file only)
  landed on top of `b33a1ad` before this task ran. `git diff --stat
  b33a1ad 321c301 -- src/` is empty, so the already-built
  `build/riftbound` (06:44 UTC, at `b33a1ad`) remained valid — no rebuild
  performed. Repo HEAD at commit time: `321c301` → then `0f993a9` (this
  task's commit).
- `build/riftbound`, `build/riftbound_tests`, `decks/kennen_tyler.txt`,
  `decks/rengar_test.txt`, `scripts/card_coverage.py` all present and
  confirmed before running.

## Command shape

Seeds 1–5 (Kennen = deck1/P1):
```
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
  --seed <N> --render-html on
```

Seeds 11–15 (seats swapped, Rengar = deck1/P1, Kennen = deck2/P2):
```
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/rengar_test.txt --deck2 decks/kennen_tyler.txt \
  --seed <N> --render-html on
```

Run sequentially; every game finished in well under a minute of wall
time (no game approached the 10-minute kill threshold; none timed out).

## 10-game results table

| # | Seed | Seats (P1/P2) | Winner (by deck) | Reason | Turns | FLOW trash | FLOW banished | EMPOWER | DISEMPOWER | ACTIVATE_COST disempower | BURN | KENNEN gains Flow | Restricted-play (TOMB:/illegal restricted intent) |
|---|------|----------------|-------------------|--------|-------|-----------|---------------|---------|------------|--------------------------|------|--------------------|------------------------------------------|
| 1 | 1  | Kennen/Rengar | Kennen | P1 reached 8 points | 15 | 1 | 1 | 8  | 4 | 4 | 2 | 1 | 0 |
| 2 | 2  | Kennen/Rengar | Kennen | P1 reached 8 points | 10 | 0 | 0 | 4  | 2 | 2 | 2 | 0 | 0 |
| 3 | 3  | Kennen/Rengar | Rengar | P2 reached 8 points | 12 | 1 | 1 | 6  | 3 | 3 | 2 | 0 | 0 |
| 4 | 4  | Kennen/Rengar | Kennen | P1 reached 8 points | 17 | 0 | 0 | 4  | 2 | 2 | 4 | 1 | 0 |
| 5 | 5  | Kennen/Rengar | Kennen | P1 reached 8 points | 12 | 1 | 1 | 4  | 2 | 2 | 4 | 0 | 0 |
| 6 | 11 | Rengar/Kennen | Rengar | P1 reached 8 points | 11 | 0 | 0 | 4  | 2 | 2 | 2 | 0 | 0 |
| 7 | 12 | Rengar/Kennen | Rengar | P1 reached 8 points | 15 | 0 | 0 | 4  | 2 | 2 | 2 | 1 | 0 |
| 8 | 13 | Rengar/Kennen | Kennen | P2 reached 8 points | 10 | 0 | 0 | 6  | 3 | 3 | 4 | 1 | 0 |
| 9 | 14 | Rengar/Kennen | Rengar | P1 reached 8 points | 14 | 2 | 2 | 6  | 3 | 3 | 4 | 0 | 0 |
| 10 | 15 | Rengar/Kennen | Kennen | P2 reached 8 points | 16 | 1 | 1 | 10 | 5 | 5 | 6 | 1 | 0 |
| 11* | 7 (pre-check, 30 sims) | Kennen/Rengar | Kennen | P1 reached 8 points | 12 | 2 | 2 | 4 | 4 | 4 | 2 | 1 | 0 |

\* Row 11 reproduces the controller's pre-check (`smoke-precheck-seed7.md`,
mcts:sims=30 both seats) for reference; excluded from the totals below.

### Batch totals (10 games, sims=50)

- **Winner by deck: Kennen 6 — Rengar 4**
- FLOW played from trash: 6
- FLOW banished: 6
- EMPOWER: 56
- DISEMPOWER: 28
- ACTIVATE_COST: disempower: 28
- BURN: 32
- KENNEN gains Flow: 5
- Restricted-play (TOMB: warning / illegal restricted intent): 0

### Note on Sandswept Tomb

Games 4 (seed 4) and 9 (seed 14) each played the Sandswept Tomb
battlefield card (id 792). Its board-state text then recurs in every
later board snapshot embedded in those two replay HTMLs, so a naive
`grep -o` for "Sandswept" balloons into the hundreds of matches — that's
normal per-turn board rendering, not a repeated event. The engine's
actual anomaly signal for this mechanic is the `TOMB: illegal
restricted intent for ...` warning; `grep -c 'TOMB:'` and `grep -c
'illegal restricted intent'` both returned 0 for all 10 replay HTMLs.

## Anomalies

**None.** No exception, assertion, crash, or `WRN`/`warn`/`assert`/
`exception`/`illegal` line appeared in any of the 10 replay HTMLs
(checked with
`grep -o 'WRN[^<"]*\|warn[^<"]*\|assert[^<"]*\|exception[^<"]*\|illegal[^<"]*'`
against every game's replay.html — zero matches across all 10). Batch
was not stopped; nothing routed back to an owning task.

## Final verification

`RIFTBOUND_ROOT=. ./build/riftbound_tests 2>&1 | tail -3`:
```
[----------] Global test environment tear-down
[==========] 1121 tests from 121 test suites ran. (1939 ms total)
[  PASSED  ] 1121 tests.

  YOU HAVE 1 DISABLED TEST
```
Matches expected: 1121 passed, 1 pre-existing disabled.

`python3 scripts/card_coverage.py | tail -2`:
```
TOTAL                                          792

INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```
Matches expected: 792 total, INCOMPLETE 0.

## Commit

`git status --short` before staging showed only `docs/superpowers/smoke/`
untracked (replays/ and *.log correctly git-ignored, confirmed not
present in status). Staged and committed only
`docs/superpowers/smoke/2026-09-07-kennen-vs-rengar.md` (no `git add -A`
used).

Commit: `0f993a9` — "Kennen: smoke self-play notes"
```
1 file changed, 122 insertions(+)
create mode 100644 docs/superpowers/smoke/2026-09-07-kennen-vs-rengar.md
```

Notes file: `/home/user/chorlick/alpharune/docs/superpowers/smoke/2026-09-07-kennen-vs-rengar.md`

## Fix round 1 (coordinator-flagged, docs-only)

The `EMPOWER:` counting grep matched as a substring of `DISEMPOWER:`,
inflating every row's `EMPOWER` column to exactly 2× `DISEMPOWER` (batch
total 56 instead of the correct 28). Recounted every replay with
`grep -o '[^S]EMPOWER:'` (a real `EMPOWER:` trace line is never preceded
by `S`, whereas `DISEMPOWER:`'s embedded `EMPOWER:` always is) and
confirmed corrected EMPOWER counts equal DISEMPOWER counts exactly in
every one of the 10 games, as expected mechanically. Checked `FLOW:`
and `BURN:` for the same class of substring overlap: both are unaffected
(`FLOW:` has no colliding trace tag; `BURN:` is colon-anchored and does
not match the unrelated `BURN_OUT` token). Corrected the per-row column,
the batch total, and added a correction note to the notes file.

Commit: `7ac1aea` — "Kennen: smoke notes — fix EMPOWER double-count"
(notes file only; `git status --short` showed no other changes staged).
