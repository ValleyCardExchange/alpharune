# Task 11a — closing the two reaction-path gaps from the final re-review

Branch `kennen-tyler-deck`, base `0fc3633`, three commits landed:

| sha | title |
| --- | --- |
| `e1e34bb` | Kennen: Repeat tranches run through the resume pump |
| `f6c8eec` | Kennen: showdown reaction plays execute (PlayReaction in executeIntent) |
| `0808d3b` | Kennen: fix Katarina card id in comments |

Status: **all three findings closed, TDD with RED recorded per test.** Full
suite green (1135 passed, 1 pre-existing disabled), coverage gate unchanged
(792 / INCOMPLETE 0). No core-header edits beyond `chain_manager.h`, which the
brief allowed. No `git add -A`, no `// APPROX`, no AI model identifier outside
the required trailers.

---

## Finding A — [Important] Repeat tranches cannot pump choices

### What I found on investigation

The brief's reading was correct and complete. `ChainManager::stepResolve`
(`src/engine/chain_manager.cpp`) popped the top item into
`state_.chain.resuming` and ran it inside a `requestChoice` / `takeChoice`
pump: call `resolve_spell`, and while the resolving Card has published a
pending choice, log it, ask the agent, `recordChoice`, re-enter. The `[Repeat]`
loop that followed re-invoked `resolve_spell` directly, once per
`repeats_paid`, with no pump at all — it only reset `resume_point` /
`resume_data` per tranche.

Consequences for a *resumable* card on a paid tranche:

1. only its `case 0` branch ever ran — the effect never happened; and
2. `EffectExecutor::pending_` was left ACTIVE when the card yielded, so the
   next `stepResolve`'s pump consumed the orphaned choice after resolving a
   **different** card and fed it to that card's re-entry.

Hard Bargain (457, `src/cards/spells/0457_hard_bargain.cpp`, in Tyler's deck)
is exactly this shape: `[Reaction]` + `[Repeat] [2]`, `hasLegalTargets` needs a
spell on the chain so it is only ever playable in the Closed State, and its
`case 0` yields via `requestChoice` whenever the counter target's controller
holds >= 2 ready runes. Its Repeat became reachable for the first time with
addendum #14's routing (closed-state spell reactions now execute through
`GameEngine::executePlaySpell` via `ChainManager::setPlaySpell`). Called Shot
(443) has the same shape.

I confirmed `parseRepeatCost` reads Hard Bargain's `[Repeat] [2]` as
`energy=2, power=0` (no trailing domain token), so the tranche is a plain
2-energy charge — relevant to sizing the rune budget in the test.

One thing the brief did not mention that the test had to account for: in the
Closed State priority goes to the **newest item's controller first**. A P1
agent that grabs its reaction at the first offer plays Hard Bargain before P2
can add the item beneath it, which leaves the paid tranche facing an empty
chain (`"nothing on chain to counter — no-op"`) and no yield at all. The first
RED run showed this (1 rescue question instead of 2), so the scripted agent
gained a state gate: P1 only plays Hard Bargain once the chain already holds
two items. That is a test-construction fix, not a production one.

### RED

New file `tests/cards/test_repeat_reactions.cpp`.

```
$ cmake --build build --target riftbound_tests -j4
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='RepeatReactionsTest.*'
```

```
[ RUN      ] RepeatReactionsTest.HardBargainRepeatTrancheCountersTheSecondSpell
test_repeat_reactions.cpp:416: Failure
Value of: rescues[1].counterspell_already_gone
  Actual: true
Expected: false
The tranche's question must be asked DURING Hard Bargain's resolution, not leaked to whatever resolves next

test_repeat_reactions.cpp:422: Failure
Expected equality of these values:
  g_tally_resolves
    Which is: 2
  0
Tally was countered by the Repeat tranche

[  FAILED  ] RepeatReactionsTest.HardBargainRepeatTrancheCountersTheSecondSpell (4 ms)
[ RUN      ] RepeatReactionsTest.ResumableRepeatTranchesRunThroughTheResumePump
test_repeat_reactions.cpp:473: Failure
  agent_answers  Which is: 1   vs   3
every tranche's published choice must reach the agent
test_repeat_reactions.cpp:475: Failure
  g_resumable_done  Which is: 1   vs   3
every tranche must consume its answer and finish its effect
test_repeat_reactions.cpp:477: Failure
Value of: exec.hasPendingChoice()
  Actual: true
Expected: false
a tranche must not leave a pending choice for the next card

[  FAILED  ] RepeatReactionsTest.ResumableRepeatTranchesRunThroughTheResumePump (2 ms)
[ RUN      ] RepeatReactionsTest.NonResumableRepeatResolvesExactlyRepeatsPaidPlusOne
[       OK ] RepeatReactionsTest.NonResumableRepeatResolvesExactlyRepeatsPaidPlusOne (2 ms)

 2 FAILED TESTS
```

The RED is precisely the described failure, and it is worth spelling out what
`g_tally_resolves == 2` means: the tranche's orphaned choice was picked up by
the *next* card's `stepResolve`, whose pump therefore looped once more and
resolved that innocent card **twice**. So the pre-fix behaviour was not merely
"the tranche did nothing" — it also double-resolved an unrelated chain item.

The three tests:

1. **`HardBargainRepeatTrancheCountersTheSecondSpell`** — real FEPR loop,
   `GameEngine` + scripted agents. P1 opens the chain with a free `[Action]`
   tally spell; P2 answers with a free `[Reaction]`; P1 then plays the real
   Hard Bargain (457) from the closed-state offer and buys exactly one Repeat
   tranche through the engine's own yes/no poll. Both counter targets'
   controllers hold >= 2 ready runes, so both resolutions must ask the rescue
   question; the agent declines both. Asserts: two rescue questions, each
   addressed to the right player about the right spell, **both asked while
   Hard Bargain is still the resolving item** (the agent records whether the
   counterspell object has already left the chain zone — the direct signal
   that the question was not leaked), neither target spell resolved, both in
   trash, Hard Bargain in trash, chain empty.
2. **`ResumableRepeatTranchesRunThroughTheResumePump`** — ChainManager level,
   where the test owns the `EffectExecutor` and can assert
   `hasPendingChoice() == false` directly (the brief's explicit ask). A minimal
   resumable spell in Hard Bargain's shape, `repeats_paid = 2`: 3 entries into
   `case 0`, 3 answers reaching the agent, 3 completed effects, no pending
   choice left over.
3. **`NonResumableRepeatResolvesExactlyRepeatsPaidPlusOne`** — the regression
   the brief asked for; green before and after.

### GREEN

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='RepeatReactionsTest.*'
[       OK ] RepeatReactionsTest.HardBargainRepeatTrancheCountersTheSecondSpell (4 ms)
[       OK ] RepeatReactionsTest.ResumableRepeatTranchesRunThroughTheResumePump (2 ms)
[       OK ] RepeatReactionsTest.NonResumableRepeatResolvesExactlyRepeatsPaidPlusOne (2 ms)
[  PASSED  ] 3 tests.
```

### The change

`ChainManager::stepResolve` — the pump body is factored into a
`runResolutionPump` lambda called for the base resolution and again for every
tranche. `resume_point` / `resume_data` are still reset per tranche exactly as
before, so each tranche starts at the card's `case 0` with a clean slate;
targets and the rest of the item are re-used as-is (the documented
`ChainItem::repeats_paid` simplification is unchanged). One incidental
tightening: the pump now reads `state_.chain.resuming->is_spell /
is_ability` rather than the local `resolved` copy — identical on the first
pass, and correct rather than incidental on a tranche.

**On the brief's `resuming`-lifetime escape hatch: no conflict, nothing to
stop for.** The disposal block already reads the item back out of
`state_.chain.resuming` *after* the Repeat loop
(`resolved = std::move(state_.chain.resuming.value())`), so the slot was
required to stay live across every tranche before this change and still is.
The pump only adds agent queries between `resolve_spell` calls; it does not
move, reset, or re-seat the slot.

---

## Finding B — [Important] Showdown reaction plays silently no-op

### What I found on investigation

Verified, and the no-op is real. `GameEngine::executeIntent`'s switch had cases
for `PlayCard` / `PlayActionCard`, the activate family, and the housekeeping
intents; `IntentType::PlayReaction` hit `default: break`. Since
`resolveShowdownDecision` dispatches `PlayReaction` into `executeIntent`, the
intent was consumed and discarded — and because `resolveShowdownDecision`
clears the focus-pass set and hands focus to the opponent regardless, the loop
looked healthy while nothing had been played, nothing paid, and the card left
in hand.

Two corrections to the brief's line references, both checked against the file:

* **Only the Pouncing block (`generateShowdownActions`, `:~2790`) is a showdown
  generator.** The Quick-Draw (`:~2882`) and Ambush (`:~2907`) blocks the brief
  cites are inside `generateClosedStateActions` (which starts at `:2851`), not
  the showdown generator. Ambush *in a showdown* is emitted as
  `PlayActionCard`, which already had a case. So the one production card class
  actually broken by this in a showdown is the Pouncing units — and Rengar,
  Pouncing (348) is in the Rengar deck, as the brief says.
* **Reaction spells in a showdown are emitted as `PlayActionCard` too.**
  `generateSpellActions` only tags an offer `PlayReaction` when
  `state_.turn.isClosedState()`; in `isShowdownOpen()` it emits
  `PlayActionCard`. So spells were not silently dropped in showdowns either.
  The new case still routes them correctly (hand-built intents, the
  `testHook_executeIntent` seam, and any future generator), and the brief asked
  for that test, so it is covered.

I also confirmed the double-execution question the brief flagged: **the two
paths are disjoint.** `executeIntent` has exactly two production callers,
`resolveMainPhaseDecision` (`:888`) and `resolveShowdownDecision` (`:3821`);
`ChainManager` never calls `executeIntent` (it reaches spells via the injected
`setPlaySpell` callback and handles non-spells in its own local branch). So the
new case cannot double-execute a closed-state play.

One thing the fix needed that the brief did not name: `resolvePermanent`'s
Quick-Draw auto-attach reads the **chain item's** `targets`, but
`ChainManager::addPermanent` had no targets parameter at all, so the gear play
path could not attach no matter which executor called it. That is the reason
the Quick-Draw test could not pass on `executePlayCard` alone.

### RED

Tests added to `tests/cards/test_combat_showdown_dispatch.cpp` (its existing
note explicitly deferred this coverage — see below).

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='CombatShowdownDispatchTest.*'
[ RUN      ] CombatShowdownDispatchTest.ResolveShowdown_PouncingUnit_LandsAtBattlefieldAsCombatant
Value of: obj.location.has_value()
  Actual: false
Expected: true
[  FAILED  ] ... (2 ms)

[ RUN      ] CombatShowdownDispatchTest.ResolveShowdown_QuickDrawGear_AttachesToItsTarget
test_combat_showdown_dispatch.cpp:357: Failure
Value of: inHandIn(s, P1, gear)   Actual: true   Expected: false
the gear play must leave hand
test_combat_showdown_dispatch.cpp:359: Failure
Value of: g.attached_to.has_value()   Actual: false   Expected: true
Quick-Draw attaches the gear as it enters
[  FAILED  ] ... (2 ms)

[ RUN      ] CombatShowdownDispatchTest.ResolveShowdown_ReactionSpell_ResolvesThroughTheChain
  g_showdown_reaction_resolves  Which is: 0   vs   1   the reaction spell must actually resolve
Value of: inHandIn(s, P1, spell)   Actual: true   Expected: false
Value of: inTrashIn(s, P1, spell)  Actual: false  Expected: true
[  FAILED  ] ... (2 ms)

 3 FAILED TESTS
```

The Pouncing test takes its intent from `engine.generateLegalActions()` — the
real `generateShowdownActions` offer, not a hand-built one — so the RED proves
the round trip the brief asked about: the generator publishes the play, the
showdown decision path consumes it, and the unit never reaches the board.

### GREEN

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='CombatShowdownDispatchTest.*'
[       OK ] ResolveShowdown_PassFocus_OneSided_PassesToOpponent (3 ms)
[       OK ] ResolveShowdown_PassFocus_BothSidesClosesShowdown (2 ms)
[       OK ] ResolveShowdown_UnknownIntent_HoldsFocus (2 ms)
[       OK ] ResolveShowdown_PouncingUnit_LandsAtBattlefieldAsCombatant (2 ms)
[       OK ] ResolveShowdown_QuickDrawGear_AttachesToItsTarget (2 ms)
[       OK ] ResolveShowdown_ReactionSpell_ResolvesThroughTheChain (2 ms)
[  PASSED  ] 6 tests.
```

### The changes

* `GameEngine::executeIntent` — `case IntentType::PlayReaction` added,
  fall-through-joined to the existing `PlayCard` / `PlayActionCard` arm, so it
  routes by card type into the same executors every other play uses: spells to
  `executePlaySpell` (chain, `[Flow]`, Sandswept Tomb, facedown reveal), units
  and gear to `executePlayCard` (which already honours `intent.play_location`,
  so a Pouncing / Ambush unit lands at its battlefield).
* `ChainManager::addPermanent` — takes an optional `targets` list (defaulted
  empty) and stamps it on the item; `chain_manager.h` doc updated. This is the
  one header edit, which the brief allowed.
* `GameEngine::executePlayCard` — passes `intent.targets` to `addPermanent`.
  Audited for blast radius: `resolvePermanent` is the only consumer of a
  permanent item's targets, and its only use is guarded by
  `card.isGear() && card.hasKeyword(Keyword::QuickDraw)`. The main-phase gear
  generator emits no targets, so no existing play changes behaviour — and the
  full suite confirms it.
* Replaced the stale block comment in `test_combat_showdown_dispatch.cpp` that
  deferred this coverage to "integration smoke runs producing the expected
  `INTENT: PlayReaction` trace lines". That line is logged at the top of
  `executeIntent`, *above* the switch, so the silent no-op printed it too — the
  stated verification could never have caught this bug. Worth flagging as a
  pattern: a trace line emitted before the dispatch is not evidence of
  dispatch.

---

## Finding C — [Minor] Wrong card id in four comments

Verified against the sources: `src/cards/units/0585_katarina_reckless.cpp`
registers **585** and is the card with
`TriggerType::WhenYouPlayFromFacedown`; `src/cards/units/0462_irelia_graceful.cpp`
registers 462. Four comments named Katarina as 462; corrected to 585 —
`game_engine.cpp:1565`, `game_engine.cpp:1918`,
`test_closed_state_plays.cpp:31`, `test_closed_state_plays.cpp:591`. (Line
numbers shifted from the brief's `~1536` / `~1889` by the Finding B comment
block landing earlier in the file.)

The three *Irelia, Graceful (462)* comments elsewhere in `game_engine.cpp`
(`:1788`, `:5545`, `:5728`) are correct and were left alone. Comment-only
change; suite re-run green before the commit.

---

## Suite and gate

Run from the repo root before each of the three commits; final state:

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1135 tests from 123 test suites ran. (2005 ms total)
[  PASSED  ] 1135 tests.
  YOU HAVE 1 DISABLED TEST

$ python3 scripts/card_coverage.py
bucket           engine-gap  partial  clean  total
implemented              50       44    622    716
engine-handled            0        0      7      7
metadata-only             0        0      0      0
stub                      0        0      0      0
vanilla                   0        0     69     69
TOTAL                                          792

INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

1129 baseline + 6 new = 1135; 1 pre-existing disabled; coverage gate unchanged
at 792 / INCOMPLETE 0. `git status` clean at every commit and at hand-off.

## Files

Production:

* `src/engine/chain_manager.cpp` — resume pump factored out and run per Repeat
  tranche; `addPermanent` carries targets.
* `src/engine/chain_manager.h` — `addPermanent` signature + doc.
* `src/engine/game_engine.cpp` — `PlayReaction` case in `executeIntent`;
  `executePlayCard` passes targets; two Katarina id comments.

Tests:

* `tests/cards/test_repeat_reactions.cpp` — new, 3 tests (Finding A).
* `tests/cards/test_combat_showdown_dispatch.cpp` — 3 tests added, stale
  coverage note replaced (Finding B).
* `tests/cards/test_closed_state_plays.cpp` — two Katarina id comments.

## Concerns

1. **Pre-existing, out of scope, and worse than Finding B: a closed-state
   non-spell reaction play puts the card in the TRASH.**
   `ChainManager::stepExecuteAndPass`'s non-spell branch
   (`chain_manager.cpp:~296-322`) ends by calling `addSpell(...)` for Ambush
   units, Pouncing units, Quick-Draw gear and facedown permanents. `addSpell`
   sets `is_spell = true` and moves the object to the Chain zone, so
   `stepFinalize` does **not** resolve it as a permanent (`is_permanent` is
   false), `stepResolve` runs it through `resolve_spell` (a no-op for a
   `UnitCard`), and the disposal block's `if (resolved.is_spell)` then trashes
   it. I verified this empirically with a throwaway probe (since deleted, tree
   clean): Rengar, Trophy Hunter (682) played as a closed-state `[Ambush]`
   reaction pays its full 5E + 1 Body, leaves hand, and ends with
   `zone = Trash`, no location, chain empty — the unit never reaches the
   battlefield. That branch should call `addPermanent` (now that it takes
   targets, Quick-Draw would attach correctly too), but it is not in this
   task's scope and I did not touch it. **Recommend a follow-up task.** Note
   this is the mirror image of Finding B: the showdown path dropped the play
   entirely, the closed-state path executes it into the wrong zone.
2. `ChainItem::repeats_paid`'s documented simplification still stands — every
   tranche re-uses the original `targets`. Hard Bargain re-reads the chain top
   at each `case 0` so it retargets naturally, but a `[Repeat]` card that
   commits its targets at play time will repeat against the *same* target. Not
   a regression, and not something this task changed.
3. The Finding A end-to-end test depends on closed-state priority order (newest
   item's controller first) via its agent gate. That is real engine behaviour,
   but if priority ordering is ever revised the gate is the thing that will
   need updating, not the assertions. The comment at the gate says so.
4. The showdown tests use `RandomAgent` (seeds 42/43, inherited from the
   fixture) to answer the cost-payment cursor. Deterministic today because
   every legal rune pick is equivalent for those cards; a future card with a
   meaningful rune choice in the same tests would want a scripted agent.

---

# Fix round 1 — review response

Two commits on top of `0808d3b`:

| sha | title |
| --- | --- |
| `2cf764b` | Kennen: closed-state permanent reactions execute through executePlayCard |
| `da9a9a2` | Kennen: comment corrections and pump consistency |

Status: **all three review items addressed.** Suite 1139 passed (1135
baseline + 4 new), 1 pre-existing disabled; coverage unchanged
(792 / INCOMPLETE 0). Files touched: `chain_manager.h`, `chain_manager.cpp`,
`game_engine.cpp`, `test_closed_state_plays.cpp`,
`test_combat_showdown_dispatch.cpp`, `test_repeat_reactions.cpp`. **No
`game_engine.h` change was needed** — the callback wiring reuses the existing
`initSubsystems` body and `executePlayCard` was already declared.

---

## Item 2 [Controller ruling] — closed-state permanents land on the board

### RED

Four tests appended to `tests/cards/test_closed_state_plays.cpp`, all driven
through the real FEPR loop the Hard Bargain test uses.

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='ClosedStatePlaysTest.Closed*Unit*:ClosedStatePlaysTest.Closed*Gear*:ClosedStatePlaysTest.ClosedFacedownPermanent*'
[ RUN      ] ClosedStatePlaysTest.ClosedPouncingUnitLandsAtTheAttackedBattlefield
Value of: obj.location.has_value()   Actual: false   Expected: true
the unit must be somewhere on the board
[  FAILED  ] ... (4 ms)

[ RUN      ] ClosedStatePlaysTest.ClosedAmbushUnitLandsReadyAtItsBattlefield
  countIn(s.player(P1).trash, nidalee)  Which is: 1   vs   0
an [Ambush] unit must not be disposed as if it were a spell
Value of: obj.location.has_value()   Actual: false   Expected: true
[  FAILED  ] ... (2 ms)

[ RUN      ] ClosedStatePlaysTest.ClosedQuickDrawGearAttachesToTheNamedUnit
  countIn(s.player(P1).trash, gear)  Which is: 1   vs   0
gear must not be disposed as if it were a spell
Value of: g.attached_to.has_value()   Actual: false   Expected: true
[  FAILED  ] ... (2 ms)

[ RUN      ] ClosedStatePlaysTest.ClosedFacedownPermanentRevealEntersPlayForFree
  played[1].energy_spent  Which is: 2   vs   0
  facedown.size()  Which is: 0   vs   1u
the permanent reveal must emit PlayedFromFacedownEvent exactly once
[  FAILED  ] ... (2 ms)

 4 FAILED TESTS
```

The RED confirms the reviewer's analysis exactly, and surfaced one extra fact
worth recording: the facedown-permanent reveal reported `energy_spent == 2`
in its `CardPlayedEvent` even though the branch never called `pay_cost_` for a
hidden play. Nothing was actually charged; the EVENT lied, so every "energy
spent this turn" consumer (Jhin's `max_spell_spent_this_turn`, Forgotten
Library / Virtuoso) was fed a cost that was never paid.

### GREEN

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='ClosedStatePlaysTest.*'
[       OK ] ClosedFlowPlayPaysFlowCostAndBanishesWithoutDuplicatingTrash (4 ms)
[       OK ] ClosedGrantedFlowPlayPaysTheGrantAndConsumesIt (2 ms)
[       OK ] ClosedRestrictedStarCrossedPaysTheDiscountAndNarrowsThePicker (2 ms)
[       OK ] ClosedHandReactionPlayStillPaysAndTrashes (2 ms)
[       OK ] LiveHiddenRevealEmitsPlayedFromFacedownEvent (2 ms)
[       OK ] ClosedPouncingUnitLandsAtTheAttackedBattlefield (2 ms)
[       OK ] ClosedAmbushUnitLandsReadyAtItsBattlefield (2 ms)
[       OK ] ClosedQuickDrawGearAttachesToTheNamedUnit (2 ms)
[       OK ] ClosedFacedownPermanentRevealEntersPlayForFree (2 ms)
[  PASSED  ] 9 tests.
```

### The change, per the reviewer's shape

* `ChainManager::setPlayCard` / `play_card_` — sibling of `setPlaySpell`,
  wired in `initSubsystems` to `GameEngine::executePlayCard`. The
  `stepExecuteAndPass` branch now picks the executor by card type
  (`card.isSpell() ? play_spell_ : play_card_`) and uses the identical
  chain-grew-so-it-happened check for both. **Nothing is hand-rolled**: no
  `addPermanent` call, no location stamp, no cost payment inside ChainManager.
* Real cards used, not stand-ins: Rengar, Pouncing (348) and Nidalee, Cat Form
  (676) from `decks/rengar_test.txt`, and Cloth Armor (387) for Quick-Draw.
  The Pouncing test asserts the unit is in
  `state.unitsAt(BattlefieldLocation{bf0}, P1)` — a combatant on the attacking
  side — not merely "not in trash". The Ambush test asserts it enters READY
  (CR: an Ambush unit entering a battlefield enters ready). The Quick-Draw
  test asserts `attached_to == bearer` and that `gears_played_this_turn`
  incremented, which is the observable proof the real gear play path ran
  rather than a copy.

### Hidden permanents

`executePlayCard` gained the facedown-reveal half `executePlaySpell` already
had, since a hidden permanent takes the same branch. Verified item by item
against the code the ChainManager copy used to run:

* **facedown-zone removal** — moved into `executePlayCard`, same loop.
* **`is_hidden` / `hidden_at` clear** — same, and `hidden_at` is captured into
  a local *before* the clear because the landing default now reads it.
* **play source** — `playSourceFor(card)` is called before the removal block,
  so `CardPlayedEvent.play_source == Hidden` still holds; the test asserts it.
* **zero cost (CR 811)** — the reveal now skips `payCardCost`, the Brazen-style
  prepay decision AND `maybePayOptionalAdditionalCost`. All three modify the
  base cost the reveal is explicitly ignoring. The test asserts
  `countExhausted(P1) == 0`.
* **reported energy** — now 0 for a reveal, fixing the lie the RED exposed.
* **`PlayedFromFacedownEvent`** — YES, emitted for permanents too, and the
  test asserts it. Katarina, Reckless (585) reads "when you play a card from
  face down", which is card-type agnostic; with `executePlayCard` as the one
  executor for permanents, emitting it there is the faithful behaviour, and it
  is ordered before `CardPlayedEvent` exactly as the spell half orders it.

**One judgment call, flagged for the reviewer.** A hidden play carries no
`play_location` (the closed-state hidden generator emits none), so the landing
default had to be decided. I used the battlefield the card was hidden at
(`hidden_at`), not the controller's base: CR 811 reveals the card where it lay,
and it is the same locus CR 811.1.d.2 already uses to restrict a hidden
SPELL's targets to that battlefield. Base would put a unit revealed from a
battlefield back at home, which reads wrong for the cards that have the
keyword (Tideturner 199, the Teemos, Edge of Night 460). It is one line
(`default_loc`) if you want it the other way. Strictly speaking CR 355.2.a
would let the controller CHOOSE base or a battlefield they control; offering
that choice is a generator-side gap, not this fix's, and I did not widen scope
to add it.

### The bare-ChainManager fallback — one thing I did not simply delete

My first cut removed the hand-rolled block outright. That turned two existing
tests red:

```
[  FAILED  ] PlayFromNonHandTest.HiddenCardRevealedAsReactionFiresWithHiddenSource
[  FAILED  ] ChainTestFixture.ReactionAddsToChainAndRestarsFEPR
```

Both drive `processFEPR` on a bare `ChainManager` with **no GameEngine and
therefore no injected callback**, deliberately — `test_play_from_non_hand.cpp`
says so in a comment at the test. Both play a SPELL.

So the fallback stays, narrowed: the minimal local play now runs **only for
spells**, and only when no executor is injected. A permanent reaction with no
executor is rejected with a `logWarn` naming `setPlayCard`, because hand-rolling
a permanent play is precisely the bug — it needs location stamping, cost
payment, `onPlay`, the gear counters and target carry-over, all of which live
in `executePlayCard`. `setPayCost` therefore stays too (I had removed it as
dead); its doc now says it serves only that fallback. Both tests are green
again with no test-side edits.

---

## Item 1 [Important] — comments stating a false premise

All five sites corrected, and I re-grepped to confirm none remain:

```
$ grep -rn "no PlayReaction case\|has no case for them" src/ tests/
(no matches)
```

* `src/engine/chain_manager.h` (`setPlaySpell` / new `setPlayCard` doc) and
  `src/engine/chain_manager.cpp` (`stepExecuteAndPass`) — rewritten to the
  real topology: `executeIntent` DOES have a `PlayReaction` case, it serves the
  showdown decision path, ChainManager answers the closed-state offers, and the
  two are disjoint **because ChainManager never calls executeIntent** — which
  is the load-bearing reason, and the one that survives future changes to
  `executeIntent`'s switch. Both docs also now record what each hand-rolled
  copy got wrong, so the reason the callbacks exist is not lost.
* `tests/cards/test_closed_state_plays.cpp:1-33` — file header rewritten; it
  now covers both halves and points at
  `test_combat_showdown_dispatch.cpp` for the showdown side.
* `src/engine/game_engine.cpp` `resolveShowdownDecision` (the pre-existing
  stale one) — it claimed "Quick-Draw gear uses PlayReaction" as a showdown
  case. Re-checked against `generateShowdownActions`: that generator emits
  `PlayActionCard` for Action/Reaction spells and Ambush units, `PlayReaction`
  only for reaction-to-attack units, and `ActivateActionAbility` for
  `[Action]` abilities. Quick-Draw is a `generateClosedStateActions` block and
  never reaches this switch. The comment now states that per-generator, and
  labels the remaining switch arms as safety nets for hand-built intents. The
  inline `case` comments were corrected to match.
* `tests/cards/test_combat_showdown_dispatch.cpp` Quick-Draw test — carries
  the clarifying note the reviewer asked for: hand-built intent, no generator
  emits it into a showdown, what it pins is the gear arm of the new case.

## Item 3 [Minors] — folded

* `hasRecordedChoice()` exists on `EffectExecutor`, so both leak assertions in
  `test_repeat_reactions.cpp` now check it alongside `hasPendingChoice()` —
  an answer recorded but never taken is the same leak seen from the other end,
  and would likewise be handed to whatever card resolves next.
* `stepResolve` reads the `resuming` slot in one style throughout: members via
  `->`, the whole item as `*state_.chain.resuming` (the pump previously mixed
  `->` with `.value()` on adjacent lines, and the disposal used `.value()`).
  Documented at the pump, including why no local reference is bound across a
  `resolve_spell` call — that call runs Card code which writes the slot.

## Suite and gate

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1139 tests from 123 test suites ran. (2007 ms total)
[  PASSED  ] 1139 tests.
  YOU HAVE 1 DISABLED TEST

$ python3 scripts/card_coverage.py
TOTAL                                          792
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

Run from the repo root before each commit; `git status` clean at hand-off.

## Concerns carried forward

1. **The hidden-permanent landing location is my call, not the spec's** — see
   above. `hidden_at`, defensible from CR 811 / 811.1.d.2, one line to change.
2. **CR 355.2.a choice for hidden plays is still not offered.** The
   closed-state hidden generator emits a bare `PlayReaction` with no
   `play_location`, so the controller never picks base-or-battlefield for a
   revealed permanent. Generator-side gap; out of scope here.
3. Concerns 2–4 from the original report still stand unchanged (the
   `repeats_paid` targets simplification, the priority-order dependence of the
   Hard Bargain test's agent gate, and `RandomAgent` answering the cost cursor
   in the showdown tests).

---

# Fix round 2 — review response

One commit on top of `c22a35a` (another agent's action-vocab commit, which
landed on the branch mid-round; only my own files were touched):

| sha | title |
| --- | --- |
| `c561d2e` | Kennen: executor-topology comments; keep .value() in stepResolve |

Status: **both items addressed.** Suite 1143 passed (the new baseline), 1
pre-existing disabled; coverage unchanged (792 / INCOMPLETE 0). Files:
`src/engine/game_engine.cpp`, `src/engine/chain_manager.cpp`. Comment- and
safety-only; no behaviour change, so no new tests.

## Item 1 [Important, comment] — stale executor topology

* `executeIntent`'s PlayReaction block still said closed-state non-spells
  "take the chain's own local branch". Reworded to the topology `2cf764b`
  created, laid out per generator so the two answers read as the distinct
  paths they are: CLOSED STATE → `ChainManager::stepExecuteAndPass`, which
  picks by card type and calls back out through `play_spell_`
  (→ `executePlaySpell`) or `play_card_` (→ `executePlayCard`), its one local
  play being the SPELLS-ONLY no-executor fallback that only unit tests reach;
  SHOWDOWN → `resolveShowdownDecision` → here. The disjointness argument
  (ChainManager never calls executeIntent) is unchanged and still stated.
* `runChain`'s re-entrancy guard comment named only `executePlaySpell` as the
  nested caller. `executePlayCard` is now a second — a closed-state Pouncing
  or Quick-Draw play re-enters `runChain` exactly the same way — so both are
  named and the guard's rationale is stated once for both.

Requested sweep, run over `src/` and `tests/`:

```
$ grep -rn "local branch" src/ tests/      → no matches
$ grep -rn "own local"    src/ tests/      → no matches
$ grep -rni "non-spell"   src/ tests/      → 13 matches, all checked
```

Every surviving `non-spell` is correct: accurate present tense (the *half* of
the branch in `chain_manager.h:117`'s `@see`, `executePlayCard`'s facedown
comment, the test file's section heading), past tense describing the fixed bug
(`chain_manager.h:104`, `test_closed_state_plays.cpp:21`/`661`), or unrelated
card-text uses in `trigger_manager.cpp`, Ravenbloom Conservatory, Karma,
Diana, `events.h` and two trash-eligibility tests.

## Item 2 [Minor] — guarded loop, unguarded deref

Picked **restore `.value()`**, not drop the guard, and applied it to both
whole-item reads rather than only the one line flagged.

The asymmetry was mine: `da9a9a2` unified the dereference style to `*` and in
doing so turned the disposal into an unguarded read of a slot the loop three
lines above had just felt the need to guard. The guard is not superstition —
both whole-item reads (`resolve_spell`'s argument and the disposal) happen
*after* arbitrary Card code has run, across possibly several Repeat tranches,
and nothing structurally forbids a Card from clearing `state_.chain.resuming`.
For a read in that position a defined `std::bad_optional_access` is strictly
better than undefined behaviour, and keeping the guard keeps the loop honest
about the same possibility.

Member reads keep `->`: each of those sits immediately after the statement or
guard that establishes the slot is populated, so they are not in the same
category. The house-style comment now states the rule as "members through
`->`, whole item through `.value()`" **and** why the split is not cosmetic —
the previous version claimed one flat style, which is what let the unsafe
version look tidy.

## Suite and gate

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1143 tests from 123 test suites ran. (2041 ms total)
[  PASSED  ] 1143 tests.
  YOU HAVE 1 DISABLED TEST

$ python3 scripts/card_coverage.py
TOTAL                                          792
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

Concerns unchanged from fix round 1 (the hidden-permanent landing location is
still my `hidden_at` judgment call; CR 355.2.a's base-or-battlefield choice for
hidden plays is still not offered by the generator).
