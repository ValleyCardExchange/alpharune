# Task 4 report — Flow: offer and pay

Branch `kennen-tyler-deck`, base HEAD `37fddf4`, commit `aa9f080`.

## Implemented

`src/engine/game_engine.cpp` only (no headers touched).

### `liveFlowCosts(obj)` (was a `return {}` stub)
Returns every live Flow permission on an object, per spec addendum #1:

- **Printed** — `obj.keywords.has(Keyword::Flow)` and a valid def id →
  `card_registry_.get(obj.card_def_id)->flowCost()`, pushed only when
  `fc.valid`.
- **Granted** — `obj.granted_flow` present AND
  `granted_flow->valid_on_turn == state_.turn.turn_number` (evaluated
  expiry, not scheduled) → a `Card::FlowCost` built from the grant's
  `{energy, power, power_domain, any_domain}`.

Both are returned when both are live; the controller chooses
(CR 829.1.c.3). Guards on `kInvalidId` / `objectExists`.

### `generateFlowPlayActions(player, action_ok, reaction_ok, actions)` (was empty)
Modelled line-for-line on `generateTrashReplayActions`, iterating
`ps.trash` instead of the grant list:

- same `cant_play_cards_this_turn` / `cant_play_spells_this_turn` guards;
- skips non-existent objects, objects whose zone is not `Trash`, non-spells;
- **identical timing gate**: `has_reaction` implies `has_action`, then
  `isNeutralOpen()` → any / `isShowdownOpen()` → action-or-reaction /
  `isClosedState()` → `has_reaction && reaction_ok` / else the
  `action_ok`/`reaction_ok` pair;
- `hasLegalTargets` gate, `enumerateLegalTargets`, `getTargetRequirements`;
- same intent-type selection (`PlayCard` / `PlayActionCard` / `PlayReaction`);
- **one intent per live offer** that is affordable — affordability via
  `canPayAdditionalCost`, looping every `Domain` when `any_domain`;
- same target shape: `needsPlayTimeTarget`/`needsPlayTimeTargetPair` → one
  empty-target intent, `count == 0` or optional-with-no-targets → one
  empty-target intent, `count == 2` → the friendly × enemy product, else one
  intent per legal target;
- every emitted intent carries `play_source = Trash` and
  `flow_source = offer.source`.

`generateTrashReplayActions` is untouched.

### Call sites — all three, right after `generateTrashReplayActions`
- `generateMainPhaseActions` (`action_ok=true, reaction_ok=true`)
- `generateShowdownActions` (`action_ok=true, reaction_ok=true`)
- `generateClosedStateActions` (`action_ok=false, reaction_ok=true`)

### `executePlaySpell` payment branch
Inserted after the existing source-zone removal (the Trash branch already
erases from `ps.trash`) and before the grant/printed payment:

- when `intent.flow_source != None`, **recompute** the offers with
  `liveFlowCosts(intent.card)` and take the one whose `source` matches the
  intent — never trusting cost data carried on the intent;
- pick the first payable domain when `any_domain`, then
  `payAdditionalCost(player, energy, power, domain)`;
- log `FLOW: <name> played from trash for [E<n>][P<n>] (printed|granted)`;
- `card.granted_flow.reset()` — a granted Flow is consumed by the play, so a
  later return to trash cannot reuse it;
- `payTrashReplayGrant`, `payCardCost` and `maybePayOptionalAdditionalCost`
  (the Brazen/optional-cost riders) are all skipped by widening the two
  existing guards to `!paid_via_flow`, exactly as `use_alt_play_cost` does
  on the hand path;
- `energy_spent` is set to the flow energy actually paid when
  `paid_via_flow`, so `CardPlayedEvent::energy_spent`,
  `ps.last_spell_energy_spent`, `ps.max_spell_spent_this_turn` and the chain
  item's `total_energy_spent` all report the flow cost, not the printed one;
- after `chain_manager_->addSpell(...)`, the chain item whose `id ==
  chain_id` gets `banish_on_leave = true` (the `(void)chain_id;` line is
  replaced by a real use). **No disposal is implemented — that is Task 5.**

## TDD — RED/GREEN per test

Build: `cmake --build build --target riftbound_tests -j4`.
Run: `cd build && ./riftbound_tests --gtest_filter='FlowTest.*'`
(the full suite must be run from `build/` — two shipped tests resolve
`../cards/../decks/...` relative to cwd and fail from the repo root; that is
pre-existing and unrelated).

### Step 1–2 — #6 and #7, RED

Tests written first (`tests/cards/test_flow.cpp`), against the stubs:

```
[ RUN      ] FlowTest.OfferedFromTrashWhenFlowCostAffordable
test_flow.cpp:211: Failure
Expected equality of these values:
  offers.size()
    Which is: 0
  1u
[  FAILED  ] FlowTest.OfferedFromTrashWhenFlowCostAffordable (4 ms)
[ RUN      ] FlowTest.NotOfferedWhenOnlyThePrintedCostIsAffordable
[       OK ] FlowTest.NotOfferedWhenOnlyThePrintedCostIsAffordable (2 ms)
[ RUN      ] FlowTest.ClosedStateOffersOnlyTheReactionFlowSpell
test_flow.cpp:269: Failure
Expected equality of these values:
  reaction_offers.size()
    Which is: 0
  1u
[  FAILED  ] FlowTest.ClosedStateOffersOnlyTheReactionFlowSpell (2 ms)
[  PASSED  ] 1 test.   [  FAILED  ] 2 tests
```

(The "not offered when only the printed cost is affordable" half of #6 is
vacuously green against an empty generator — its RED is proven by the
first half; it is the negative control for the affordability gate.)

### Step 3–4 — implement the offer; GREEN

```
[==========] Running 3 tests from 1 test suite.
[       OK ] FlowTest.OfferedFromTrashWhenFlowCostAffordable (3 ms)
[       OK ] FlowTest.NotOfferedWhenOnlyThePrintedCostIsAffordable (2 ms)
[       OK ] FlowTest.ClosedStateOffersOnlyTheReactionFlowSpell (2 ms)
[  PASSED  ] 3 tests.
```

Full suite: `1072 tests from 118 test suites ran … [ PASSED ] 1072 tests.`
(1069 pre-existing + 3, 1 disabled.)

### Step 5–6 — #8, RED

```
[ RUN      ] FlowTest.FlowPlayPaysOnlyTheFlowCostAndBanishesOnLeave
test_flow.cpp:331: Failure  countExhausted(s, P1) Which is: 1   vs 2
test_flow.cpp:335: Failure  countReady(s, P1)     Which is: 3   vs 1
test_flow.cpp:338: Failure  rune_deck.size()      Which is: 0   vs 1u
test_flow.cpp:346: Failure  played[0].energy_spent Which is: 1  vs 2
test_flow.cpp:349: Failure  max_spell_spent_this_turn Which is: 1 vs 2
test_flow.cpp:361: Failure  chain_energy          Which is: 1   vs 2
test_flow.cpp:362: Failure  banish_on_leave       Actual: false vs true
[  FAILED  ] FlowTest.FlowPlayPaysOnlyTheFlowCostAndBanishesOnLeave (2 ms)
```

The pre-implementation path charged the **printed** 1E (one rune exhausted,
nothing recycled) and set no banish flag — exactly the failure the brief
predicted.

### Step 7–8 — implement the payment; GREEN

```
[==========] Running 4 tests from 1 test suite.
[       OK ] FlowTest.OfferedFromTrashWhenFlowCostAffordable (3 ms)
[       OK ] FlowTest.NotOfferedWhenOnlyThePrintedCostIsAffordable (2 ms)
[       OK ] FlowTest.ClosedStateOffersOnlyTheReactionFlowSpell (2 ms)
[       OK ] FlowTest.FlowPlayPaysOnlyTheFlowCostAndBanishesOnLeave (2 ms)
[  PASSED  ] 4 tests.
```

Full suite: `1073 tests … [ PASSED ] 1073 tests.`

### Step 9–10 — #12 and #29

Both passed on first run, as the brief anticipated (the two generators are
independent, and `liveFlowCosts` already returns both offers). RED was
therefore proven by temporary probes, each reverted immediately after:

**#12** — early-`return` inserted at the top of `generateFlowPlayActions`:

```
test_flow.cpp:392: Failure
Expected equality of these values:
  offers.size()
    Which is: 1
  2u
The two permissions are independent costs, so the spell must be offered
twice: once for the Death-from-Below grant and once for its own printed
[Flow] cost. Neither generator may swallow the other's offer.
[  FAILED  ] FlowTest.TrashReplayGrantAndPrintedFlowYieldTwoDistinctIntents
```

**#29** — `if (offers.size() > 1) offers.resize(1);` added at the end of
`liveFlowCosts` (i.e. only one live cost returned):

```
test_flow.cpp:437: Failure
Expected equality of these values:
  offers.size()
    Which is: 1
  2u
CR 829.1.c.3 / addendum #1 — both live Flow costs are offered and the
controller chooses; one intent per cost.
[  FAILED  ] FlowTest.GrantedAndPrintedFlowYieldTwoIntentsEachPayingItsOwnCost
```

Probes reverted (`grep -c "TEMPORARY RED PROBE" src/engine/game_engine.cpp`
→ `0`), rebuilt, GREEN:

```
[==========] Running 2 tests from 1 test suite.
[       OK ] FlowTest.TrashReplayGrantAndPrintedFlowYieldTwoDistinctIntents (3 ms)
[       OK ] FlowTest.GrantedAndPrintedFlowYieldTwoIntentsEachPayingItsOwnCost (2 ms)
[  PASSED  ] 2 tests.
```

### Final full suite (pre-commit)

```
cd build && ./riftbound_tests
[==========] 1075 tests from 118 test suites ran. (1894 ms total)
[  PASSED  ] 1075 tests.
  YOU HAVE 1 DISABLED TEST
```

1069 pre-existing + 6 new, 1 pre-existing disabled test. No regressions.

## Tests written

`tests/cards/test_flow.cpp` — two test-local `SpellCard` subclasses
registered at ids 901/902 (printed 1E, `Keyword::Flow`, `flow_energy = 2`,
`flow_power = 1`, `flow_any_domain = true`, Fury domain; one `Action`, one
`Reaction`). The fixture re-runs `card_db.buildFromClasses(card_registry)`
after registering them, because `executePlaySpell` reads `card_db_`
(`ability_text` for `[Repeat]`, printed `energy_cost`) and `CardDB::get`
throws on an unknown id.

| # | Test | Asserts |
|---|------|---------|
| 6a | `OfferedFromTrashWhenFlowCostAffordable` | 4 ready Fury runes → exactly one intent, `PlayCard`, `play_source == Trash`, `flow_source == Printed` |
| 6b | `NotOfferedWhenOnlyThePrintedCostIsAffordable` | 1 ready rune (affords printed 1E, not `[E2][P1]`) → no intent at all |
| 7 | `ClosedStateOffersOnlyTheReactionFlowSpell` | `oc_state = Closed`, `priority_holder = P1`, 8 runes → the `[Action]`-only spell is not offered; the `[Reaction]` one is, as `PlayReaction` |
| 8 | `FlowPlayPaysOnlyTheFlowCostAndBanishesOnLeave` | real rune deltas (2 exhausted, 1 ready, 1 recycled into `rune_deck`) — a third exhausted rune would mean the printed cost was charged too; `CardPlayedEvent::energy_spent == 2`; `max_spell_spent_this_turn == 2`; sampled at `ChainItemFinalizedEvent`: the card is off the trash list, its zone is no longer `Trash`, and the chain item carries `banish_on_leave == true` and `total_energy_spent == 2` |
| 12 | `TrashReplayGrantAndPrintedFlowYieldTwoDistinctIntents` | a `PlayerState::TrashReplayGrant` (Death from Below's shape, pushed on the state) + printed Flow → exactly 2 intents: one `flow_source == None`, one `flow_source == Printed` |
| 29 | `GrantedAndPrintedFlowYieldTwoIntentsEachPayingItsOwnCost` | live `GrantedFlow{energy=1}` + printed `[E2][P1]` → 2 intents, one per source; a grant stamped with `turn_number - 1` collapses the list back to 1 (evaluated expiry); executing the **granted** intent exhausts exactly 1 rune, recycles nothing, reports `max_spell_spent_this_turn == 1`, and clears `granted_flow` |

The `banish_on_leave` / chain facts are sampled from an
`on_chain_item_finalized` handler, because `stepResolve` pops the item
before resolving and (Task 5 pending) the spell lands back in the trash
afterwards — those facts are only observable while the item is live.

## Self-review

- **Three call sites** — main, showdown, closed; each immediately after
  `generateTrashReplayActions`, with the same `action_ok`/`reaction_ok`. ✔
- **Both offers** — printed and granted, one intent per affordable cost
  (test #29). ✔
- **Payment uses the recomputed offer** — `liveFlowCosts(intent.card)`
  matched on `intent.flow_source`; nothing is read from the intent but the
  source tag. ✔
- **Riders skipped** — `payTrashReplayGrant`, `payCardCost` and
  `maybePayOptionalAdditionalCost` are all behind `!paid_via_flow`. ✔
- **`energy_spent` correct** — `CardPlayedEvent`,
  `ps.last_spell_energy_spent`, `ps.max_spell_spent_this_turn` and the chain
  item's `total_energy_spent` all read the flow energy (tests #8, #29). ✔
- **Banish flag set, disposal NOT implemented** — only
  `ChainItem::banish_on_leave = true`; no change to `ChainManager` or
  `card_helpers.h`. ✔
- **Grant cleared** — `card.granted_flow.reset()` after payment, asserted in
  #29. ✔
- **Names exact** — `liveFlowCosts`, `generateFlowPlayActions`, `FlowOffer`,
  `Intent::flow_source`, `Intent::FlowSource::{None,Printed,Granted}`,
  `ChainItem::banish_on_leave`; log line is
  `FLOW: <name> played from trash for [E<n>][P<n>] (printed|granted)`. ✔
- **`generateTrashReplayActions` untouched** (test #12 is the witness). ✔
- **Files** — `src/engine/game_engine.cpp` + `tests/cards/test_flow.cpp`
  only; no header edited, so no 35-minute rebuild. ✔
- **Tests assert real state** — rune object exhaustion/recycling counts,
  emitted intents, event payloads and chain-item flags; no assertions on
  internal helper return values. ✔

## Concerns / notes for later tasks

1. **Defensive fallback on a vanished offer.** If `flow_source != None` but
   no matching live offer comes back (a cost that expired between offer and
   execution — not reachable through `generateLegalActions` today), the code
   falls through to the ordinary grant/printed payment rather than aborting
   the play. That is the conservative choice (the card has already left the
   trash), but it means such a play would silently pay the printed cost.
   Worth a rule if a card is ever printed that can revoke a Flow grant
   mid-chain.
2. **`granted_flow` is cleared on any flow play**, printed or granted. This
   matches the brief and the spec's reasoning (the object leaves the trash
   either way), but it is slightly broader than "the granted cost was used".
   A card that returns the spell to trash the same turn would not get the
   grant back — believed correct per CR 829/the spec's "consumed on play"
   note, but flagging it as a deliberate choice rather than an accident.
3. **`Repeat` is left live on a flow play** — spec addendum #6 says the
   additional-cost rider path stays untouched and unexercised, and no
   in-scope Flow spell carries `[Repeat]`, so `parseRepeatCost` still runs
   against the card's ability text after a flow payment. If that ever
   combines, `total_energy_spent` = flow energy + repeat tranches, which
   looks right, but it is untested.
4. **Two shipped tests are cwd-sensitive** (`BaronNashor_Integration…`,
   `FullGameRunsWithSpellsInDecks`): they resolve
   `../cards/../decks/miss_fortune_test.txt` and fail when the binary is run
   from the repo root instead of `build/`. Pre-existing, unrelated to this
   task, but it will bite anyone running the suite the other way.
5. **Task 5 must now honour the flag** at both disposal sites
   (`ChainManager::stepResolve` and `counterChainTop` /
   `revertCounteredPlay` in `card_helpers.h`); until then a flow spell still
   resolves into the trash, which is exactly what test #8 works around by
   sampling at finalize time.

---

# Fix round 1 — execution validates its intent and fails loudly

Commit on top of `aa9f080`. Same two files, no headers.

## What changed

### Finding 1 (Important) — the execution branch failed OPEN
A new **validation block at the top of `executePlaySpell`**, before the
`event_play_source` derivation and before the source-zone removal — i.e.
before anything mutates. When `intent.flow_source != None` it requires, in
order:

1. `intent.play_source == Intent::PlaySource::Trash`
   → reject `"play_source is not Trash"`;
2. `card.zone == ZoneType::Trash` **and** the object is actually in
   `ps.trash` → reject `"the card is not in its controller's trash"`;
3. a live offer from `liveFlowCosts(intent.card)` whose `source` matches the
   intent → reject `"no live granted flow cost"` / `"no live printed flow
   cost"`;
4. `canPayAdditionalCost` for the resolved cost — looping domains when
   `any_domain` and **keeping the domain it found** → reject
   `"cannot pay [E<n>][P<n>]"`.

Each rejection logs `FLOW: illegal flow intent for <name> — <reason>` and
`return`s, executing nothing. The log level is **`logWarn`, not `logTrace`**:
`game_runner.cpp:60` / `main.cpp:666` suppress Trace unless a run passes
`--trace`, so a Trace line would have been invisible in an ordinary run and
would not have been "loud" in any useful sense. Warning is always printed
(`[WRN]`).

The resolved `flow_cost` / `flow_domain` are then carried down to the payment
site, so payment spends exactly what validation approved — no second,
possibly-different resolution.

Sub-findings, all folded:

- **(a) silent fallthrough** — gone; a non-matching `flow_source` now returns
  before the trash erase instead of playing the spell for its printed cost.
- **(b) `granted_flow.reset()` outside the loop** — the reset now lives
  inside the success branch and fires **only for
  `Intent::FlowSource::Granted`**. A rejected play resets nothing (it returns
  long before), and a *printed*-cost play no longer destroys a live grant.
  This also folds Minor #3 of the review's numbering (printed play must not
  consume the grant). Because the rejection returns early, `payTrashReplayGrant`
  can no longer be reached by a failed flow play and cannot eat an unrelated
  Death-from-Below grant.
- **(c) discarded `payAdditionalCost` bool** — `paid_via_flow` is now
  literally that bool. For a fixed-domain offer affordability is checked in
  validation (it previously never was); for `[A]` with no payable domain the
  play is rejected instead of paid anyway.

  **What I chose for the `false`-after-validation case:** it is unreachable —
  `canPayAdditionalCost` was checked for the *same* `(energy, power, domain)`
  triple, and nothing between validation and payment touches runes. It is
  handled as an engine invariant violation the way the engine's one existing
  "can't happen" cost guard is (`assert` — `game_engine.cpp:5448`
  `beginCostPayment`), plus an always-visible `logWarn`
  (`FLOW: payment failed after validation for <name> — engine invariant
  violated`). `paid_via_flow` stays `false`, so a release build charges the
  spell through the ordinary path rather than handing it out free. I did not
  add a rollback: `payAdditionalCost` can mutate before returning `false`,
  there is no rollback machinery in `executePlaySpell`, and inventing one for
  an unreachable branch is exactly the unrequested abstraction the house rules
  forbid.

- **`liveFlowCosts` zone check** — added `if (o.zone != ZoneType::Trash)
  return offers;` with a comment tying it to CR 829.1.b. A Flow cost is a
  permission to play *from the trash* and says nothing anywhere else, so an
  object in hand now has no live flow cost at all. This is a second,
  independent barrier behind the `play_source` check.

### Finding 2 (Minor) — third copy of the timing gate
Extracted a file-local static helper in an anonymous namespace above
`generateFlowPlayActions`:

```
bool trashPlayTimingAllows(const TurnState& turn, const GameObject& card,
                           bool action_ok, bool reaction_ok)
```

Both `generateFlowPlayActions` and `generateTrashReplayActions` now call it;
the two blocks were byte-identical. `generateSpellActions` is untouched — its
shape differs (`allowed = has_reaction; if (!reaction_ok) allowed = false;`)
and the review said to leave it alone. No header change; no behaviour change
(the whole suite is the witness).

### Finding 3 (Minor) — chain-item lookup
`total_energy_spent`, `repeats_paid` and `banish_on_leave` are now set in a
**single by-id loop** over `state_.chain.items` keyed on the `chain_id`
`addSpell` returned. `addSpell` appends, so this is exactly the item the old
`items.back()` reached — behaviour is unchanged, the `back()`/by-id
inconsistency the new code introduced is gone, and the flow flag no longer
needs its own second scan.

### Finding 4 (Minor, coverage) — three added tests
See the table below: `ShowdownCallSiteOffersFlowPlays`,
`CantPlaySpellsThisTurnSuppressesFlowOffers`,
`PrintedFlowPlayLeavesALiveGrantInPlace`.

## Tests added (5) — TDD

| Test | Covers | Asserts |
|------|--------|---------|
| `HandTaggedFlowIntentIsRejectedAndMutatesNothing` | #1, the hand-forgery hole | a hand card with a hand-built `{play_source=Hand, flow_source=Printed}` intent, and the same intent relabelled `play_source=Trash`, are both rejected: no `CardPlayedEvent`, 0 runes exhausted, 0 recycled, card still in hand, chain empty, `cards_played_this_turn == 0`, and **two** `[WRN] FLOW: illegal flow intent` lines |
| `StaleFlowSourceIsRejectedAndLeavesGrantsIntact` | #1(a)(b) | trash spell whose `granted_flow.valid_on_turn` is last turn, intent tagged `Granted`: rejected, no payment, card still in trash, chain empty, `granted_flow` **still present**, the unrelated `TrashReplayGrant` **still present**, and the single warning names the reason (`no live granted flow cost`) |
| `PrintedFlowPlayLeavesALiveGrantInPlace` | #1(b) / Minor #3 | with a live grant AND printed Flow, executing the **printed** offer exhausts 2 + recycles 1 (the printed cost, not the cheaper grant) and leaves `granted_flow` intact |
| `ShowdownCallSiteOffersFlowPlays` | #4 | showdown-open with `focus_holder = P1` → one offer, `PlayActionCard`, `flow_source == Printed`, `play_source == Trash` |
| `CantPlaySpellsThisTurnSuppressesFlowOffers` | #4 | offered while unsuppressed; `cant_play_spells_this_turn = true` → no offer |

### RED

The three finding-1 tests were RED against the round-1 code — and the failure
output is a direct demonstration of the hole:

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='FlowTest.HandTagged*'
[ RUN      ] FlowTest.HandTaggedFlowIntentIsRejectedAndMutatesNothing
test_flow.cpp:523: Failure  played.empty()          Actual: false  Expected: true
test_flow.cpp:526: Failure  countExhausted(s, P1)   Which is: 2    vs 0
test_flow.cpp:529: Failure  rune_deck.size()        Which is: 1    vs 0u
test_flow.cpp:531: Failure  hand.size()             Which is: 0    vs 1u
test_flow.cpp:535: Failure  cards_played_this_turn  Which is: 1    vs 0
```

i.e. the forged intent **did** play the card out of hand and **did** pay the
flow cost (2 exhausted + 1 recycled) from hand.

```
[ RUN      ] FlowTest.StaleFlowSourceIsRejectedAndLeavesGrantsIntact
test_flow.cpp:...: Failure  played.empty()                    Actual: false
test_flow.cpp:592: Failure  countExhausted(s, P1)             Which is: 1  vs 0
test_flow.cpp:597: Failure  granted_flow.has_value()          Actual: false  Expected: true
test_flow.cpp:601: Failure  trash_replay_grants.size()        Which is: 0  vs 1u
```

— the rejected play destroyed the grant record AND ate the unrelated
Death-from-Below grant, exactly as finding 1(b) predicted.

```
[ RUN      ] FlowTest.PrintedFlowPlayLeavesALiveGrantInPlace
test_flow.cpp:634: Failure  granted_flow.has_value()  Actual: false  Expected: true
```

The two coverage tests (#4) pass against the round-1 code by construction, so
their RED was proven with temporary probes, each reverted immediately:

- **showdown** — deleted the `generateFlowPlayActions` call from
  `generateShowdownActions`:
  `offers.size() Which is: 0 vs 1u — generateFlowPlayActions must be wired
  into the SHOWDOWN action generator too…` → FAILED.
- **lockout** — narrowed the guard to `if (ps.cant_play_cards_this_turn) return;`:
  `Actual: false Expected: true — A spell lockout suppresses flow plays…`
  → FAILED.

`grep -c "TEMPORARY RED PROBE" src/engine/game_engine.cpp` → `0` after both.

### GREEN

```
$ cmake --build build --target riftbound_tests -j4
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='FlowTest.*'
[==========] Running 11 tests from 1 test suite.
[       OK ] FlowTest.OfferedFromTrashWhenFlowCostAffordable (3 ms)
[       OK ] FlowTest.NotOfferedWhenOnlyThePrintedCostIsAffordable (2 ms)
[       OK ] FlowTest.ClosedStateOffersOnlyTheReactionFlowSpell (2 ms)
[       OK ] FlowTest.FlowPlayPaysOnlyTheFlowCostAndBanishesOnLeave (2 ms)
[       OK ] FlowTest.TrashReplayGrantAndPrintedFlowYieldTwoDistinctIntents (2 ms)
[       OK ] FlowTest.GrantedAndPrintedFlowYieldTwoIntentsEachPayingItsOwnCost (2 ms)
[       OK ] FlowTest.HandTaggedFlowIntentIsRejectedAndMutatesNothing (2 ms)
[       OK ] FlowTest.StaleFlowSourceIsRejectedAndLeavesGrantsIntact (2 ms)
[       OK ] FlowTest.PrintedFlowPlayLeavesALiveGrantInPlace (2 ms)
[       OK ] FlowTest.ShowdownCallSiteOffersFlowPlays (2 ms)
[       OK ] FlowTest.CantPlaySpellsThisTurnSuppressesFlowOffers (2 ms)
[  PASSED  ] 11 tests.
```

### Full suite

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1080 tests from 118 test suites ran. (1870 ms total)
[  PASSED  ] 1080 tests.
  YOU HAVE 1 DISABLED TEST
```

1069 pre-existing + 11, 1 pre-existing disabled. (`RIFTBOUND_ROOT=.` from the
repo root fixes the two cwd-sensitive deck-loading tests noted in concern 4
of the round-1 report — that concern is now answered, not outstanding.)

## Concerns after this round

1. The `payAdditionalCost`-returns-false branch is dead code by construction
   (see the choice above). It is asserted, logged, and untested — I did not
   write a test for it because reaching it requires the invariant to already
   be broken.
2. Round-1 concerns 1 and 2 are **resolved** by this change (fail-loud
   validation; grant cleared only on a granted play). Concern 3 (`[Repeat]`
   left live on a flow play, per addendum #6) and concern 5 (Task 5 must
   honour `banish_on_leave` at both disposal sites) still stand. Concern 4 is
   answered by `RIFTBOUND_ROOT=.`.
3. Parked by the controller, unchanged: the OpenSpiel action-vocab collision
   between printed / granted / replay intents for one card def.
