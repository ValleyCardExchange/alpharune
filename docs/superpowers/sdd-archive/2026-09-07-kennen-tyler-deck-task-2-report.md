# Task 2 report: Empower / Disempower actions and the disempower cost

## Fix round 2 (review finding)

One finding, from my own round-1 residual note: the ruling is "every
board-exit path", and combat death via `GameEngine::killUnit` is a board
exit that `EmpowerTest.EmpoweredClearsWhenKilled` (which drives
`EffectExecutor::killObject` directly) doesn't cover.

### Fix

Added `unit.is_empowered = false;` in `GameEngine::killUnit`'s "no
replacement — unit dies normally" block (`game_engine.cpp`, right beside
the existing `damage_marked = 0` / `combat_designation = None` resets —
the same spot the function's own pre-existing comment already flags as
mirroring `EffectExecutor::killObject`: "CR 183.1: tokens cease to
exist... Matches EffectExecutor::killObject — same rule, different kill
path"). One line, no new duplicated block: `killUnit` already carries
its own full copy of the zone-move logic (pre-existing, not something
this task introduced), so the CR-441 status reset now travels with it
the same way it already does in the parallel `killObject` path.

I did not extract a cross-cutting shared helper for this single boolean
assignment: doing so would mean either giving `GameEngine` and
`EffectExecutor` a common status-reset entry point (touching
`effect_executor.h`, which is outside this round's allowed file set —
round 1 already landed and was reviewed without it) or adding a new
layer of indirection for one field write — more machinery than a
one-line fix warrants. Flagging that call for the controller rather than
making it unilaterally.

Not touched: the three early-return "replacement instead of death"
branches in `killUnit` (Tactical Retreat self-replacement, Altar of
Blood, legacy "would die → instead" text scan) — none of them are a
board exit (all three recall the unit to base), matching the same
reasoning already applied to `killObject`'s parallel Tactical-Retreat
branch in round 2's implementation.

### TDD evidence

Test added: `EmpowerTest.EmpoweredClearsOnCombatDeath` — builds a bare
`GameEngine` (mirrors `ElderDragonShieldTest` in
`test_targeting_and_combat_invariants.cpp`), places an empowered unit at
base with lethal `damage_marked`, calls
`engine.testHook_processLethalDamage()` (the existing hook that reaches
`killUnit` for any unit with lethal damage — the same entry point that
test file already uses to drive combat-style kills without a full combat
step), then asserts `is_empowered == false`. No `testHook_setAgents` /
`testHook_initSubsystems` needed: the object's `card_def_id` is
`kInvalidId`, so `killUnit`'s replacement-effect scans
(`card_registry_.get(kInvalidId)` → `nullptr`) never reach
`effect_executor_`, matching why `ElderDragonShieldTest` doesn't need
them either.

**RED** — `RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='EmpowerTest.*'`,
with the new `unit.is_empowered = false;` line temporarily removed from
`killUnit` (edited out, rebuilt, tested, then restored — kept in the
same working tree since it's a single line inside an already-modified
function, rather than stashing the whole file):

```
[ RUN      ] EmpowerTest.EmpoweredClearsOnCombatDeath
.../test_empower.cpp:129: Failure
Value of: s.getObject(unit_id).is_empowered
  Actual: true
Expected: false
Empowered must clear on combat death (GameEngine::killUnit), not just
the effect/ability-kill path (EffectExecutor::killObject)
[  FAILED  ] EmpowerTest.EmpoweredClearsOnCombatDeath (1 ms)
...
[  PASSED  ] 6 tests.
[  FAILED  ] 1 test
```

Why expected: `killUnit`'s death block didn't touch `is_empowered` at
all before this fix, so an empowered unit stayed empowered through
combat death.

**GREEN** — after restoring the line, same command:

```
[==========] 7 tests from 1 test suite ran. (13 ms total)
[  PASSED  ] 7 tests.
```

**Full suite (final):**

```
[==========] 1065 tests from 116 test suites ran. (1885 ms total)
[  PASSED  ] 1065 tests.
  YOU HAVE 1 DISABLED TEST
```

1064 (prior total) + 1 new = 1065. Same 1 pre-existing disabled test.

### Files changed this round

- `src/engine/game_engine.cpp` — one line in `killUnit`.
- `tests/cards/test_empower.cpp` — `EmpoweredClearsOnCombatDeath`.

## Fix round 1 (review findings)

### Finding 1: triplicated subsystem-init block

Fixed as prescribed. Added `GameEngine::initSubsystems()` — a single
private method in `game_engine.cpp` (declared in `game_engine.h`) holding
the `ChainManager`/`EffectExecutor`/`TriggerManager` construction and
wiring that was previously duplicated verbatim in `runGame` and
`resumeFromSnapshot`. Both now just call `initSubsystems()`; behavior is
identical (same calls, same order, nothing else changed in either
function). `testHook_initSubsystems()` in the header is now a one-line
wrapper (`{ initSubsystems(); }`), matching the shape of every other
`testHook_*` wrapper in that file.

Regression check: full suite green before and after
(1062/1062, both times) — see below.

### Finding 2: Empowered must clear on leaving the board

The controller's spec addendum #10 (commit `8281afc`, landed on this
branch mid-fix — pulled in automatically since I work directly against
the branch) corrects the spec: "same places `is_stunned` is reset" was
wrong (stun decays per-turn at Ending Step, not on board exit).
Empowered clears in the executor's board-exit paths: `killObject`,
`bounceToHand`, `banishObject`, and any recycle that takes an object off
the board (`recycleCards`). This matches exactly what the coordinator's
message asked for.

Implemented `obj.is_empowered = false;` at each board-exit point:
- `killObject` — both the unit branch (before the trash/banishment push)
  and the gear branch. NOT set in the death-replacement-recall branch
  (Tactical Retreat 737) — that unit is recalled to base, not leaving
  the board.
- `bounceToHand` — both the token-ceases-to-exist branch (bounced to
  Banishment) and the normal bounce-to-hand branch.
- `banishObject` — on the move to Banishment.
- `recycleCards`'s `recycle_one` lambda — covers a permanent recycled
  off the board (e.g. via `recycle_self` cost) back to Main/Rune Deck.
  (Harmless no-op for cards recycled from off-board zones, since
  `is_empowered` should already be false there.)

**Not touched:** `GameEngine::killUnit` (the separate combat-kill path
in `game_engine.cpp`, ~line 5503) duplicates `killObject`'s zone-move
logic independently but was not named in the fix instructions (which
scoped this to the three `EffectExecutor` methods + `recycleCards`), so
I left it alone. Flagging as a residual gap: a unit that dies in COMBAT
(via `killUnit`) rather than via an effect (`EffectExecutor::killObject`)
does not currently clear `is_empowered`. Worth a follow-up if combat
death of an empowered legend/unit becomes relevant later — legends don't
currently die via combat in this engine's normal flow, so it's likely
low-impact for Kennen, but noting it rather than silently leaving it
uncovered.

#### TDD evidence

Tests added to `test_empower.cpp`: `EmpowerTest.EmpoweredClearsWhenKilled`
and `EmpowerTest.EmpoweredClearsWhenBouncedToHand`.

**RED** — `RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='EmpowerTest.*'`,
run against the pre-fix `effect_executor.cpp` (temporarily `git stash`ed
just that file to reproduce cleanly, confirmed byte-identical restore
afterward with `diff`):

```
[ RUN      ] EmpowerTest.EmpoweredClearsWhenKilled
.../test_empower.cpp:85: Failure
Value of: state.getObject(obj_id).is_empowered
  Actual: true
Expected: false
Empowered must clear when the object leaves the board via death
[  FAILED  ] EmpowerTest.EmpoweredClearsWhenKilled (1 ms)
[ RUN      ] EmpowerTest.EmpoweredClearsWhenBouncedToHand
.../test_empower.cpp:96: Failure
Value of: state.getObject(obj_id).is_empowered
  Actual: true
Expected: false
Empowered must clear when the object leaves the board via bounce
[  FAILED  ] EmpowerTest.EmpoweredClearsWhenBouncedToHand (1 ms)
...
[  PASSED  ] 4 tests.
[  FAILED  ] 2 tests
```

Why expected: `killObject`/`bounceToHand` didn't touch `is_empowered` at
all before this fix, so an empowered unit stayed empowered after leaving
the board.

**GREEN** — after restoring the fix, same command:

```
[==========] 6 tests from 1 test suite ran. (11 ms total)
[  PASSED  ] 6 tests.
```

**Full suite (final, post both fixes):**

```
[==========] 1064 tests from 116 test suites ran. (1797 ms total)
[  PASSED  ] 1064 tests.
  YOU HAVE 1 DISABLED TEST
```

1062 (prior total) + 2 new = 1064. Same 1 pre-existing disabled test.

## What was implemented

1. **`EffectExecutor::empowerObject` / `disempowerObject`**
   (`src/engine/effect_executor.cpp`) — filled the Task-1 stubs per spec
   §1:
   - `empowerObject`: no-op if already empowered (CR 441.1.c); otherwise
     sets `is_empowered`, logs `EMPOWER: <name>`, emits
     `ObjectEmpoweredEvent{object, controller}`.
   - `disempowerObject`: no-op if not empowered (CR 442.1.a.1); otherwise
     clears `is_empowered`, logs `DISEMPOWER: <name>`. No event (matches
     spec — only empower emits).

2. **Three offer gates** (`src/engine/game_engine.cpp`) — added
   `if (<cost>.disempower_self && !obj.is_empowered) continue;` beside
   the existing `exhaust && is_exhausted` check at:
   - the showdown `[Action]` site (`generateShowdownActions`, ~line 2462)
   - the reaction site (`generateClosedStateActions`, ~line 2666)
   - `generateActivateAbilityActions` (~line 2999)
   The 4th `is_exhausted` site (aura-granted abilities, ~line 3065) is
   NOT one of the three named in the brief and was left untouched.

3. **Payment** (`GameEngine::executeIntent`, `ActivateAbility` /
   `ActivateActionAbility` case, ~line 1183) — after the exhaust
   component: `if (act_cost.disempower_self) { logTrace("ACTIVATE_COST:
   disempower " + source.name); effect_executor_->disempowerObject(...); }`.

4. **Renderer** (`src/io/state_renderer.cpp`, `renderPlayerSummary`) —
   appends ` [EMPOWERED]` after the legend name when
   `state.getObject(ps.legend_zone).is_empowered`.

5. **HTML marker (addendum #8) — NOT added.** Checked
   `src/io/play_index_html.cpp` with grep: the god-mode zone dump only
   iterates `['hand','main_deck','trash','banishment','rune_deck','base']`
   per player (`renderGod`, ~line 536) and battlefield units
   (`battlefieldBlock`); `legend_zone`/`champion_zone` never appear as a
   rendered row anywhere, and the only place `LegendZone` is mentioned is
   as a destination-dropdown option string for the state-editor move
   picker (unrelated to rendering the legend card itself). The file does
   not render the legend card, so per addendum #8 nothing was added there.

6. **`tests/cards/test_empower.cpp`** (new) — 4 tests (spec listed 3;
   added one extra for direct empower→disempower coverage).

## Deviation from the brief's file list — needs controller review

Test #3 needed to drive a real `ActivateAbility` intent through
`GameEngine::executeIntent` (via the existing `testHook_executeIntent`)
so the disempower **payment** code under test actually runs — the
payment lives only in `GameEngine::executeIntent`, not in the card's
`onActivate`, so there was no way to exercise it without going through
the engine. But a `GameEngine` constructed directly (the pattern every
existing `GameEngine engine(card_db, events, card_registry)` test in this
repo uses for hand-built `GameState`s — see `test_champions_legends.cpp`,
`test_ivern_deck.cpp`) leaves `chain_manager_` / `effect_executor_` /
`trigger_manager_` null: they're only initialized inside `runGame` /
`resumeFromSnapshot`. Every existing test using this pattern only ever
calls `generateLegalActions()` (read-only, doesn't touch those members);
none previously executed an activated ability to completion this way.
`ActivateAbility`'s handler unconditionally calls
`effect_executor_->disempowerObject(...)` and
`chain_manager_->addAbility(...)` + `runChain()`, so it segfaulted
(confirmed with gdb: null-pointer dereference inside `disempowerObject`,
called from `executeIntent`).

I added a small test-only hook, `GameEngine::testHook_initSubsystems()`
in `src/engine/game_engine.h`, that runs the identical subsystem-init
block already duplicated in `runGame` and `resumeFromSnapshot` (same
`ChainManager`/`EffectExecutor`/`TriggerManager` construction, same
afford/pay-cost/agent-query callbacks), without touching `agents_` or
starting a turn loop. It changes no existing behavior — purely additive,
mirrors two already-identical blocks, and follows the exact pattern of
every other `testHook_*` method already in that file (including the
adjacent, previously-unused `testHook_executeIntent` this task's brief
pointed me at).

This file is **not** in the brief's named list
(`effect_executor.cpp`, `game_engine.cpp`, `state_renderer.cpp`,
`test_empower.cpp`, conditionally `play_index_html.cpp`). I made the call
to add it rather than report NEEDS_CONTEXT, because: (a) it's required
to test the actual behavior the brief specifies for test #3 ("execute
it; assert `is_empowered == false` and `is_exhausted == true`") through
the real engine path rather than a hand-rolled bypass of the code under
test; (b) it's zero-risk to production behavior (test-only, additive,
mirrors existing code exactly); (c) the brief itself pointed at
`testHook_executeIntent` as "the engine's public intent-execution entry
point" for this test, which only works standalone with this addition.
Flagging for explicit review since it's outside the stated scope.

## TDD evidence

### Tests #1 + #2 (RED)

Command: `RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='EmpowerTest.*'`
against the still-empty stubs:

```
[ RUN      ] EmpowerTest.EmpoweringSetsStatusAndEmitsEventOnce
.../test_empower.cpp:40: Failure
Value of: state.getObject(obj_id).is_empowered
  Actual: false
Expected: true
.../test_empower.cpp:41: Failure
Expected equality of these values:
  empower_events
    Which is: 0
  1
[  FAILED  ] EmpowerTest.EmpoweringSetsStatusAndEmitsEventOnce (3 ms)
[ RUN      ] EmpowerTest.DisempoweringNonEmpoweredObjectIsNoOp
[       OK ] EmpowerTest.DisempoweringNonEmpoweredObjectIsNoOp (1 ms)
[ RUN      ] EmpowerTest.DisempoweringEmpoweredObjectClearsStatus
.../test_empower.cpp:70: Failure
Value of: state.getObject(obj_id).is_empowered
  Actual: false
Expected: true
[  FAILED  ] EmpowerTest.DisempoweringEmpoweredObjectClearsStatus (1 ms)
```

`EmpoweringSetsStatusAndEmitsEventOnce` failed as expected (status never
set, event never emitted — the stub is a no-op). Note:
`DisempoweringNonEmpoweredObjectIsNoOp` (spec test #2) passed even
against the empty stub — with a no-op `disempowerObject`, "leaves it
non-empowered and emits nothing" is trivially true before any code is
written, so it can't RED on its own; it's a real regression guard for
after implementation, not a discriminating test. My third test
(`DisempoweringEmpoweredObjectClearsStatus`, not in the spec's numbered
list but useful coverage of the "clears an empowered object" half of
disempower) did RED correctly, since it depends on `empowerObject`
actually setting the flag first.

Why expected: `empowerObject`/`disempowerObject` were empty bodies
(Task 1 scaffolding) — no field write, no event emission.

### Tests #1 + #2 (GREEN)

Same command after implementing the two executor methods:

```
[ RUN      ] EmpowerTest.EmpoweringSetsStatusAndEmitsEventOnce
[       OK ] EmpowerTest.EmpoweringSetsStatusAndEmitsEventOnce (3 ms)
[ RUN      ] EmpowerTest.DisempoweringNonEmpoweredObjectIsNoOp
[       OK ] EmpowerTest.DisempoweringNonEmpoweredObjectIsNoOp (1 ms)
[ RUN      ] EmpowerTest.DisempoweringEmpoweredObjectClearsStatus
[       OK ] EmpowerTest.DisempoweringEmpoweredObjectClearsStatus (1 ms)
```

Full suite passed at this point too (1058 pre-existing + these 3 new,
test #3 not yet written).

### Test #3 (RED)

Command: `RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='EmpowerTest.*'`
with the gates/payment not yet added:

```
[ RUN      ] EmpowerTest.DisempowerSelfCostGatesAndPaysOnActivation
.../test_empower.cpp:151: Failure
Value of: hasActivateFor(legend)
  Actual: true
Expected: false
disempower_self ability must be gated while the source is not empowered
```

Why expected: `generateActivateAbilityActions` only checked
`act_cost.exhaust`; with no `disempower_self` gate the ability was
offered even while `is_empowered == false`.

(This RED run also surfaced the `testHook_initSubsystems` gap above —
without the gate, the test proceeded past the failed `EXPECT_FALSE` into
executing the intent, which then segfaulted on the null
`effect_executor_`. Confirmed with `gdb -batch -ex run -ex bt`:
`SIGSEGV` inside `EffectExecutor::disempowerObject`, called from
`GameEngine::executeIntent`, called from the test body — a null-`this`
dereference, not a logic bug in the new code. Fixed by adding the hook
and calling it before building state, as described above.)

### Test #3 (GREEN)

```
[ RUN      ] EmpowerTest.DisempowerSelfCostGatesAndPaysOnActivation
[       OK ] EmpowerTest.DisempowerSelfCostGatesAndPaysOnActivation (1 ms)
```

### Full suite (final)

`RIFTBOUND_ROOT=. ./build/riftbound_tests`:

```
[==========] 1062 tests from 116 test suites ran. (1801 ms total)
[  PASSED  ] 1062 tests.
  YOU HAVE 1 DISABLED TEST
```

1058 pre-existing + 4 new (`EmpowerTest` has 4 cases: spec's 2 + 1 extra
coverage test + test #3) = 1062. 1 pre-existing disabled test, as
expected.

## Files changed

- `src/engine/effect_executor.cpp` — `empowerObject`/`disempowerObject` bodies.
- `src/engine/game_engine.cpp` — 3 offer gates + payment in `executeIntent`.
- `src/engine/game_engine.h` — added `testHook_initSubsystems()` (see
  deviation note above).
- `src/io/state_renderer.cpp` — ` [EMPOWERED]` marker.
- `tests/cards/test_empower.cpp` — new, 4 tests.
- `src/io/play_index_html.cpp` — **not touched** (does not render the
  legend card; verified by grep, addendum #8 condition not met).

## Engine execution entry point used

`GameEngine::testHook_executeIntent(const Intent&)` (a pre-existing
public inline wrapper around the private `executeIntent`, declared in
`game_engine.h` — the same method the main-phase loop and showdown/
reaction dispatch call for every activate/play intent). Test #3 also
needed `testHook_setAgents` (pre-existing) and the new
`testHook_initSubsystems` to make that entry point usable on a
manually-constructed engine.

## Test-local card registration

Registered a minimal `EmpowerCostTestLegend : public LegendCard`
(id 900, well above the 787-card registry) directly into `card_registry`
inside the `TEST_F` body, via `card_registry.registerCard(900, ...)`.
Did **not** rebuild `CardDB` (`card_db.buildFromClasses(card_registry)`)
afterward: `CardTestFixture::SetUp()` calls `loadAll()` then
`buildFromClasses()` before the test body runs, but the code paths this
test exercises (`GameEngine::generateActivateAbilityActions`,
`executeIntent`'s ability dispatch) only ever call
`card_registry_.get(card_def_id)`, never `card_db.get(900)` — the
`GameObject`'s `name`/`card_type`/etc. are set by hand in the test
(mirroring how `addUnit(..., kInvalidId)` handles unregistered cards in
the fixture), so no `CardDB` entry was needed for id 900.

## Self-review

- Names verified against the brief/spec verbatim: `empowerObject`,
  `disempowerObject`, `ActivationCost::disempower_self`,
  `ObjectEmpoweredEvent`, `EMPOWER:`/`DISEMPOWER:`/
  `ACTIVATE_COST: disempower` log prefixes, `[EMPOWERED]` renderer text.
- No extra behaviour: did not add an `is_empowered` reset-on-leave-board
  path (the `game_object.h` field comment from Task 1 mentions this, but
  it's not in Task 2's file list or test list, and no existing code
  resets `is_stunned` on leaving the board either — the only `is_stunned`
  reset found is the per-ending-step universal decay loop, a different
  mechanism from what the comment describes). Leaving this for whichever
  later task actually needs it.
- Did not touch the 4th `is_exhausted` gate site (aura-granted
  abilities) — out of the three named sites.
- No card files touched, `card_helpers.h` untouched.
- Followed existing code style/log format at every site (matched
  `killObject`/`exhaustObject` shape in `effect_executor.cpp`; matched
  the `recycle_self`/`discard`/`xp_cost` payment block shape in
  `game_engine.cpp`; matched `test_champions_legends.cpp`'s
  `GameEngine` + `mutableState()` setup shape in the new test).
- Tests assert real engine state (`is_empowered`, `is_exhausted`,
  `ObjectEmpoweredEvent` count via `EventBus::on_object_empowered`), not
  reimplemented logic.
- Full-suite output is clean — no new warnings observed in the build log.

## Concerns

1. **`game_engine.h` deviation** (see above) — the one file touched
   outside the brief's list. Purely additive test infrastructure, but
   flagging explicitly since the brief was specific about scope.
2. The `is_empowered`-survives-vs-cleared-on-leaving-board behavior
   documented in `game_object.h`'s field comment (from Task 1) has no
   implementation anywhere yet (and neither does the analogous
   `is_stunned` "leaves the board" case it's compared to — only a
   per-turn decay exists for that). Not in this task's scope per the
   brief's test list, but worth the controller confirming it's deferred
   deliberately rather than dropped.
