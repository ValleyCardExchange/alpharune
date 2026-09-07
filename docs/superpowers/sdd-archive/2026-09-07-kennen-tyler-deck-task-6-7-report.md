# Task 6/7 report — Burn N + burn-out refactor; revealAndChoose rest-destination

Branch `kennen-tyler-deck`, base HEAD `c493501`. Two commits:
`ba96b59` (Task 6), `06ab7c1` (Task 7).

## Task 6 — Burn N helper with shared burn-out

Files: `src/engine/effect_executor.cpp`, `src/engine/game_engine.cpp`,
`tests/cards/test_kennen_cards.cpp` (new). No headers.

### Implementation

`EffectExecutor::burnOut(player)` is now the single implementation of
CR 431.2/431.3: no-op if trash is empty; otherwise recycle trash into
deck (`zone = MainDeck`, append, clear trash), shuffle if `rng_` is set,
opponent `score++`, log `BURN_OUT: ...`, and on victory-score reach with
more points than the burner: `game_over = true`, `winner`,
`game_over_reason`, `emit(GameOverEvent{...})` — same fields/emit as both
prior inline copies, verified against `test_burn_out_and_scoring.cpp`.

`EffectExecutor::drawCards`'s inline block was replaced with: `if
(ps.trash.empty()) break;` then `burnOut(player); if (state_.game_over)
return; if (ps.main_deck.empty()) break;` — behaviorally identical to
the old inline code (same order of operations, same early return).

`EffectExecutor::burnCards(player, count)` (new): loop `count` times;
if the deck is empty, call `burnOut(player)`, return immediately if it
set `game_over`, and `break` if the deck is still empty (deck AND trash
both empty — CR 440.4's stop condition); otherwise pop `main_deck.back()`
(top card), set `zone = Trash` / `location = nullopt` (matches
`recycleCards`/`discardCards`/`killObject` precedent), push to trash,
log `BURN: <name>`.

`GameEngine::drawCards`'s inline block now delegates to
`effect_executor_->burnOut(player)` when the executor is set. See the
caller-safety finding below for why a fallback branch also exists.

### drawCards-caller safety check (the brief's explicit ask)

The brief listed 4 call sites of `GameEngine::drawCards` itself
(444/446, 815, 2076, 4021) and asked me to verify `initSubsystems()` has
run before each. All 4 are only reachable through `drawOpeningHands`,
`drawPhase`, mulligan resolution, and `scoreConquer` — and in the real
`runGame()`/`resumeFromSnapshot()` flow, `initSubsystems()` runs first
(lines 83/185) before any of those are called. **That part checks out.**

But the brief also said "if any caller can run with a null executor, say
so" — and one does, one level up: `tests/cards/test_burn_out_and_scoring.cpp`'s
`BurnOutScoringTest::makeEngineAndSeedBattlefields()` constructs a bare
`GameEngine` and never calls `initSubsystems()` (or
`testHook_initSubsystems()`), then several tests call
`testHook_scoreConquer()` / `testHook_drawPhase()` directly — bypassing
`runGame()` entirely. `ScoreConquer_AtPenultimate_NotAllBFsScored_
DrawWithEmptyDeck_TriggersBurnOut` specifically drives P1's deck to empty
with a non-empty trash, forcing the burn-out branch.

**I verified this empirically, not just by inspection**, per this repo's
own "subagent reports are claims, not facts" rule: I did the naive
`effect_executor_->burnOut(player)` delegation with only an `assert()`
guard (matching the brief's literal suggestion) and ran that one test in
isolation:

```
$ ./build/riftbound_tests --gtest_filter='BurnOutScoringTest.ScoreConquer_AtPenultimate_NotAllBFsScored_DrawWithEmptyDeck_TriggersBurnOut'
[ RUN      ] BurnOutScoringTest.ScoreConquer_AtPenultimate_NotAllBFsScored_DrawWithEmptyDeck_TriggersBurnOut
Segmentation fault
$ echo $?
139
```

The `assert()` alone does not help: this project's test build is
`CMAKE_BUILD_TYPE=Release` (confirmed in `build/CMakeCache.txt` —
`-DNDEBUG` on all of Release/RelWithDebInfo/MinSizeRel), so `assert()`
compiles out and the null-`unique_ptr` dereference segfaults instead.
Since `test_burn_out_and_scoring.cpp` is outside my allowed file list, I
could not add `testHook_initSubsystems()` to that fixture to fix the gap
at its source.

**Resolution:** `GameEngine::drawCards` checks `effect_executor_` and,
only in the null case, constructs a throwaway `EffectExecutor` bound to
the engine's own `state_`/`rng_` and calls `.burnOut()` on *that*. This
is not a second copy of the burn-out **logic** — `burned_out = true`
still appears exactly once in the whole codebase (grep below) — it's a
second *instance* of the one class that owns it, used only when the
persistent instance isn't ready yet. The `assert(false, ...)` stays in
that branch as a loud (debug-build) signal that this path was hit,
exactly per the brief's ask, with the difference that release builds now
degrade safely instead of crashing. Re-ran the specific test after this
fix: green (see full suite run below).

**Concern for the controller:** this is a real gap in the existing test
fixture (constructing a `GameEngine` and calling internal test hooks
without `initSubsystems()`), not something Task 6 created — the prior
inline duplicate code just happened not to need `effect_executor_`. If a
future refactor makes any other `GameEngine::drawCards`-reachable code
path depend on `effect_executor_`, the same fixture gap will bite again;
the durable fix is `testHook_initSubsystems()` inside
`makeEngineAndSeedBattlefields()`, which I did not make since it's
outside this task's file list.

### TDD — RED/GREEN

Stashed only the two source files (`git stash push --
src/engine/effect_executor.cpp src/engine/game_engine.cpp`, leaving the
new untracked test file in place) to run tests against the original
stubs, then `git stash pop` to restore the implementation.

RED (stubs):
```
[ RUN      ] KennenCardsTest.BurnCards_TopTwo_GoToTrash_OrderPreserved
test_kennen_cards.cpp:32: Failure
Expected equality of these values:
  state.player(P1).trash.size()  Which is: 0
  2u                              Which is: 2
[  FAILED  ]
[ RUN      ] KennenCardsTest.BurnCards_EmptyDeckMidBurn_BurnsOutThenContinues
test_kennen_cards.cpp:77: Failure  burned_out: Actual false, Expected true
test_kennen_cards.cpp:79: Failure  P2 score: 2 vs 3
test_kennen_cards.cpp:85: Failure  trash.size(): 2 vs 1u
[  FAILED  ]
```

GREEN (implementation restored):
```
[ RUN      ] KennenCardsTest.BurnCards_TopTwo_GoToTrash_OrderPreserved
[       OK ] (3 ms)
[ RUN      ] KennenCardsTest.BurnCards_EmptyDeckMidBurn_BurnsOutThenContinues
[       OK ] (1 ms)
```

`BurnOutScoringTest.*` (18 tests, the refactor's regression guard) and
`VoidTest.*` (3 tests, `revealAndChoose` peek precedent) re-run green
after the refactor, including the null-executor test discussed above.

Full suite at the Task 6 commit: `1089 passed` (1087 existing + 2 new),
1 pre-existing disabled.

## Task 7 — revealAndChoose rest-destination

Files: `src/engine/effect_executor.cpp`,
`tests/cards/test_kennen_cards.cpp`. No headers (the `RestDestination`
enum and the parameter already existed from Task 1).

### Implementation

`revealAndChoose`'s per-card "draw or skip" branch gains an `else if
(rest == RestDestination::Trash)` arm between the existing "draw"
branch and the existing "recycle to bottom" `else`: sets `zone =
Trash`, `location = nullopt`, pushes to `ps.trash`, logs `CHOSE: trash
<name>`. Cards are processed in revealed order (`for (auto card_id :
revealed)`, unchanged), so trash receives them in that same order.
Every line of the pre-existing default (`Recycle`) path is untouched —
diff confirms it's a pure insertion, nothing on the `else` branch moved
or changed.

### TDD — RED/GREEN

Same stash technique, isolated to the `revealAndChoose` hunk this time
(reverted just the signature + added branch, kept Task 6's already-
committed code intact).

RED (parameter still ignored):
```
[ RUN      ] KennenCardsTest.RevealAndChoose_TrashRest_NonChosenGoToTrashInRevealedOrder
test_kennen_cards.cpp:128: Failure
Expected equality of these values:
  state.player(P1).trash.size()  Which is: 0
  2u                              Which is: 2
[  FAILED  ]
```
(`RevealAndChoose_DefaultRest_StillRecyclesToBottom` passed even
pre-fix, as expected — it only exercises the unchanged default path.)

GREEN:
```
[ RUN      ] KennenCardsTest.RevealAndChoose_TrashRest_NonChosenGoToTrashInRevealedOrder
[       OK ] (1 ms)
[ RUN      ] KennenCardsTest.RevealAndChoose_DefaultRest_StillRecyclesToBottom
[       OK ] (1 ms)
```

Test asserts: chosen card (`b`, the agent's "index 1" pick out of
revealed order `[C, B, A]`) lands in hand; `trash == [C, A]` in exactly
that revealed order with `zone`/`location` updated; `draws_this_turn ==
1`; exactly one `CardsDrawnEvent` with `count == 1` (subscribed via
`events.on_cards_drawn.connect`, precedent from `tests/test_chain.cpp`).

Full suite at the Task 7 commit (final state): `1091 passed` (1087
existing + 4 new), 1 pre-existing disabled.

## Files changed

- `src/engine/effect_executor.cpp` — `burnOut`, `burnCards`,
  `drawCards`'s delegation, `revealAndChoose`'s `Trash` branch.
- `src/engine/game_engine.cpp` — `drawCards`'s delegation to
  `effect_executor_->burnOut()` with the null-executor fallback.
- `tests/cards/test_kennen_cards.cpp` (new) — 4 tests: `#13`
  (`BurnCards_TopTwo_GoToTrash_OrderPreserved`), `#14`
  (`BurnCards_EmptyDeckMidBurn_BurnsOutThenContinues`), Task 7 Step 1
  (`RevealAndChoose_TrashRest_NonChosenGoToTrashInRevealedOrder`), plus
  a default-Recycle regression guard
  (`RevealAndChoose_DefaultRest_StillRecyclesToBottom`).

## Self-review

- **One burn-out copy remains**: `grep -rn "burned_out = true" src/`
  returns exactly one hit, `effect_executor.cpp:1238` (inside
  `burnOut`). ✔
- **Every `drawCards` caller safe**: the 4 call sites inside
  `game_engine.cpp` all run after `initSubsystems()` in the real
  `runGame`/`resumeFromSnapshot` flow. The one caller that ISN'T safe
  (a test fixture bypassing `initSubsystems()`) is handled by the
  same-class fallback described above and verified empirically, not
  silently patched over with duplicated logic. ✔ (with the concern
  above flagged for the controller)
- **Burn order = top first**: `burnCards` pops `main_deck.back()`
  (deck's top, per this codebase's convention) each iteration; test #13
  asserts `trash[0] == top`, `trash[1] == middle`. ✔
- **Revealed-order preserved into trash**: `revealAndChoose`'s existing
  `for (auto card_id : revealed)` loop order is untouched; the new
  branch only changes what happens to a non-chosen card, not the
  iteration order. Test asserts `trash == [C, A]` matching revealed
  order `[C, B, A]` minus the chosen `B`. ✔
- **Default Recycle path byte-identical**: diff on
  `src/engine/effect_executor.cpp` for Task 7 is a pure insertion (one
  `else if` arm) — the pre-existing `else` recycle branch has zero
  changed lines. `RevealAndChoose_DefaultRest_StillRecyclesToBottom`
  passed even before Task 7's code was applied (it only exercises the
  untouched path), confirming no accidental behavior change. ✔
- **Tests assert real deck/trash contents and scores**: every new test
  reads `state.player(...).trash`/`main_deck`/`score`/`burned_out`/
  `draws_this_turn` and `state.getObject(id).zone`/`.location`
  directly, plus one live `CardsDrawnEvent` subscription — no mocks,
  no return-value-only assertions. ✔

## Concerns

1. **Test-fixture gap in `test_burn_out_and_scoring.cpp`** (detailed
   above): `BurnOutScoringTest::makeEngineAndSeedBattlefields()`
   constructs a `GameEngine` and drives it through test-only hooks
   without ever calling `initSubsystems()`. Task 6's refactor exposed
   this as a real segfault risk (verified, not hypothetical) that I
   worked around at the `GameEngine::drawCards` call site rather than
   fixing at the source, since the test file was outside this task's
   file list. If Tyler wants the fixture itself fixed (adding
   `testHook_initSubsystems()` to that helper), that's a one-line
   change I did not make.
2. **`assert(false, ...)` in the fallback branch never fires in this
   project's Release/`-DNDEBUG` builds** (all of Release/RelWithDebInfo/
   MinSizeRel define `NDEBUG`; there's no separate Debug test target in
   this build tree). It's there as intended documentation/CI-signal on
   any build that does define debug assertions, but don't rely on it to
   catch this path in the test suite as configured today.
3. Both `burnCards` and `revealAndChoose`'s new trash branch reuse the
   `zone = Trash; location = nullopt` convention from
   `recycleCards`/`discardCards`/`killObject` rather than calling a
   shared helper — matching the existing codebase style (there's no
   single "moveToTrash" helper to call; each site sets the two fields
   inline). No behavior risk, just noting the pattern for consistency
   with future trash-destination code.

---

# Fix round 1 — burn-out fixture initialises subsystems; drop drawCards fallback

Commit `5e10eb0` on top of `06ab7c1`. Controller ruling widened this
task's scope to `tests/cards/test_burn_out_and_scoring.cpp`. Files:
`src/engine/game_engine.cpp`, `tests/cards/test_burn_out_and_scoring.cpp`,
`tests/cards/test_kennen_cards.cpp` (minor optional coverage case).

## Finding (Important) — fallback branch was a permanent production
## workaround for a test-fixture bug, with a dead assert

Per the review: the null-executor fallback in `GameEngine::drawCards`
(a throwaway `EffectExecutor` constructed inline when `effect_executor_`
was null) papered over a bug in
`BurnOutScoringTest::makeEngineAndSeedBattlefields()` rather than fixing
it, and its `assert(false, ...)` gives zero runtime signal in this
repo's Release/`-DNDEBUG` test build if the branch is ever hit again by
some other future caller. Both points are correct — this was the right
call to widen scope and fix at the source instead of living with a
permanent branch that exists only to compensate for one fixture.

## Fix

**`tests/cards/test_burn_out_and_scoring.cpp`** —
`makeEngineAndSeedBattlefields()` now calls
`engine_->testHook_initSubsystems()` immediately after constructing the
engine, before touching `state_` — mirroring `runGame()`'s own order
(`state_ = GameState{}; initSubsystems(); ...`). This wires up
`effect_executor_`/`chain_manager_`/`trigger_manager_` exactly as every
real game does, so `testHook_scoreConquer()` / `testHook_drawPhase()`
(and any other hook this fixture drives) now run with a live
`effect_executor_`, same as production.

**`src/engine/game_engine.cpp`** — `GameEngine::drawCards`'s empty-deck
branch now delegates unconditionally:

```cpp
assert(effect_executor_ &&
       "GameEngine::drawCards requires initSubsystems() to have "
       "run before an empty-deck draw (burnOut lives on the "
       "executor)");
effect_executor_->burnOut(player);
if (state_.game_over) return;
```

The throwaway-`EffectExecutor` fallback branch is deleted entirely. The
`assert` stays as documentation of the precondition (and a live signal
on any build that does define debug assertions), not as a safety net —
the actual safety net is the fixture fix above.

**`tests/cards/test_kennen_cards.cpp`** — added the reviewer's optional
minor case, `BurnCards_BurnOutEndsGame_StopsWithoutBurningMore`: deck
has 1 card, trash has 1, P2 sits one point short of victory; burning 3
cards burns the 1 deck card, then the second burn's `burnOut` recycles
both cards and pushes P2 to victory with more points than P1, ending
the game — asserts `state.game_over`/`winner`/`score`, and that neither
the third burn nor the rest of the second happened (trash is empty,
both cards sit in the reshuffled deck, untouched).

## Tests — burn-out suite through the real executor path, then full suite

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='BurnOutScoringTest.*'
[==========] Running 18 tests from 1 test suite.
...
[  PASSED  ] 18 tests.
```

All 18 pass with the fixture driving the real `effect_executor_` path
(no fallback branch exists anymore to fall back to) — including
`ScoreConquer_AtPenultimate_NotAllBFsScored_DrawWithEmptyDeck_
TriggersBurnOut`, the exact test that segfaulted under the naive
delegation in the original Task 6 pass.

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.*'
[==========] Running 5 tests from 1 test suite.
...
[  PASSED  ] 5 tests.
```

Full suite from the repo root:

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1092 tests from 119 test suites ran. (1864 ms total)
[  PASSED  ] 1092 tests.
  YOU HAVE 1 DISABLED TEST
```

1092 = the 1091 from the prior commit + 1 (the optional
`BurnCards_BurnOutEndsGame_StopsWithoutBurningMore` case, which the
review flagged as optional/add-only-if-cheap — it was a few lines on
the existing fixture, so it's included). The review's own "expect 1091"
was written before accounting for that optional addition; 1092 is the
correct total once it's added, with zero unexplained tests.

## Self-review

- Fixed at the source: the fixture now initialises subsystems like
  every other engine test, instead of the production code compensating
  for it. ✔
- `grep -rn "burned_out = true" src/` still returns exactly one hit
  (`effect_executor.cpp`'s `burnOut`) — the fix touched no burn-out
  logic, only the fixture and the now-unconditional delegation. ✔
- No other `GameEngine` test-hook fixture in this file (or elsewhere
  under `tests/`) was touched — this was the one fixture that skipped
  `initSubsystems()` while driving hooks that reach `effect_executor_`.
- Full suite green, no regressions, one net-new passing test.

## Concerns

None outstanding from this round. The Task 6 report's concern #1 (the
fixture gap) is now resolved; concern #2 (dead assert under
`-DNDEBUG`) is now moot since the branch it guarded is gone — the
remaining `assert` is precondition documentation only, matching the
review's own "an assert on non-null is fine as documentation" framing.
