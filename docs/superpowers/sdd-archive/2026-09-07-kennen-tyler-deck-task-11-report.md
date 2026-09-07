# Task 11 report: action vocabulary — new cards and distinct Flow / Tomb offers

## Status: DONE

Branch `kennen-tyler-deck`, base `da9a9a2` (confirmed via `git log -1
--oneline` before starting and again before committing — unchanged; no
other agent landed a commit on this branch during the session).

## Setup verified

- `git status --short` clean at start.
- `build/` pre-existed and was current for `riftbound_tests` at start;
  `riftbound` (the CLI binary) was rebuilt for the smoke step since
  `action_vocab.h` is transitively included by `mcts_agent.cpp` /
  `riftbound_game.cpp`.

## Files changed

- `src/openspiel/action_vocab.h` — `kNumCardDefIds` 787 → 792 (with an
  updated doc comment naming the guard test); three new `ActionVerb`
  entries (`PlayFlowPrinted`, `PlayFlowGranted`, `PlayTombRestricted`,
  each arity `kNumCardDefIds`) appended at the end of the enum, before
  `Count`; `verbArity` cases for the three new verbs; top-of-file doc
  comment updated to mention the new verbs and to stop hardcoding `787`
  in the "Play → card_def_id (1..787)" line.
- `src/openspiel/action_vocab.cpp` — the collapsed Play-family case
  (`PlayCard`/`PlayActionCard`/`PlayReaction`) now picks a verb based on
  `intent.target_battlefield_restriction` (→ `PlayTombRestricted`, checked
  first) then `intent.flow_source` (`Printed` → `PlayFlowPrinted`,
  `Granted` → `PlayFlowGranted`), falling back to the original `Play`
  verb otherwise. Precedence (Tomb wins when both are set) is documented
  in a comment at the case.
- `tests/test_action_vocab.cpp` — added `#include "cards/card_registry.h"`
  + `#include "core/card_db.h"`; `SlotsInRange` now uses `kNumCardDefIds`
  instead of the literal `787`; five new tests (below); updated
  `VocabSizeIsReasonable`'s comment (no numeric pin existed elsewhere in
  the repo — confirmed by grep across `src/` and `tests/` for `787` and
  `kVocabSize`; the only other `787`s are unrelated per-card test
  constants like `kVoidreaver = 787`, left untouched).
- `src/openspiel/riftbound_game.cpp` — **not touched**; `NumDistinctActions()`
  already returns `kVocabSize` directly, which grows automatically.

## TDD log (RED → GREEN per test, full suite green at the end)

### 1. Registry-size guard — `ActionVocab.RegistrySizeMatchesConstant`

RED (before bumping `kNumCardDefIds`):
```
Expected equality of these values:
  kNumCardDefIds
    Which is: 787
  static_cast<int>(db.size())
    Which is: 792
kNumCardDefIds (action_vocab.h) is 787 but the registry now holds 792 cards...
[  FAILED  ] ActionVocab.RegistrySizeMatchesConstant
```
GREEN after bumping `kNumCardDefIds` to 792 (and updating `SlotsInRange`'s
`787` literal to `kNumCardDefIds`): 24/24 `ActionVocab*` tests passed.

### 2. Encode tests — distinct slots for the four offer shapes

Added `ActionVocab.FlowAndTombOffersEncodeToDistinctSlots` (plain /
Printed-Flow / Granted-Flow / Tomb-restricted for one card, asserts all
four slots pairwise distinct, all `< kVocabSize`, and the plain slot
equals `verbOffset(ActionVerb::Play) + 99` for `def_id=100` — the
pre-change value, confirmed by probing the pre-change binary:
`verbOffset(Play)=154`, so `154+99=253`) and
`ActionVocab.RestrictedFlowOfferPrefersTombVerb` (both fields set → same
slot as Tomb-restricted-only, documenting the precedence).

RED (encode logic not yet changed — all four intents still collapsed
onto slot 253, the pre-existing bug reproduced live):
```
Expected: (plain_slot) != (printed_slot), actual: 253 vs 253
Expected: (plain_slot) != (granted_slot), actual: 253 vs 253
Expected: (plain_slot) != (restricted_slot), actual: 253 vs 253
Expected: (printed_slot) != (granted_slot), actual: 253 vs 253
Expected: (printed_slot) != (restricted_slot), actual: 253 vs 253
Expected: (granted_slot) != (restricted_slot), actual: 253 vs 253
[  FAILED  ] ActionVocab.FlowAndTombOffersEncodeToDistinctSlots
[       OK ] ActionVocab.RestrictedFlowOfferPrefersTombVerb   (trivially — both sides used the same fallback verb)
```
GREEN after adding the three verbs + the verb-selection logic in
`encodeAction`'s Play-family case: 26/26 `ActionVocab*` passed.

### 3. Decode round-trip — `ActionVocab.DecodeDistinguishesFlowAndTombOffersInLegalList`

Built a legal-action list of all four offers (plain listed FIRST,
reproducing the shape that used to always win under first-match decode)
and asserted `decodeAction` returns the intent matching each of the four
distinct slots, checking `flow_source` / `target_battlefield_restriction`
on the decoded intent. This test was added and built AFTER the encode
change already landed (encode and decode share the same underlying fix —
`decodeAction`'s shape is unchanged, it already does a first-match scan
over `encodeAction` — so there was no separate RED phase for decode
specifically; the four-slot RED above IS the decode bug's root cause).
Passed immediately: 27/27 `ActionVocab*` passed.

## Pre/post `kVocabSize`

Probed via a standalone translation unit compiled against
`action_vocab.h` directly (`g++ -std=c++20 -I src -I .`), both before and
after the change:

| | `kNumCardDefIds` | `verbOffset(Play)` | `kVocabSize` |
|---|---|---|---|
| pre-change (HEAD `da9a9a2`) | 787 | 154 | 9615 |
| post-change | 792 | 154 (unchanged — Play precedes every def-id-sized verb in the enum other than itself, so its offset doesn't shift) | 12051 |

Growth: `12051 - 9615 = 2436 = 3 × 792 (new verbs) + 40 (8 existing
def-id-scaled verbs × 5-card bump) + 20 (ActivateAbility's
`kNumCardDefIds * kMaxAbilitiesPerCard` scaling × 5-card bump)` — checks
out exactly against the arithmetic, confirming no stray verb was missed.

No trained AlphaZero policy head exists in this fork (`src/ml/` has no
saved checkpoint referencing a fixed action-space size), so this growth
is acceptable, per the brief.

**Known, out-of-scope divergence surfaced during this task:**
`src/ml/feature_extractor.h`'s `kCardVocabSize = 787` is a SEPARATE
constant (not `action_vocab.h`'s `kNumCardDefIds`, no shared definition)
used for observation-tensor feature encoding. It is now stale relative
to the registry (792 cards) but was explicitly out of the brief's file
list (`src/ml/` isn't `action_vocab.*`/`riftbound_game.cpp`/tests) and
touching it would be an engine/feature-pipeline change beyond this
task's scope. Flagging it loudly rather than fixing it silently or
ignoring it: cards 788–792 (Kennen, Lightning Rush, Up from the Deep,
Heart of the Tempest, Sandswept Tomb) are invisible to any observation
tensor built by `FeatureExtractor` today. No test currently pins
`kCardVocabSize` against the registry the way the new
`RegistrySizeMatchesConstant` test now pins `kNumCardDefIds` — worth a
follow-up task if the ML pipeline is ever exercised again for this
fork.

## MCTS smoke step

Rebuilt `riftbound` (`cmake --build build --target riftbound -j4`) after
the vocab change — required, since `mcts_agent.cpp` and
`riftbound_game.cpp` include `action_vocab.h`. Build succeeded cleanly.

**Discrepancy caught before running:** the brief names `--seed 2` and
`--seed 12` as "the games where the Tomb was live," but Task 10's own
report (`task-10-report.md`, "Note on Sandswept Tomb") states the Tomb
battlefield was actually played in games with **seed 4 and seed 14**,
not 2 or 12. Rather than guess which was intended, I ran all four seeds
(2, 4, 12, 14) using Task 10's exact command shape, seats included (2
and 4 with Kennen=deck1/P1; 12 and 14 respectively using the seat
convention Task 10 used for that seed's row — 14 keeps Kennen=P1 like
its row 9, 12 swaps seats like Task 10's seed-11-15 block).

Commands run:
```
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
  --seed 2 --render-html on
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
  --seed 4 --render-html on
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/rengar_test.txt --deck2 decks/kennen_tyler.txt \
  --seed 12 --render-html on
./build/riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
  --seed 14 --render-html on
```
All four completed normally (no crash, no timeout; `Loaded 792 cards
from classes` confirms the rebuilt binary is running against the bumped
registry constant).

Grep results verbatim (`grep -c PATTERN replays/<run>/replay.html`; a
missing `grep -c` line below the `===` header means 0 matches, since
`grep -c` exits 1 on zero matches and stopped the `&&`-chained command —
re-run individually and confirmed 0 for all four):

```
=== seed2 (20260907-094057) ===
granted: 0
TOMB: 0
restricted: 0
printed flow: 4
=== seed4 (20260907-094228) ===
granted: 0
TOMB: 0
restricted: 0
printed flow: 1
=== seed14 (20260907-094426) ===
granted: 0
TOMB: 0
restricted: 0
printed flow: 1
=== seed12 (20260907-094534) ===
granted: 0
TOMB: 0
restricted: 0
printed flow: 0
```

**Honest read:** across all four seeds, `(granted)` (the Flow-trace
string for a granted-Flow play — `game_engine.cpp:1800-1803`,
`"... (" + (flow_source==Granted?"granted":"printed") + ")"`) never
appears, and neither does `TOMB:` (the illegal-restricted-intent warning
— its absence is a GOOD sign, meaning no illegal restricted intent was
ever attempted, not evidence either way about whether a legal restricted
offer was taken) or a bare `restricted` string (there is in fact no
success-path trace log for a *chosen* Tomb-restricted play anywhere in
the engine — I checked `game_engine.cpp`, `chain_manager.cpp`, and
`card.cpp`; the only log lines touching "restricted" are code comments,
not runtime trace/warn output). `(printed)` (the ordinary printed-Flow
trace) DOES appear 1-4 times per game that plays a Flow spell from
trash, confirming the FLOW trace path itself still fires correctly.

So: **the fix is verified correct at the unit level (encode/decode now
produce four genuinely distinct, round-trippable slots), and the CLI
binary that consumes it was rebuilt and ran without incident, but the
smoke seeds gave no direct textual evidence that the MCTS agent
*prefers* a granted-Flow or Tomb-restricted offer over the plain one in
these four particular games** — consistent with the brief's own caveat
("the agent may still not prefer them"). Confirming agent *preference*
would need either more seeds, a scripted agent that forces the choice,
or a dedicated trace line for a successfully-taken restricted play
(none exists today — flagged above as a possible follow-up, not built
here since it's outside this task's scope).

## Full suite (from repo root, before committing)

```
RIFTBOUND_ROOT=. ./build/riftbound_tests
...
[==========] 1143 tests from 123 test suites ran. (2001 ms total)
[  PASSED  ] 1143 tests.

  YOU HAVE 1 DISABLED TEST
```
Baseline was 1139 passed / 1 disabled; this task added exactly 4 new
tests (`RegistrySizeMatchesConstant`,
`FlowAndTombOffersEncodeToDistinctSlots`,
`RestrictedFlowOfferPrefersTombVerb`,
`DecodeDistinguishesFlowAndTombOffersInLegalList`) →
1139 + 4 = 1143, matching exactly. HEAD was still `da9a9a2` at commit
time (checked again immediately before `git commit`) — no other agent
landed a commit on this branch during the session.

## Concerns

1. **`src/ml/feature_extractor.h`'s `kCardVocabSize = 787` is now stale**
   relative to the 792-card registry (see above) — out of this task's
   scope but flagged for whoever next touches the ML feature pipeline.
2. **The brief's smoke-seed claim (`--seed 2`/`--seed 12` = "the Tomb
   games") does not match Task 10's own report** (which names seed 4 and
   14). I ran all four rather than guess; noted here rather than silently
   substituting seeds without saying so.
3. **No engine trace line exists for a successfully-taken Tomb-restricted
   play** (only for the illegal-intent case), so the smoke step's honest
   answer is "no textual evidence either way" rather than a clean
   yes/no — a future task wanting to confirm agent *preference* for
   these offers will need a new log line or a scripted-agent test, not
   just more `--seed` values.
4. No engine or card files were touched; no core-header edits beyond the
   allowed `action_vocab.h`/`.cpp`.
