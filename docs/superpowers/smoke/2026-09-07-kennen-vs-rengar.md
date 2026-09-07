# Kennen vs Rengar smoke self-play — 2026-09-07

Task 10 of `.superpowers/sdd/2026-09-07-kennen-tyler-deck/`: 10-game smoke
self-play batch plus final full-suite/coverage verification.

## Binary and command

Binary rebuilt at HEAD `b33a1ad` (06:44 UTC, per the controller's pre-check
note). This session's `git status --short` was clean at the time of the
run and `git diff --stat b33a1ad 321c301 -- src/` is empty, so the one
docs-only commit that landed on top of `b33a1ad` (spec addendum #12,
`321c301`) does not require a rebuild; `./build/riftbound` used for every
game below was built at `b33a1ad` and remains valid at the repo's current
HEAD, `321c301`.

Command shape (Kennen as deck1/P1, seeds 1–5):

```
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
  --seed <N> --render-html on
```

Command shape (seats swapped — Rengar as deck1/P1, Kennen as deck2/P2 —
seeds 11–15, same sim count):

```
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/rengar_test.txt --deck2 decks/kennen_tyler.txt \
  --seed <N> --render-html on
```

All 10 games ran sequentially, well under the 10-minute kill threshold
(each finished in well under a minute of wall time). No game timed out.

Markers were counted by grepping each game's `replays/<ts>/replay.html`
(stdout carries no engine trace lines — confirmed again this run: every
game's stdout was just the `P1:`/`P2:`/`Seed:`/`Game over:`/`Replay
saved to` lines).

## Results (10 games)

| # | Seed | Seats (P1/P2) | Winner (by deck) | Reason | Turns | FLOW trash | FLOW banished | EMPOWER | DISEMPOWER | ACTIVATE_COST disempower | BURN | KENNEN gains Flow | Sandswept/TOMB/restricted anomaly lines |
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
| 11* | 7 (30 sims, controller pre-check) | Kennen/Rengar | Kennen | P1 reached 8 points | 12 | 2 | 2 | 4  | 4 | 4 | 2 | 1 | 0 |

\* Row 11 is the controller's pre-check game (mcts:sims=30 both seats,
same binary at `b33a1ad`), reproduced from
`.superpowers/sdd/2026-09-07-kennen-tyler-deck/smoke-precheck-seed7.md`
for reference; it is not part of the 10-game sims=50 batch totals below.

Games 4 and 14 each played and used the Sandswept Tomb battlefield card
(id 792); its board-state text ("Sandswept Tomb  [Ctrl:--] ...") then
recurs in every subsequent board snapshot embedded in those two replay
HTMLs, which makes a naive `grep -o` count balloon into the hundreds —
that is normal per-turn board rendering, not repeated restricted-play
events. The actual restricted-play anomaly signal is the engine's
`TOMB: illegal restricted intent for ...` warning (`game_engine.cpp`);
`grep -c 'TOMB:'` and `grep -c 'illegal restricted intent'` both return
**0** for all 10 games, which is what the "Sandswept/TOMB/restricted
anomaly lines" column above reports.

### Batch totals (10 games, sims=50 both seats)

- Winner by deck: **Kennen 6 — Rengar 4**
- `FLOW: ... played from trash`: 6
- `FLOW: ... banished`: 6
- `EMPOWER:`: 56
- `DISEMPOWER:`: 28
- `ACTIVATE_COST: disempower`: 28
- `BURN:`: 32
- `KENNEN: ... gains Flow`: 5
- Sandswept/TOMB/restricted anomaly lines (`TOMB:` warning or `illegal
  restricted intent`): 0
- `WRN` / `warn` / `assert` / `exception` / `illegal` lines anywhere in
  any of the 10 replay HTMLs: **0** (checked with
  `grep -o 'WRN[^<"]*\|warn[^<"]*\|assert[^<"]*\|exception[^<"]*\|illegal[^<"]*'`
  against every replay — no matches in any game)

No exception, assertion, crash, or warning line appeared in any of the
10 games. The batch is **not blocked**.

## Honest framing

This is a smoke check of the engine, not a strength measurement: MCTS
at 50 simulations plays legal, not expert, Riftbound; the agent also
cannot distinguish printed-vs-granted Flow offers or restricted-vs-plain
Tomb offers for the same card (known action-vocabulary aliasing), so
results understate the value of those choices.

## Final verification

`RIFTBOUND_ROOT=. ./build/riftbound_tests 2>&1 | tail -3`:

```
[----------] Global test environment tear-down
[==========] 1121 tests from 121 test suites ran. (1939 ms total)
[  PASSED  ] 1121 tests.

  YOU HAVE 1 DISABLED TEST
```

`python3 scripts/card_coverage.py | tail -2`:

```
TOTAL                                          792

INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

Both match the expected numbers (1121 passed / 1 pre-existing disabled;
792 total cards / 0 INCOMPLETE).
