# L1 — decision log writer + `--decision-log`: report

Branch `replay-loop`, commit `f657e57` ("Loop: decision log writer and
--decision-log"), on top of `818ef91` (the parallel L2 analysis-script
commit) / `dca2d02` (spec+plan) / `8006d38`.

Spec: `docs/superpowers/specs/2026-09-07-replay-loop-iter0-design.md`
("L1 — decision log"). Plan: `docs/superpowers/plans/2026-09-07-replay-loop-iter0.md`
("L1 — decision log writer + `--decision-log`").

## Files touched

- `src/io/decision_log_writer.h` (new)
- `src/io/decision_log_writer.cpp` (new)
- `src/engine/game_runner.h` — `GameConfig` gains `decision_log_dir`,
  `deck1_path`, `deck2_path`
- `src/engine/game_runner.cpp` — writer construction/hookup/finish
  beside `ReplayWriter`
- `src/main.cpp` — `--decision-log <dir>` flag, single-game-path
  wiring, help text
- `CMakeLists.txt` — added `src/io/decision_log_writer.cpp` to
  `riftbound_core` (this file list is explicit, not globbed)
- `tests/test_decision_log.cpp` (new)

No card, rules, or core-header files touched. No files under `scripts/`
or `tests/analysis/` touched or run.

## Schema

Top-level JSON object per game file: `schema`, `decks`, `agents`,
`seed`, `seats`, `engine_version` (header) → `log` (the array of
per-decision records — named `log`, not `decisions`, since the footer
already needs a key literally named `decisions` for the total decision
COUNT, and a JSON object can't use one key twice) → `winner`, `reason`,
`turns`, `final_scores`, `decisions` (footer, spliced onto the
top-level object, no nested "footer" key). Full shape documented in the
header comment of `decision_log_writer.h`.

**Cross-check with the parallel L2 script**: `scripts/analyze_batch.py`
landed on this branch (commit `818ef91`) while I was building this, and
its docstring — independently derived from the same spec text — names
the schema field-for-field identically to what I'd already built
(`log`, the `phase{phase,ns_state,oc_state}` shape, `battlefields[].{id,
name, controller, contested, units:{p1:{count,might},p2:{...}}}`,
`players[].{seat,hand,deck,trash,banishment,runes_ready,
runes_exhausted,legend_empowered}`, and the exact footer keys). I did
not need to change anything to match it. Its own docstring says it was
"cross-checked ... against the real `decision_log_writer.cpp`" — so
this is now a mutually-verified contract, not just my guess.

`root_value` is always `null` today: `MctsAgent` (src/agents/mcts_agent.cpp)
computes a root value internally (`root->total_reward / root->explore_count`)
but exposes no accessor, and that file is out of scope for L1 (not in
the allowed file list, no core-header edits). The writer has a working
`setNextRootValue()` one-shot-pending mechanism (mirrors
`ReplayWriter::setNextWinProb`) ready for whenever an agent-side change
wires it up — no call site does yet.

## TDD evidence

`tests/test_decision_log.cpp`, three tests per the plan:

1. **`DecisionLogTest.TwoDecisionsAndFooterProduceValidCompleteJson`** —
   feeds two hand-built decisions (via `CardTestFixture`) and a footer;
   parses the result and asserts every field in the spec's contract is
   present with the right type/value, including `players[].runes_ready`
   /`runes_exhausted` counts, `battlefields[].units`, and that no
   `truncated` marker leaked into a cleanly-finished file.
2. **`DecisionLogTest.FlowGrantedAndRestrictedBattlefieldSerialize`** —
   an `Intent` with `flow_source = Granted` and
   `target_battlefield_restriction = 1` round-trips both.
3. **`DecisionLogEndToEnd.TwoGamesViaGameRunnerProduceTwoParseableFiles`** —
   drives `GameRunner` directly (random vs random, `kennen_tyler.txt` /
   `rengar_test.txt`) for 2 games with `decision_log_dir` set; asserts
   two files exist at the exact `game_<i>_seed_<seed>.json` paths with
   seeds matching the runner's own `base_seed + i` formula, both
   parseable, both carrying full footers.

**RED observed before GREEN, all three tests, by mutation** (not by
writing tests against not-yet-written code, since the writer was built
alongside them — so I verified failure capability directly): temporarily
broke `actor` (wrong constant), `flow_source` (hard-coded to `None`),
and `finish()` (early-returned, skipping the footer), rebuilt, and
confirmed all three tests failed with the expected mismatches:

```
DecisionLogTest.TwoDecisionsAndFooterProduceValidCompleteJson: FAILED
  actor: "BROKEN_FOR_RED_CHECK" != "P1"/"P2"; winner missing
DecisionLogTest.FlowGrantedAndRestrictedBattlefieldSerialize: FAILED
  flow_source: "None" != "Granted"
DecisionLogEndToEnd.TwoGamesViaGameRunnerProduceTwoParseableFiles: FAILED
  winner missing
```

Reverted, rebuilt, all three GREEN again. (Full transcript ran in this
session; not re-pasted here in full to keep this report short — the
three failures above are the literal `gtest` failure lines observed.)

## Full suite

```
RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1202 tests from 129 test suites ran. (2075 ms total)
[  PASSED  ] 1202 tests.
  YOU HAVE 1 DISABLED TEST
```
1199 baseline + 3 new = 1202; 1 pre-existing disabled test, unchanged.
Coverage gate (792 registered card slots via `test_action_vocab.cpp`)
untouched — no card/rules files were modified, and the binary's own
`Loaded 792 cards from classes` banner confirms the count didn't move.

## Smoke-seed identity check

```
./build/riftbound --agent1 mcts:sims=20 --agent2 mcts:sims=20 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt --seed 4242
  → Game over: winner=1 | reason=P1 reached 8 points | turns=14

./build/riftbound --agent1 mcts:sims=20 --agent2 mcts:sims=20 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt --seed 4242 \
  --decision-log /tmp/dl
  → Game over: winner=1 | reason=P1 reached 8 points | turns=14
```
`diff` of the two `Game over` lines: **identical**. Adding
`--decision-log` does not perturb RNG consumption, agent decisions, or
the outcome — the writer only reads `GameState`/`Intent`, never
mutates or draws from any RNG.

## Concern: file size exceeds the 200 KB target

`game_0_seed_4242.json` is **332,819 bytes (≈325 KB)** — over the
spec's "≤200 KB per game" target and the smoke check's "< 200 KB" bar.
This is the one requirement I could not satisfy without either (a)
breaking the schema now mutually cross-validated against
`scripts/analyze_batch.py`, or (b) silently degrading data on exactly
the decisions the whole loop cares about (the mistake-candidate board
summaries) via delta/omission encoding the consumer doesn't understand
(it does `.get("battlefields") or []`, so an omitted key reads as "zero
battlefields," not "unchanged" — actively misleading, not just sparse).
I chose not to do either unilaterally, and instead measured and report
this plainly rather than force a fake pass. I checked, root-caused, and
tried the one contract-safe reduction before concluding this:

- Root cause: this matchup has real per-game decision counts far above
  what a ~200 KB budget assumes at full per-decision fidelity. The
  smoke seed has **405 decisions** for 14 turns — Kennen's chain/
  reaction design (per `CLAUDE.md`: "faithful Empower/Flow,
  reaction-path fixes") means most of those are `MakeChoice` (110),
  `PassPriority` (104), and `ActivateReactionAbility` (50), each still
  carrying a full per-decision snapshot per spec ("one record per
  decision", no type-based filtering requested). I confirmed this is a
  real engine property, not something my code introduced: the existing
  HTML replay path (`--render-html on`) produces 520 snapshots for the
  same seed (decisions + phase seams), same order of magnitude.
- This isn't specific to MCTS or this one seed either — I ran a 4-game
  `random` vs `random` batch on the same decks (seed 100,
  `--games 4 --threads 4 --decision-log`) and got 261–348 decisions and
  212–282 KB per file. L3's planned batch (`mcts:sims=20` both seats,
  same decks) will hit the same wall at scale.
- I traced the byte budget: `battlefields` (35%) and `players` (31%)
  dominate; `chosen` (16%, contractually fixed at
  `type/card/source_zone/flow_source/restricted_bf/destination` — can't
  shrink) and `phase` (9%) are smaller and shape-fixed by the (now
  cross-validated) contract. The only genuinely safe reduction I found
  — dropping `battlefields[].name` (the analysis script already falls
  back to `bf.get("id")` when absent) — saves only ~18 KB, nowhere
  near the ~125 KB needed, so I didn't apply it as a half-measure that
  changes the shape without fixing the actual problem.
- Recommendation, not implemented here (out of L1's TDD-scoped file
  list and the plan's explicit non-goals): either raise the per-game
  target for reaction-heavy decks, or a follow-up could delta-encode
  `battlefields`/`players` (skip re-emitting when byte-identical to the
  prior record) — but that requires `scripts/analyze_batch.py` to grow
  "inherit from the last record that had this key" semantics first, so
  it's a joint L1/L2 change, not something to do unilaterally on one
  side.

Everything else the smoke check asked for passed: identical outcome,
valid JSON, correct schema, correct seed in both the filename and the
`seed` field.

## Other things verified beyond the letter of the task

- Ran a real 4-game, 4-thread batch with `--decision-log` (not just the
  single-threaded unit test) to make sure concurrent `GameRunner`
  instances don't race on directory creation. They safely can (POSIX
  `mkdir` + `EEXIST` tolerance), but I removed the risk anyway: batch
  mode now creates `decision_log_dir` once in `main.cpp` before
  `BatchRunner::runBatch` starts its thread pool, rather than relying
  on N concurrent `create_directories` calls inside the writer's
  constructor.
- All 4 batch-mode files parsed, had valid footers, and correct
  per-game seeds (`base_seed + game_index`).
