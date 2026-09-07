# Task 3 report: Play source on the card-played event + the non-hand trigger

## Fix round 1 (review findings)

### Finding 1 [Important]: `Hidden` play_source unreachable in live gameplay

Confirmed exactly as the review stated: the two `game_engine.cpp`
facedown sites I patched in the original round
(`GameEngine::executePlayFromHidden`, ~2049/2057) are dead code —
`grep -rn "executePlayFromHidden" src/ tests/` finds only the
declaration/definition, no caller anywhere. The LIVE CR 811
facedown-reveal-as-reaction path is
`ChainManager::stepExecuteAndPass`'s `PlayReaction` branch
(`src/engine/chain_manager.cpp:229-266` at review time): a hidden card
is offered as a `PlayReaction` intent while still facedown, and that
branch clears `card.is_hidden` and emits `CardPlayedEvent` without ever
setting `play_source` (defaulting to `Hand`).

**Controller ruling applied:** `src/engine/chain_manager.cpp` added to
Task 3's allowed files.

**Fix:** captured `Intent::PlaySource event_play_source =
playSourceForZone(card.zone, card.is_hidden);` as the first statement
inside the `PlayReaction` branch — before `card.is_hidden = false;` two
lines later — and added `event_play_source` as the 6th field on the
`CardPlayedEvent{...}` emit at the end of that branch. Same shared
`playSourceForZone` helper `EffectExecutor::playIgnoringCost` already
uses (no new mapping, no duplication); `ChainManager` can't call
`GameEngine::playSourceFor` either, for the same reason
`EffectExecutor` can't.

Left the dead `GameEngine::executePlayFromHidden` sites exactly as they
are, per the ruling — not deleting dead code this round. Observation
for whoever eventually cleans it up: `executePlayFromHidden` is fully
unreferenced (`IntentType::PlayFromHidden` or whatever would dispatch
to it doesn't appear to exist as a generated legal action either — the
generator side of this path was apparently never wired up, only the
executor side), so the two `CardPlayedEvent` emits I added there in
round 1 are inert but harmless.

#### TDD evidence

New test: `PlayFromNonHandTest.HiddenCardRevealedAsReactionFiresWithHiddenSource`
(`tests/cards/test_play_from_non_hand.cpp`). No existing test drives a
hidden card through the real closed-state `PlayReaction` path (checked:
`grep -rn "is_hidden = true" tests/` finds only
`test_jhin_deck.cpp`'s `LotusTrap_FacedownDiscardedOnBFControlLoss`,
which replicates BF-control-loss cleanup logic inline and never touches
`ChainManager`). So, per the review's suggestion, I drove `ChainManager`
directly — a `ChainManager` + a hand-wired `TriggerManager` (both bound
to the fixture's own `state`/`events`/`card_db`/`card_registry`, no full
`GameEngine`), mirroring the shape of `card_test_fixture.h`'s
`driveThroughChain` helper (which only covers hand-played spells, not
the facedown-reaction branch, so I assembled the same pieces by hand):

- A test-local legend (`NonHandWatcherLegend`, same class as the other
  two tests) in `legend_zone`.
- A hidden `Spell`-typed object at BF#0's facedown zone
  (`is_hidden = true`, `hidden_at = 0`).
- A dummy spell already added to the chain via `cm.addSpell(...)`,
  controlled by the same player — this opens the Execute/Pass priority
  window that a `PlayReaction` intent is offered into (CR: the newest
  chain item's controller gets first priority, so the player can react
  to their own item immediately without needing a second player in the
  loop).
- A hand-written `query_agent` lambda: returns `PlayReaction` targeting
  the hidden card on its first invocation, `PassPriority` on every
  later one. `stepExecuteAndPass` only inspects `chosen.type` /
  `chosen.card` / `chosen.targets` — never the injected `actions` list
  — so no real legal-action generator was needed; this is "driving
  ChainManager directly," as suggested.
- A `resolve_spell` callback mirroring `GameEngine::resolveSpell`'s
  `is_ability` → `onTrigger` / else → `onResolve` dispatch (simplified —
  no per-target legality re-validation, since neither chain item here
  has real targets).

Subscribed to `events.on_card_played` and ran `cm.processFEPR(...)`.
Asserts: exactly one `CardPlayedEvent` total (the dummy spell entered
the chain via a direct `cm.addSpell(...)` call, not through a
play-intent path that emits one, so only the hidden card's
`PlayReaction` handling ever emits `CardPlayedEvent`), with
`play_source == Hidden`; the legend's `card_counters["fired"] == 1` and
`card_counters["subject"] == hidden_card`.

**RED** — temporarily reverted just the `chain_manager.cpp` fix (kept
the capture, reverted the emit back to the pre-fix 5-field call,
`(void)`-cast the now-unused local — same "edit out, rebuild, test,
restore" pattern as Task 2's round 2), rebuilt (incremental — only
`chain_manager.cpp` recompiled), ran:

```
$ cmake --build build --target riftbound_tests -j4
[1/5] Building CXX object CMakeFiles/riftbound_core.dir/src/engine/chain_manager.cpp.o
[2/5] Building CXX object CMakeFiles/riftbound_tests.dir/tests/cards/test_play_from_non_hand.cpp.o
[3/5] Building CXX object CMakeFiles/riftbound_core.dir/src/engine/game_engine.cpp.o
[4/5] Linking CXX static library libriftbound_core.a
[5/5] Linking CXX executable riftbound_tests

$ ./build/riftbound_tests --gtest_filter='PlayFromNonHandTest.HiddenCardRevealedAsReactionFiresWithHiddenSource'
.../test_play_from_non_hand.cpp:474: Failure
Expected equality of these values:
  played[0].play_source
    Which is: 1-byte object <00>
  Intent::PlaySource::Hidden
    Which is: 1-byte object <04>
A facedown card revealed and played as a reaction through the LIVE CR
811 path (ChainManager::stepExecuteAndPass) must carry play_source ==
Hidden.

.../test_play_from_non_hand.cpp:480: Failure
Expected equality of these values:
  state.getObject(legend_id).card_counters["fired"]
    Which is: 0
  1
WhenYouPlayFromNonHand must fire on the legend for a live
hidden-reveal-as-reaction play.

.../test_play_from_non_hand.cpp:483: Failure  (subject: 0 vs 2)
[  FAILED  ] PlayFromNonHandTest.HiddenCardRevealedAsReactionFiresWithHiddenSource (3 ms)
```

Why expected: exactly the reported bug — `play_source` defaulted to
`Hand`, so `TriggerManager::onCardPlayed`'s `e.play_source != Hand`
gate never opened and the legend never fired.

**GREEN** — restored the fix (`diff` against the pre-revert file
confirmed byte-identical), rebuilt, reran:

```
$ ./build/riftbound_tests --gtest_filter='PlayFromNonHandTest.*:CardTestFixture.TokenCreationEmitsNoCardPlayedEvent'
[==========] 4 tests from 2 test suites ran. (8 ms total)
[  PASSED  ] 4 tests.
```

**Full suite (final):**

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1069 tests from 117 test suites ran. (1802 ms total)
[  PASSED  ] 1069 tests.
  YOU HAVE 1 DISABLED TEST
```

1068 (prior total) + 1 new = 1069. Same 1 pre-existing disabled test.
Exit code 0.

### Finding 2 [Minor]: rename `play_source` → `event_play_source`

Renamed the zone-derived local in both `executePlayCard` and
`executePlaySpell` (`src/engine/game_engine.cpp`) from `play_source` to
`event_play_source`, and updated their two `CardPlayedEvent{...}` emit
sites to match. Left `executePlayFromHidden`'s local named
`play_source` — that function never reads `intent.play_source`, so
there was nothing to shadow there and the finding didn't ask for it.
Added a one-line comment at each renamed capture site explaining (a)
why the name changed (avoids shadow-by-name confusion with
`intent.play_source`, which legitimately drives a different concern —
`current_play_source` / cost-payment / the trash-replay-grant path) and
(b) why the capture-before-mutation is kept even though it's currently
redundant in these two functions specifically (their zone-removal
blocks mutate `ps.hand`/`ps.champion_zone`/`ps.trash`, not `card.zone`
itself — the capture is cheap insurance against a future change to
those blocks silently breaking `play_source`).

No behavior change; covered by the same full-suite run above (renaming
a local doesn't need its own RED/GREEN cycle).

### Files changed this round

- `src/engine/chain_manager.cpp` — `play_source` on the live
  facedown-reveal-as-reaction `CardPlayedEvent` emit (Finding 1).
- `src/engine/game_engine.cpp` — `play_source` → `event_play_source`
  rename + comments in `executePlayCard`/`executePlaySpell` (Finding 2).
- `tests/cards/test_play_from_non_hand.cpp` —
  `HiddenCardRevealedAsReactionFiresWithHiddenSource` (Finding 1).

## What was implemented

Per the dispatch notes (which carry addendum #2 and win over the brief's
"set from the executing intent" wording, itself already marked
`[SUPERSEDED → addendum #2]` in the spec): play source is derived from
the played object's **zone at execution time**, never from `Intent`.

1. **Shared mapping — `playSourceForZone(ZoneType, bool is_hidden)`**
   (`src/core/intent.h`, free `inline` function after the `Intent`
   struct, same `riftbound` namespace). Hand → Hand; Trash → Trash;
   Banishment → Banishment; ChampionZone → ChampionZone; Chain →
   ChainZone; `is_hidden` (checked first, any zone) → Hidden; anything
   else (including the default `MainDeck`/`RuneDeck`/etc.) → Hand. No
   `Deck` value.

2. **`GameEngine::playSourceFor(const GameObject&)`**
   (`src/engine/game_engine.cpp:2890`) — now `return
   playSourceForZone(obj.zone, obj.is_hidden);`, replacing the
   unconditional-Hand stub.

3. **The four `game_engine.cpp` emit sites** — each now captures
   `Intent::PlaySource play_source = playSourceFor(card);` **before**
   the code that mutates the object's zone/hidden-flag, then passes
   `play_source` as the event's 6th field:
   - `executePlayCard` (~line 1269, captured before the hand/champion-zone
     removal block at ~1275) — covers permanents including champion
     plays (verified card.zone is untouched by that removal block itself;
     it only mutates `ps.hand`/`ps.champion_zone`, so capture-before is
     defense-in-depth, not strictly load-bearing here, but matches the
     brief's ordering guidance and protects against future changes).
   - `executePlaySpell` (~line 1408, captured before the
     hand/trash removal block at ~1414) — same reasoning; covers normal
     hand spells and Fizz/Death-from-Below-style trash replays that go
     through the spell path.
   - `executePlayFromHidden`, permanent branch and spell branch (~line
     2031/2049/2057) — captured at line 2031, **before** `card.is_hidden
     = false;` at line 2035. This one IS load-bearing: `is_hidden` is
     the only signal `playSourceForZone` has for a facedown reveal
     (zone stays `FacedownZone`, not `BattlefieldZone`, through this
     whole function), and it's cleared two lines later in the same
     function. Capturing after that line would have silently produced
     `Hand` instead of `Hidden`.

4. **`EffectExecutor::playIgnoringCost`** (`src/engine/effect_executor.cpp`)
   — captures `Intent::PlaySource play_source =
   playSourceForZone(obj.zone, obj.is_hidden);` as the very first
   statement, before `obj.zone` is overwritten to
   `Base`/`BattlefieldZone` two lines later. This one is also
   load-bearing: `obj.zone` is unconditionally rewritten before the
   `CardPlayedEvent` emit at the end of the function, so without the
   early capture the zone would already read post-play. `EffectExecutor`
   cannot call `GameEngine::playSourceFor` (no back-reference), so it
   calls the shared `playSourceForZone` helper directly — same mapping,
   no duplication.

5. **`TriggerManager::onCardPlayed`** (`src/engine/trigger_manager.cpp`,
   inserted after the existing `WhenYouPlayASpell`/`WhenYouPlayAUnit`
   legend-zone sweep, ~line 361) — when `e.play_source !=
   Intent::PlaySource::Hand`:
   - Same loop shape as the `WhenYouPlayASpell`/`WhenYouPlayAUnit` board
     scan above it: iterate `state_.objects`, require `location`, same
     controller, skip the played card itself (`id == e.object`), fire
     `WhenYouPlayFromNonHand` via `fireTrigger(id, e.player, 0,
     TriggerType::WhenYouPlayFromNonHand, /*subject=*/e.object)`.
   - The legend: checked and fired **directly** (not through
     `fireLegendTrigger`) because `fireLegendTrigger` has no `subject`
     parameter and the brief requires the played card to be the chain
     item's triggering subject. This mirrors the existing
     `WhenAUnitBecomesMighty` legend-with-subject pattern a few dozen
     lines above (`fireTrigger(legend_id, controller, 0,
     TriggerType::WhenAUnitBecomesMighty, /*subject=*/e.object)`) rather
     than introducing a new `fireLegendTrigger` overload, which would
     have meant touching `trigger_manager.h` outside this task's file
     list for a single extra call site.

## Where the shared mapping lives, and why

`core/intent.h`, as a free `inline` function (`playSourceForZone`)
declared right after the `Intent` struct, in the same header both
`game_engine.h` and `effect_executor.h` already `#include`
unconditionally. This was the "no new header" branch of the brief's
option: no existing-header edit beyond `intent.h` was needed, since
`Intent::PlaySource` (the return type) is itself defined in that file
and `ZoneType` comes in transitively via `intent.h`'s existing
`#include "types.h"`. `GameEngine::playSourceFor` is now a one-line
wrapper over it (kept as a member function since the brief/spec name it
by that signature and other card code may eventually call it); the
`EffectExecutor::playIgnoringCost` and `TriggerManager` files call
`playSourceForZone`/consume `play_source` without needing any new
declaration. One mapping, defined once, in one place, no rebuild-cost
header beyond `intent.h` itself (which was already going to be touched
regardless, since it's the only place both silos share).

## New test file

`tests/cards/test_play_from_non_hand.cpp` — 3 tests:

1. **`PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource`**
   (spec test #4). Registers a test-local legend (`NonHandWatcherLegend`,
   id 970) in `legend_zone` AND a test-local on-board unit
   (`NonHandWatcherUnit`, id 971) at base — both declare
   `triggerTypes() = {WhenYouPlayFromNonHand}` and increment
   `card_counters["fired"]` on trigger (the legend also records the
   chain item's `triggering_subject` into `card_counters["subject"]`,
   read via `ctx.state.chain.resuming->triggering_subject`, mirroring
   `StealthyPursuer`/`FioraWorthy`'s existing pattern for reading a
   trigger's subject). Drives a REAL card through the REAL engine: The
   Harrowing (id 198, "play a unit from your trash, ignoring its Energy
   cost", 6E+2 Chaos P) played from hand via `generateLegalActions()` +
   `testHook_executeIntent`, targeting a unit sitting in P1's trash.
   Harrowing's own `onResolve` calls `EffectExecutor::playIgnoringCost`
   on the trash unit — the exact code path this task changes — while
   already deep inside the engine's own `runChain()`/FEPR resolve loop,
   so the newly-fired `WhenYouPlayFromNonHand` ability chain item gets
   picked up and resolved by that same outer loop (this is the real
   in-game mechanism, not a test bypass). Subscribes to
   `events.on_card_played` and asserts: two events total — Harrowing
   itself (`play_source == Hand`) then the trash unit (`play_source ==
   Trash`); both watchers fired exactly once (from the trash replay
   only); the legend's recorded subject equals the trash-replayed
   unit's id.

   I did not use `playIgnoringCost` directly from the test as the brief
   literally suggests, because `playIgnoringCost` only boards
   permanents — playing an actual *spell* object through it would sit
   it on the board "incorrectly" (documented limitation, see
   `0025_blind_fury.cpp`'s comment: "playIgnoringCost only boards
   permanents; a spell would be placed on the board incorrectly"). Using
   a real card (The Harrowing) that itself calls `playIgnoringCost` on a
   *unit* gives the same `play_source == Trash` proof through the
   correct, CR-legal shape of the primitive, and additionally proves the
   trigger fires when reached the way real gameplay reaches it (through
   an already-running chain resolution), not through a hand-assembled
   `EffectExecutor`/`TriggerManager` pair with no chain driving it.

2. **`PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource`**
   (spec test #5 / addendum #3). Darius, Trifarian (id 27, a real
   Champion unit, 5E+1 Fury P — its own printed trigger is a no-op here
   since only one card is played) placed in `champion_zone`, played via
   `generateLegalActions()` + `testHook_executeIntent`. Asserts the
   observed `CardPlayedEvent.play_source == ChampionZone` and the
   watcher legend fired once.

3. **`CardTestFixture.TokenCreationEmitsNoCardPlayedEvent`** (addendum
   #7 guard, cheap — no new scaffolding needed). Calls
   `EffectExecutor::createToken` directly and asserts
   `events.on_card_played` never fires — confirms tokens can never feed
   `WhenYouPlayFromNonHand` (that function emits no `CardPlayedEvent` at
   all; unchanged by this task, verified by reading it).

## TDD evidence

### Test #4 — RED against the Task-1/2 stub

Command: `cmake --build build --target riftbound_tests -j4` then
`./build/riftbound_tests --gtest_filter='PlayFromNonHandTest.*:CardTestFixture.TokenCreationEmitsNoCardPlayedEvent'`,
run with the test file written but **none of the four source files
touched yet** (`playSourceFor` still the unconditional-`Hand` stub from
Task 2; no `WhenYouPlayFromNonHand` dispatch in `trigger_manager.cpp`
at all):

```
[ RUN      ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource
.../test_play_from_non_hand.cpp:247: Failure
Expected equality of these values:
  played[1].play_source
    Which is: 1-byte object <00>
  Intent::PlaySource::Trash
    Which is: 1-byte object <01>
The replayed unit's zone was Trash at the moment playIgnoringCost ran...

.../test_play_from_non_hand.cpp:253: Failure
Expected equality of these values:
  s.getObject(legend_id).card_counters["fired"]
    Which is: 0
  1
WhenYouPlayFromNonHand must fire on the legend exactly once...

.../test_play_from_non_hand.cpp:257: Failure  (subject: 0 vs 13)
.../test_play_from_non_hand.cpp:263: Failure  (board watcher fired: 0 vs 1)
[  FAILED  ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource (1 ms)

[ RUN      ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource
.../test_play_from_non_hand.cpp:330: Failure  (play_source: Hand vs ChampionZone)
.../test_play_from_non_hand.cpp:336: Failure  (legend fired: 0 vs 1)
[  FAILED  ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource (1 ms)

[  PASSED  ] 1 test.  (the token guard — unrelated to the stub)
[  FAILED  ] 2 tests.
```

Both failures are exactly what the stub predicts: Harrowing's own play
correctly showed `Hand` (test setup was already right for that half);
the trash-replayed unit's event showed `Hand` instead of `Trash`
because `playSourceFor`/`playIgnoringCost` didn't exist yet; the
champion's event showed `Hand` instead of `ChampionZone` for the same
reason; and both watchers never fired because `onCardPlayed` had no
`WhenYouPlayFromNonHand` dispatch at all yet. Confirms the test
exercises the right code and would have passed for the wrong reason
against the stub if it hadn't asserted `play_source` explicitly (per
the dispatch notes' warning).

### Test #4 + #5 — GREEN after implementation

Same filter, after all four source edits landed and the widened
`intent.h` rebuild completed:

```
[ RUN      ] CardTestFixture.TokenCreationEmitsNoCardPlayedEvent
[       OK ] CardTestFixture.TokenCreationEmitsNoCardPlayedEvent (3 ms)
[ RUN      ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource
[       OK ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource (2 ms)
[ RUN      ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource
[       OK ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource (1 ms)
[  PASSED  ] 3 tests.
```

### Test #5 — RED-by-revert proof (brief Step 6)

Test #5 passed immediately on first implementation (both the mapping
and the dispatch landed together), so per the brief's Step 6 I proved
the champion-zone mapping specifically discriminates: temporarily
reverted **only** `GameEngine::playSourceFor` back to the
unconditional-`Hand` stub (`core/intent.h`'s `playSourceForZone` and
everything else left as implemented), rebuilt (incremental — only
`game_engine.cpp` recompiled), reran:

```
$ cmake --build build --target riftbound_tests -j4
[1/3] Building CXX object CMakeFiles/riftbound_core.dir/src/engine/game_engine.cpp.o
[2/3] Linking CXX static library libriftbound_core.a
[3/3] Linking CXX executable riftbound_tests

$ ./build/riftbound_tests --gtest_filter='PlayFromNonHandTest.*'
[ RUN      ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource
[       OK ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource (3 ms)
[ RUN      ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource
.../test_play_from_non_hand.cpp:330: Failure
Expected equality of these values:
  played[0].play_source
    Which is: 1-byte object <00>
  Intent::PlaySource::ChampionZone
    Which is: 1-byte object <03>
.../test_play_from_non_hand.cpp:336: Failure  (legend fired: 0 vs 1)
[  FAILED  ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource (2 ms)
[  PASSED  ] 1 test.
[  FAILED  ] 1 test.
```

Test #4 stayed GREEN through this revert — expected and a useful
cross-check: #4's `play_source == Trash` assertion is produced by
`EffectExecutor::playIgnoringCost` calling `playSourceForZone` directly
(not through `GameEngine::playSourceFor`), so reverting only the
engine-side wrapper isolates the champion-zone path cleanly and proves
the two call sites are independently covered, not accidentally both
passing off one lucky code path.

Restored `playSourceFor` to `return playSourceForZone(obj.zone,
obj.is_hidden);` (`diff` against the pre-revert file confirmed
byte-identical), rebuilt, reran — both tests GREEN again:

```
[ RUN      ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource
[       OK ] PlayFromNonHandTest.HandPlayDoesNotFire_TrashReplayFiresWithTrashSource (1 ms)
[ RUN      ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource
[       OK ] PlayFromNonHandTest.ChampionPlayedFromChampionZoneFiresWithChampionZoneSource (1 ms)
[  PASSED  ] 3 tests.
```

### Full suite

`RIFTBOUND_ROOT=. ./build/riftbound_tests`:

```
[==========] 1068 tests from 117 test suites ran. (1811 ms total)
[  PASSED  ] 1068 tests.
YOU HAVE 1 DISABLED TEST
```

1065 (prior total per dispatch) + 3 new (2 `PlayFromNonHandTest` cases +
1 `CardTestFixture` guard case) = 1068. Same 1 pre-existing disabled
test. Exit code 0.

## Files changed

- `src/core/intent.h` — added the shared `playSourceForZone(ZoneType,
  bool)` free function (the one existing-header edit named in the
  brief's constraints; justified above).
- `src/engine/game_engine.cpp` — implemented `playSourceFor`; captured
  and set `play_source` at all four `CardPlayedEvent` emit sites
  (`executePlayCard`, `executePlaySpell`, `executePlayFromHidden` ×2).
- `src/engine/effect_executor.cpp` — captured and set `play_source` in
  `playIgnoringCost`.
- `src/engine/trigger_manager.cpp` — added the `WhenYouPlayFromNonHand`
  dispatch (board-card loop + legend, with subject) in `onCardPlayed`.
- `tests/cards/test_play_from_non_hand.cpp` — new, 3 tests (test #4,
  test #5, addendum #7 guard).

Rebuild cost: `core/intent.h` is included (transitively, via
`core/events.h` or `core/intent.h` directly) by essentially every
`.cpp` in `src/cards/`, so the first build after this change relinked
`riftbound_core` from a full recompile (~870 translation units,
~15 minutes on this 4-core sandbox). Every subsequent incremental build
(the revert/restore cycle for test #5) only recompiled `game_engine.cpp`
itself, as expected for a `.cpp`-only edit.

## Self-review vs brief + notes

- **Every emit site covered**: confirmed all 5 (4 in `game_engine.cpp` +
  `playIgnoringCost` in `effect_executor.cpp`) via `grep -n
  "CardPlayedEvent{"` across both files before and after — no site
  missed, no stray site touched.
- **Mapping defined once**: `playSourceForZone` is the only place the
  zone→source switch exists; both `GameEngine::playSourceFor` and
  `EffectExecutor::playIgnoringCost` delegate to it. No duplicated
  switch statement anywhere.
- **Play source derived from zone, never intent**: verified — none of
  the four `game_engine.cpp` sites or `playIgnoringCost` read
  `intent.play_source` for the event's `play_source` field. (The
  pre-existing `intent.play_source` reads in `executePlaySpell`, e.g.
  `ps.current_play_source = intent.play_source;` for Rek'Sai's
  auto-Accelerate and the trash-replay-grant cost path, are untouched —
  those are a different concern, the COST payment path, not the event.)
- **Capture-before-mutation**: verified per-site above; the two sites
  where this is actually load-bearing (`executePlayFromHidden`'s
  `is_hidden` clear, and `playIgnoringCost`'s zone overwrite) were
  double-checked by reading the intervening lines, not assumed.
- **Names exact**: `playSourceFor`, `WhenYouPlayFromNonHand`,
  `PlaySource::{Hand,Trash,Banishment,ChampionZone,Hidden,ChainZone}`
  all matched verbatim against the existing declarations from Tasks 1-2
  — no renaming, no new enum values.
- **No extra behaviour**: did not touch `intent.play_source`'s existing
  consumers (Rek'Sai auto-Accelerate, trash-replay-grant cost path);
  did not add a `fireLegendTrigger` overload (used the existing
  direct-`fireTrigger` legend pattern instead, avoiding a
  `trigger_manager.h` edit); did not implement test #28 fully (out of
  scope per the brief — added only the cheap guard assertion suggested
  as optional) or Heart of the Tempest's actual card behavior (also out
  of scope — a later task per the dispatch's framing).
- **Tests assert real fields and effects**: `play_source` read off the
  real `CardPlayedEvent` via a live `EventBus` subscription (not
  reimplemented/mocked), trigger effects asserted via
  `GameObject::card_counters` written by real `onTrigger` overrides
  fired through the real `TriggerManager`/chain machinery, not manual
  calls into card code.
- **File scope**: only the four allowed source files + the new test
  file + the one named header edit — confirmed via `git status
  --porcelain` before committing (see below).

## Concerns

1. **`executePlayCard`/`executePlaySpell`'s capture-before-mutation is
   currently redundant** (their own zone-removal blocks mutate
   `ps.hand`/`ps.champion_zone`/`ps.trash`, not `card.zone` itself), so
   `playSourceFor(card)` would give the same answer called before or
   after those blocks today. I still capture before, per the dispatch
   notes' explicit instruction and as defense against a future change
   to those blocks silently breaking `play_source`. Flagging in case a
   reviewer wants the capture moved to right before each emit instead
   (functionally identical today; I judged "matches the stated rule
   literally + safer against drift" as the better default).
2. **The `#4` test does not exercise `playIgnoringCost` directly on a
   spell object** (see rationale above) — it proves the mapping and
   dispatch through a real unit replay instead, which I judge is a
   *stronger* proof (real chain-driven trigger dispatch, CR-correct
   primitive usage) but is a deliberate deviation from the brief's
   literal test-#4 phrasing ("play the SPELL via playIgnoringCost").
   Flagging for the controller's awareness.
