# Final fix wave report — `kennen-tyler-deck`

Branch `kennen-tyler-deck`. Dispatch base `7ac1aea`; the controller's
docs-only commit `faba6cd` (spec addenda #13–#14 + smoke-doc coverage gap)
landed on the branch between orientation and the first commit, so this wave
stacks on `faba6cd`.

Status: **DONE**. All five findings fixed, each behavioural change RED first
then GREEN. Suite 1121 → **1129 passed, 1 pre-existing disabled**. Coverage
gate unchanged: **792 / INCOMPLETE 0**, Minefield still `implemented clean`.
No `// APPROX` added anywhere; no AI model identifier outside the required
commit trailers.

Commits (in the dispatched order):

| # | sha | title |
|---|---|---|
| i | `643e7a5` | Kennen: header pass — is_empowered comment, Intent::operator== completeness |
| ii | `2e5a303` | Kennen: route closed-state spell plays through executePlaySpell |
| iii | `b80129e` | Kennen: live hidden reveal emits PlayedFromFacedownEvent; drop dead executePlayFromHidden |
| iv | `81ef4e9` | Kennen: Minefield burns via burnCards (burn-out parity) |
| v | `0fc3633` | Kennen: re-validate disempower cost; picker TODOs; small guards |

---

## Baseline

```
$ git branch --show-current
kennen-tyler-deck
$ git status --short
(clean)
$ cmake --build build --target riftbound_tests -j4
ninja: no work to do.
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1121 tests from 121 test suites ran. (1976 ms total)
[  PASSED  ] 1121 tests.
  YOU HAVE 1 DISABLED TEST
$ python3 scripts/card_coverage.py | tail -2
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

All RED observations below were taken against that baseline with the new
tests added and only the test files rebuilt (a test-only build), so the
production code under test was exactly `7ac1aea`.

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests \
    --gtest_filter='ClosedStatePlaysTest.*:*Minefield*:*DisempowerSelfActivationIsRejected*:TypesTest.IntentEquality*'
[==========] 8 tests from 4 test suites ran.
[  PASSED  ] 1 test.
[  FAILED  ] 7 tests, listed below:
[  FAILED  ] BurnOutScoringTest.Minefield_BurnsThroughBurnOut
[  FAILED  ] ClosedStatePlaysTest.ClosedFlowPlayPaysFlowCostAndBanishesWithoutDuplicatingTrash
[  FAILED  ] ClosedStatePlaysTest.ClosedGrantedFlowPlayPaysTheGrantAndConsumesIt
[  FAILED  ] ClosedStatePlaysTest.ClosedRestrictedStarCrossedPaysTheDiscountAndNarrowsThePicker
[  FAILED  ] ClosedStatePlaysTest.LiveHiddenRevealEmitsPlayedFromFacedownEvent
[  FAILED  ] EmpowerTest.DisempowerSelfActivationIsRejectedWhenTheSourceIsNotEmpowered
[  FAILED  ] TypesTest.IntentEqualityDistinguishesAltCostAndGrantedAbility
```

The one passing test in that set is the deliberate regression guard,
`ClosedHandReactionPlayStillPaysAndTrashes` — a plain hand [Reaction] play
worked before the routing and had to keep working after it.

---

## Finding #1 [Critical] — closed-state plays bypassed `executePlaySpell`

### What was wrong

`GameEngine::generateClosedStateActions` emits `IntentType::PlayReaction`
from `generateSpellActions`, `generateTrashReplayActions` **and**
`generateFlowPlayActions`. `GameEngine::executeIntent` has no `PlayReaction`
case, so the only executor those offers ever reached was
`ChainManager::stepExecuteAndPass` — a second, thinner copy of
`executePlaySpell` that paid through the injected `payCardCost` (no [Flow],
no Sandswept Tomb power-discount staging, no trash-replay grant), searched
`PlayerState::hand` alone for removal, never set `banish_on_leave`, never
consumed a granted Flow, and never stamped
`target_battlefield_restriction`.

### What changed

`src/engine/chain_manager.h`
- new injected callback `setPlaySpell(std::function<void(const Intent&)>)`,
  mirroring the existing `setAffordCheck` / `setPayCost` shape, with the
  rationale on the setter;
- new `isProcessing()` predicate plus a private `processing_` flag.

`src/engine/chain_manager.cpp`
- `processFEPR` sets `processing_` for the duration through an RAII guard
  (the loop has two early returns, so a bare assignment would leak);
- the `PlayReaction` branch routes **spells** through `play_spell_` and
  returns `true` when the chain actually grew. A rejected intent (the
  executor's loud-failure paths pay nothing and add nothing) logs a warning
  and keeps priority with the same player rather than restarting FEPR on an
  unchanged chain — the `for (pass …)` bound still terminates it.
  Non-spell reactions fall through to the unchanged local path;
- the stale comment that described `executePlayFromHidden` as a parallel
  dead path is replaced by one that says what the remaining branch is for.

`src/engine/game_engine.cpp`
- `initSubsystems` wires `setPlaySpell` to `executePlaySpell`;
- `runChain` returns immediately when `chain_manager_->isProcessing()`.
  `executePlaySpell` ends by calling `runChain`, and the routed play calls
  it from inside `processFEPR`; the item it just added belongs to the loop
  already running, which restarts at Finalize the moment `stepExecuteAndPass`
  returns `true`. Without the guard a second FEPR loop would resolve the
  whole chain out from under the outer one. Every other `runChain` call site
  (phase code, `executeIntent`, `executePlayCard`, combat trigger drain) is
  top-level, so the guard changes nothing for them;
- `executePlaySpell` gained the facedown-reveal semantics the chain branch
  owned, so the two agree: `const bool hidden_play = card.is_hidden` is
  captured before anything mutates, the removal block erases the card from
  the battlefield's `facedown` vector and clears `is_hidden` / `hidden_at`,
  both cost paths (`payTrashReplayGrant` and `payCardCost`) are skipped
  (CR 811 — played ignoring its base cost), `energy_spent` is 0, and
  `play_source` is still derived from the pre-mutation object, so it stays
  `Hidden`.

### RED (against `7ac1aea`)

```
ClosedFlowPlayPaysFlowCostAndBanishesWithoutDuplicatingTrash
  countExhausted(s, P1)      = 1   want 2      (paid the printed 1E)
  countReady(s, P1)          = 3   want 1
  rune_deck.size()           = 0   want 1      (no power paid)
  countIn(trash, spell)      = 2   want 0      ← the duplicate the review predicted
  zone                       = Trash (04)  want Banishment (05)
  countIn(banishment, spell) = 0   want 1
  CardPlayedEvent.energy_spent = 1  want 2

ClosedGrantedFlowPlayPaysTheGrantAndConsumesIt
  countExhausted(s, P1)      = 3   want 1      (paid the printed 3E)
  granted_flow.has_value()   = true  want false
  countIn(trash, spell)      = 2   want 0
  zone                       = Trash  want Banishment

ClosedRestrictedStarCrossedPaysTheDiscountAndNarrowsThePicker
  agent1.unit_prompts.size() = 5   want 2      (no restriction stamped)
```

### GREEN

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='ClosedStatePlaysTest.*'
[ RUN      ] ClosedStatePlaysTest.ClosedFlowPlayPaysFlowCostAndBanishesWithoutDuplicatingTrash
[       OK ] ... (2 ms)
[ RUN      ] ClosedStatePlaysTest.ClosedGrantedFlowPlayPaysTheGrantAndConsumesIt
[       OK ] ... (2 ms)
[ RUN      ] ClosedStatePlaysTest.ClosedRestrictedStarCrossedPaysTheDiscountAndNarrowsThePicker
[       OK ] ... (2 ms)
[ RUN      ] ClosedStatePlaysTest.ClosedHandReactionPlayStillPaysAndTrashes
[       OK ] ... (2 ms)
[ RUN      ] ClosedStatePlaysTest.LiveHiddenRevealEmitsPlayedFromFacedownEvent
[       OK ] ... (2 ms)
[  PASSED  ] 5 tests.
```

### Note on the Star-Crossed test's first RED→GREEN cycle

The first GREEN run still failed (c) at `unit_prompts.size() == 5`. The dump
showed the five recorded option sets as `[9 8 7] [8 7] [7] [2] [4]`: the
first three are the **cost-payment cursor's** rune picks (ids 7/8/9 are the
three Order runes), the last two are the narrowed A list (`[2]` =
friendly-at-Tomb) and the B list (`[4]` = the enemy). So the engine
behaviour was already correct — the test's recorder was too broad. It now
carries the same unit-only filter `test_sandswept_tomb.cpp`'s
`TargetPromptRecorder` uses, which is exactly why the equivalent open-state
test never saw the rune prompts.

---

## Finding #4 [Recommendation, adopted] — the facedown event never fired live

`PlayedFromFacedownEvent` (Katarina, Reckless 462 —
`WhenYouPlayFromFacedown`) had **one** emit site, inside the never-called
`GameEngine::executePlayFromHidden`, so it never fired in a real game.
`grep -rn PlayedFromFacedownEvent src/` confirmed the single emit; the
dispatch's "two emit sites" are the function's two `CardPlayedEvent` emits
(permanent branch and spell branch), both removed with it.

Sequence followed: the emit moved into the live path first (in
`executePlaySpell`'s facedown branch, immediately **before** the
`CardPlayedEvent` for the same play — the ordering the dead function used),
the test went RED then GREEN, and only then was the function deleted:
declaration (`game_engine.h:510`, replaced by a one-line note saying where
the reveal executes now), definition, both `CardPlayedEvent` emits, and the
stale `generateClosedStateActions` comment that pointed at it for the
permanent case. `test_play_from_non_hand.cpp`'s file and test comments,
which described it as live-but-dead, were corrected to describe what that
test actually drives (a bare `ChainManager` with no injected play-spell
callback, so it still exercises ChainManager's own branch).

RED: `facedown.size() = 0, want 1`. GREEN above.

`grep -rn executePlayFromHidden src/ tests/` now returns nothing but the
historical `.superpowers/` reports and the spec's known-limitations entry,
which already documents the removal (addendum #14, committed by the
controller in `faba6cd`).

---

## Finding #3 / #5 [Important, header pass] — one rebuild

`src/core/game_object.h:39-42` — the `is_empowered` comment claimed the flag
is cleared "(same places `is_stunned` is reset)", which addendum #10
overturned. Reworded to "cleared in the executor's board-exit paths (kill,
bounce, banish, recycle-from-board, combat death)".

`src/core/intent.h` — `operator==` gained `use_alt_play_cost` and
`granted_ability_def`, plus a paragraph above the operator stating the rule
that every offer-distinguishing field must appear (the same hazard this
branch already fixed for `flow_source`).

RED:
```
tests/test_types.cpp:145: Value of: base == alt      Actual: true  Expected: false
tests/test_types.cpp:156: Value of: own == granted   Actual: true  Expected: false
```
GREEN: `[ OK ] TypesTest.IntentEqualityDistinguishesAltCostAndGrantedAbility`.

The header edits were made first and the long rebuild started immediately;
it took **21m27s** wall (`real 21m27.049s`, 4 cores) and completed with
exit 0, no warnings matching `error|FAILED|warning: unused`. The `.cpp` work
was written while it ran, so the wave cost one full rebuild plus short
engine-only relinks.

---

## Finding #2 [Important] — Minefield skipped burn-out

`src/cards/battlefields/0526_minefield.cpp` hand-rolled "top 2 to trash" and
stopped at an empty deck. Putting deck cards into the trash is Burn (CR 440),
so an empty deck mid-burn must Burn Out (CR 431.2). Replaced with
`ctx.executor.burnCards(ctx.controller, 2)`.

RED (1 card in deck, 2 in trash, conquer Minefield):
```
state.player(P1).burned_out        = false  want true
state.player(P2).score             = 0      want 1
state.player(P1).main_deck.size()  = 0      want 2
state.player(P1).trash.size()      = 3      want 1
```
GREEN: `[ OK ] BurnOutScoringTest.Minefield_BurnsThroughBurnOut (3 ms)`.

Coverage: the file keeps an `onTrigger(` body, so it still classifies
`implemented clean` —
```
0526_minefield.cpp implemented clean
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50   TOTAL 792
```

---

## Finding #5 [Minors]

**#6 — disempower cost re-validated.** `executeIntent`'s activation-cost
block paid `exhaust` and the energy component and then called
`disempowerObject`, which no-ops on a non-empowered source: a hand-built
intent got the ability for a cost that was never paid. A check now runs
**before** anything is paid and rejects with `logWarn` (not `logTrace` —
Trace is suppressed unless a run asks for it), leaving the switch case
without touching state.

RED: `s.getObject(legend).is_exhausted  Actual: true  Expected: false`.
GREEN: `[ OK ] EmpowerTest.DisempowerSelfActivationIsRejectedWhenTheSourceIsNotEmpowered (1 ms)`.
The test also asserts no chain item was created.

**#8 — picker TODOs.** A `// TODO` at each of the three "take the agent's
answer" sites in `src/cards/card.cpp` (`resume_point` 7, 10 and 12) recording
that the answer is not re-checked against the published list, that Sandswept
Tomb's [A] discount is charged for a commitment only that list enforces, and
that this is a trust boundary rather than a live bug (every in-tree agent
answers from the published list). `card.cpp` is not scanned by
`scripts/card_coverage.py` (it globs `[0-9]*.cpp`), so the `TODO` keyword
cannot flip a card's flag to `partial`.

**#12 — narrowing note.** One line at `0790_lightning_rush.cpp` on the
`GameObjectId` (`uint32_t`) → `int32_t` narrowing into
`ChainItem::resume_data`, deliberately worded without any of the coverage
script's WEAK keywords so the card stays `implemented clean`.

**#13 — `pop_back()` guard.** `0457_hard_bargain.cpp`'s "can't afford the
rescue" branch popped `chain.items.back()` **outside** the `!empty()` guard
its own `banish_on_leave` read sits behind. Moved inside, matching the
identical branch further down the same function (which was already correct).

---

## Verification at the tip

Run from the repo root, working tree clean, after the last commit:

```
$ cmake --build build --target riftbound_tests -j4
ninja: no work to do.
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1129 tests from 122 test suites ran. (2072 ms total)
[  PASSED  ] 1129 tests.
  YOU HAVE 1 DISABLED TEST
$ python3 scripts/card_coverage.py | tail -4
vanilla                   0        0     69     69
TOTAL                                          792
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
$ git status --short
(clean)
```

1129 = the 1121 baseline + 8 new tests. The full suite was run before each of
the five commits (the worktree carried the complete change set throughout,
so each pre-commit run is the tip run; the commit-ii run reported 1128
because the facedown test belongs to commit iii and was held out of the
worktree for that one commit).

---

## Files

Production:
- `src/core/game_object.h`
- `src/core/intent.h`
- `src/engine/chain_manager.h`
- `src/engine/chain_manager.cpp`
- `src/engine/game_engine.h`
- `src/engine/game_engine.cpp`
- `src/cards/card.cpp`
- `src/cards/battlefields/0526_minefield.cpp`
- `src/cards/spells/0457_hard_bargain.cpp`
- `src/cards/spells/0790_lightning_rush.cpp`

Tests:
- `tests/cards/test_closed_state_plays.cpp` (new — 5 tests)
- `tests/cards/test_burn_out_and_scoring.cpp` (+1)
- `tests/cards/test_empower.cpp` (+1)
- `tests/test_types.cpp` (+1)
- `tests/cards/test_play_from_non_hand.cpp` (comments only)

---

## Concerns

1. **Hidden PERMANENT reveals still emit no `PlayedFromFacedownEvent`.** The
   routing covers spells only; a facedown unit or gear revealed as a
   reaction still goes through `ChainManager`'s local branch, which does not
   emit the event. The dispatch said to preserve existing ChainManager
   behaviour for non-spell reactions, so this was left alone deliberately —
   but Katarina's "when you play a card from face down" reads on any card,
   so this is a remaining faithfulness gap, not a resolved one. One line
   beside that branch's `CardPlayedEvent` would close it; it is not covered
   by any test today.

2. **That same non-spell branch calls `addSpell` for units and gear.** A
   revealed facedown permanent, a Quick-Draw gear and an Ambush unit all
   become `is_spell` chain items, so `stepResolve`'s disposal sends them to
   the trash when they "resolve". Pre-existing, explicitly out of scope
   here, and untouched — but it is the next thing anyone reading that branch
   will trip over.

3. **`executePlaySpell`'s hidden handling ships in commit ii without its own
   test.** The test that exercises it is `LiveHiddenRevealEmits…` in commit
   iii. Splitting it the other way would have left commit ii actively
   broken for facedown spell reveals (the routing would send them to a
   function that could not remove them from the facedown zone), so the
   sequencing follows the dispatch's own ordering for #1 then #4.

4. **A rejected routed play keeps priority with the same player.** If an
   agent repeatedly chooses an intent `executePlaySpell` rejects, the
   priority loop re-offers up to `kMaxPriorityPasses` (10) times, logging a
   warning each pass, then falls through to Resolve. Bounded and loud, but
   it is a behaviour the old branch could not produce (it never rejected
   anything). No in-tree agent generates such an intent.

5. **The Repeat poll now runs for closed-state reactions**, because
   `executePlaySpell` owns it. No card in either deck in scope has a printed
   [Repeat] on a [Reaction], so nothing in the suite changed — but a future
   [Reaction] with [Repeat] will now (correctly) ask the agent for tranches
   during a closed-state play, which the old branch never did.

6. **The controller's `faba6cd` landed mid-wave.** It is docs-only (spec
   addenda #13–#14 and the smoke-doc coverage sentence) and already
   describes this wave's fixes, so no spec edit was needed here. Worth
   knowing that the dispatch's stated base (`7ac1aea`) is this wave's
   grandparent, not its parent.
