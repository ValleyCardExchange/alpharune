# Task 8 report — four VEN cards + registration + deck file

Branch `kennen-tyler-deck`, base HEAD `5e10eb0`. Five commits:
`0785168` (789 Kennen), `68368b3` (788 Heart of the Tempest),
`2bd8a6a` (790 Lightning Rush), `9ef7829` (791 Up from the Deep),
`2049853` (deck file).

Per the controller's ruling relayed in the task: this covers ONLY the
four cards 788/789/790/791. Sandswept Tomb (792) and deck-validator
test #26 are Task 9's — the deck file still names Sandswept Tomb, which
this task does not register, so the deck does not yet fully validate.

## Card data fidelity

All def fields (id, def_id, public_code, collector_number, set_code/
set_name, artist, card_type/super_type, domains, tags, costs, might,
rarity, keywords, flow_energy/flow_power/flow_any_domain, image_url,
ability_text) were copied verbatim from
`.superpowers/sdd/2026-09-07-kennen-tyler-deck/task-8-carddata.md`,
which itself quotes `docs/knowledge/riftbound/reference/cards/VEN.json`
in personal-ai (I did not re-derive text from that JSON myself — I used
the carddata brief's verbatim transcription, per the task's explicit
instruction to use it verbatim). Diffed every `d.` field against the
carddata file line-by-line before committing each card; no field
deviates. Glyph convention (`[M]`, `[E]`, `[N]`, `[A]`) matches
`0173_ride_the_wind.cpp` and other existing OGN/UNL defs, and was
already baked into the carddata brief's quoted text, so no glyph
translation was needed on my end.

## 789 — Kennen, Storm of Shuriken (VEN-113/166)

**File:** `src/cards/units/0789_kennen_storm_of_shuriken.cpp`.

Champion unit. `triggerTypes() = {WhenYouPlayMe, WhenIConquer}`,
branching on `ctx.firing_trigger` (Blitzcrank/Darius multi-trigger
pattern).

- `WhenYouPlayMe` → `ctx.executor.burnCards(ctx.controller, 2)`.
- `WhenIConquer` → mandatory (no "may"): collect every `CardType::Spell`
  in the controller's trash; if none, do nothing; otherwise
  `pickTarget(ctx, label, spells)` (resumable — same suspend check as
  `TheHarrowing`'s `resume_point == 7` pattern) and set the chosen
  spell's `GameObject::granted_flow = {energy = def.energy_cost, power =
  def.power_cost, power_domain = def.domains.front() (Fury if the spell
  prints no domain), any_domain = false, valid_on_turn =
  state.turn.turn_number}`. Logs `KENNEN: <spell> gains Flow
  [E<n>][P<n>] this turn`.

### TDD

RED (before the card file/registration existed):
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.Kennen_*'
```
→ 3 tests, all failed with `C++ exception with description "Card ID not
found: 789" thrown in the test body` (from `addUnit`'s `card_db.get`
lookup) — the expected "card not registered" RED.

Card + registration written; GREEN:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.Kennen_*'
[==========] Running 3 tests from 1 test suite.
[  PASSED  ] 3 tests.
```
Full suite: 1095/1095 (1092 baseline + 3 new), 1 pre-existing disabled
test.

Tests (`tests/cards/test_kennen_cards.cpp`):
- `Kennen_PlayTrigger_WiresBurnTwo` — one small card-level case wiring
  `WhenYouPlayMe` to `burnCards` (the helper itself is #13/#14, already
  covered by Task 6).
- `Kennen_Conquer_TwoSpellsInTrash_AgentChoosesSecond_GrantsItsFlow`
  (test #15) — two real spells (The Harrowing id 198, Ride the Wind id
  173) in trash; a picker forces the agent to choose the second
  (Ride the Wind); asserts `granted_flow` matches its printed
  energy/power/domain, `any_domain=false`, `valid_on_turn` stamped to
  the current turn, and the NON-chosen spell is untouched.
- `Kennen_Conquer_NoSpellInTrash_ChangesNothing` (test #16) — a unit
  (not a spell) in trash; fires the trigger directly (`fireTriggerAs`,
  no chain resumption needed since the empty-spell-list branch returns
  before calling `pickTarget`); asserts nothing changed.

## 788 — Heart of the Tempest (VEN-155/166)

**File:** `src/cards/legends/0788_heart_of_the_tempest.cpp`.

Legend. `triggerType() = WhenYouPlayFromNonHand` →
`ctx.executor.empowerObject(ctx.source)`. One `ActivatedAbility`:
`cost = {exhaust=true, disempower_self=true}`, `targets = {count=1,
must_be_unit=true}` (any unit, not just friendly — the printed text
says "a unit"), `is_action=true`, `needs_activation_time_target=true`.
`onActivate` picks via `pickTarget` (falls back to `targets[0]` if the
engine ever supplies one) and calls
`ctx.executor.giveTemporaryKeyword(picked, Keyword::Assault, 2)`.
`getTargetRequirements()` mirrors the ability's targets so the default
(single-arg) `enumerateLegalTargets` — used by the multi-ability
`enumerateLegalTargets(state, controller, ability_index)` default
fallthrough — enumerates correctly (Blind Monk / Malzahar precedent).

### TDD

RED:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.HeartOfTheTempest*'
```
→ failed twice: `is_empowered` stayed false, and the action intent was
never found (card not registered — `HeartOfTheTempest_
EmpowersOnTrashPlay_ActionGivesAssaultThatExpires`).

First attempt after authoring the card still failed with the SAME two
assertions — not a registration problem this time, but a test-harness
bug I found while driving it: I had called
`EffectExecutor::playIgnoringCost` directly on a bare `EffectExecutor`
with no enclosing `executeIntent`/`runChain()`. `TriggerManager::
onCardPlayed` correctly queues the resulting `WhenYouPlayFromNonHand`
ability onto the chain (`chain_.addAbility(...)`), but nothing ever
resolved it — `playIgnoringCost` itself never calls `runChain()`,
and I hadn't either. Fixed by rebuilding the test to play a REAL card
(The Harrowing, id 198) from hand through the full engine
(`engine.generateLegalActions()` + `engine.testHook_executeIntent`),
which drives `executePlaySpell` → The Harrowing's `onResolve` (trash
replay via `playIgnoringCost`) → `runChain()` at the end of
`executePlaySpell`, which drains the queued legend trigger too — same
shape as `test_play_from_non_hand.cpp`'s test #4. GREEN after that fix:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.HeartOfTheTempest*'
[==========] Running 1 test from 1 test suite.
[       OK ] KennenCardsTest.HeartOfTheTempest_EmpowersOnTrashPlay_ActionGivesAssaultThatExpires (4 ms)
```
Full suite: 1096/1096 (+1), 1 pre-existing disabled test.

Test #17 (`HeartOfTheTempest_EmpowersOnTrashPlay_ActionGivesAssaultThatExpires`)
drives the whole chain: real GameEngine, The Harrowing replays a
trashed unit → legend becomes empowered → `generateLegalActions()`
offers the legend's action (found by `ability_source == legend_id`) →
`testHook_executeIntent` runs it → asserts the legend is disempowered
+ exhausted (cost paid) and the single legal unit target now has
`Keyword::Assault` with `assault_value`/`temp_assault_value == 2` →
finally calls the engine's own `GameEngine::expireTemporaryKeywords`
(the pure per-object helper the real Expiration Step runs, exposed
publicly per its doc comment specifically so card tests can verify
this without spinning up a full turn loop) and asserts the `Assault`
keyword bit clears. I did not additionally drive a full `endingStep`/
`expirationStep` — that helper is private and no test hook exposes it,
and the doc comment on `expireTemporaryKeywords` says it exists for
exactly this purpose.

## 790 — Lightning Rush (VEN-156/166)

**File:** `src/cards/spells/0790_lightning_rush.cpp`.

Signature spell. `Keyword::Flow` set, `flow_energy=2, flow_power=1,
flow_any_domain=true`. `onResolve` is a single line:
`ctx.executor.revealAndChoose(ctx.controller, 3,
EffectExecutor::RestDestination::Trash)`. That helper (built in Task 7,
already unit-tested there) already implements: public reveal (CR 424),
one draw-or-skip choice per revealed card (the "none" option IS just
skipping every card — no separate decline was needed, confirming the
brief's speculative concern that a decline path might be missing), the
chosen card lands in hand as a real draw (`draws_this_turn`
incremented, `CardsDrawnEvent` emitted), and non-chosen cards go to
trash in revealed order.

### TDD

RED: `Expected: (card) != (nullptr), actual: NULL vs (nullptr)` for all
three tests (card not registered). GREEN after authoring + registering:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.LightningRush*'
[==========] Running 3 tests from 1 test suite.
[  PASSED  ] 3 tests.
```
Full suite: 1099/1099 (+3), 1 pre-existing disabled test.

Tests:
- `LightningRush_DrawsSecondOfThree_OtherTwoToTrashInOrder` (#18) —
  3-card deck [A,B,C top]; agent draws B (the 2nd revealed, [C,B,A]
  order); asserts B in hand, `handSize==1`, trash == [C,A] in that
  order.
- `LightningRush_AgentPicksNone_AllThreeToTrash` (#19) — agent skips
  every reveal; all three end in trash, hand untouched.
- `LightningRush_TwoCardDeck_RevealsTwo` (#20) — 2-card deck; both
  revealed and (agent skipping) both trashed; deck ends empty.

## 791 — Up from the Deep (VEN-100/166)

**File:** `src/cards/spells/0791_up_from_the_deep.cpp`.

Spell. `Keyword::Flow` set, `flow_energy=3` (no power). `onResolve`
calls `ctx.executor.createToken(...)` twice: `CardType::Unit,
"Tentacle", might=1, tags={"Tentacle","Bilgewater"}, KeywordSet{},
LocationId{BaseLocation{ctx.controller}}, enter_ready=false` (engine
convention: units enter exhausted unless readied — matches
`0270_altar_to_unity.cpp`).

### TDD

RED: `Expected: (card) != (nullptr) ...` for both tests. GREEN after
authoring + registering:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.UpFromTheDeep*'
[==========] Running 2 tests from 1 test suite.
[  PASSED  ] 2 tests.
```
Full suite: 1101/1101 (+2), 1 pre-existing disabled test.

Tests:
- `UpFromTheDeep_CreatesTwoExhaustedOneMightTentacles` (#21) — finds
  both created objects by name; asserts `CardType::Unit`,
  `base_might`/`current_might == 1`, `is_exhausted`, both tags present,
  location is `BaseLocation{P1}`, `zone == Base`.
- `UpFromTheDeep_TokensDoNotEmpowerHeartOfTheTempest` (#28, addendum
  #7) — a real Heart of the Tempest legend on board; resolving Up from
  the Deep must NOT empower it, since `createToken` emits no
  `CardPlayedEvent` (tokens are not cards, CR 185/350.2). Asserts
  `is_empowered` stays false.

## Registration

`src/cards/cards_init.cpp`: added `register_card_788` through
`register_card_791` declarations (after 787) and calls (after 787),
in numeric order in the final state. Re-ran `cmake -S . -B build -G
Ninja` once after the first new file appeared so CMake's
`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` picked up the new sources;
subsequent adds needed the same reconfigure (each new .cpp file is a
new glob match).

## Deck file

`decks/kennen_tyler.txt`: legend line `1 Kennen, Heart of the Tempest`
→ `1 Heart of the Tempest` (legends register under title only —
`CardDB::findByName` / the Blind Monk convention cited in the spec).
Rest of the file unchanged. No test currently loads this deck file
(`tests/test_deck_validator.cpp` has no reference to `kennen_tyler`),
so this change has no effect on the passing suite; it also does not
yet make the deck fully valid, since Sandswept Tomb (792, Task 9) is
still unregistered.

## Full suite (final state, all 4 cards + deck file)

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1101 tests from 119 test suites ran. (1881 ms total)
[  PASSED  ] 1101 tests.
  YOU HAVE 1 DISABLED TEST
```
1092 baseline + 9 new Kennen tests (1 play-trigger wiring case + #15,
#16, #17, #18, #19, #20, #21, #28) = 1101. The 1 disabled test is the
same pre-existing one noted in every prior task report on this branch.

## Coverage gate

```
$ python3 scripts/card_coverage.py
bucket           engine-gap  partial  clean  total
implemented              50       44    621    715
engine-handled            0        0      7      7
metadata-only             0        0      0      0
stub                      0        0      0      0
vanilla                   0        0     69     69
TOTAL                                          791

INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```
Total is 791 (787 + the 4 new cards — Sandswept Tomb is Task 9's, so
the total is not yet 792). Verified programmatically (not just by the
absence from the printed table) that none of the four new files appear
in the `incomplete.json` or `engine-gap.json` lists `--write-lists`
produces — all four classify `implemented`/`clean`. The pre-existing
50 engine-gap / 44 partial counts are unrelated cards, untouched by
this task.

## Files touched

- `src/cards/units/0789_kennen_storm_of_shuriken.cpp` (new)
- `src/cards/legends/0788_heart_of_the_tempest.cpp` (new)
- `src/cards/spells/0790_lightning_rush.cpp` (new)
- `src/cards/spells/0791_up_from_the_deep.cpp` (new)
- `src/cards/cards_init.cpp` (registration)
- `tests/cards/test_kennen_cards.cpp` (9 new tests appended)
- `decks/kennen_tyler.txt` (legend line)

No headers touched, no engine edits — every engine piece named in the
task brief (`empowerObject`, `WhenYouPlayFromNonHand`,
`disempower_self`, `giveTemporaryKeyword`, `burnCards`,
`GameObject::granted_flow`, `Keyword::Flow` + `flow_*`,
`revealAndChoose` with `RestDestination::Trash`, `createToken`) already
existed from Tasks 1-7 exactly as documented, with no gaps found.

## Self-review

- Every `d.` field on all four cards diffed against
  `task-8-carddata.md` verbatim (ids, def_ids, public codes,
  collector numbers, set_code/set_name, artist, card_type/super_type,
  domains, tags, costs, might, rarity, keywords, flow_*, image_url,
  ability_text) — no deviation.
- `grep -inE "approx|todo|fixme|simplified|workaround|placeholder|
  best-effort|for now"` across all four new files: no matches. No
  `// APPROX` anywhere — nothing needed approximating; every clause of
  all four cards' text maps onto an existing, tested engine primitive.
- Tests assert real zones/flags/keywords, not just "no exception":
  `zone`, `location` (variant type + its `.player`), `is_exhausted`,
  `keywords.has(...)`, `assault_value`/`temp_assault_value`,
  `granted_flow`'s full field set, `is_empowered`, trash/hand vector
  contents and order, deck size.
- Coverage gate: total 791, 0 incomplete, all four new cards confirmed
  `implemented`/clean via the written JSON lists (not just eyeballing
  the summary table).

## Concerns

- **Deck does not yet fully validate.** `decks/kennen_tyler.txt` still
  lists `1 Sandswept Tomb` in Battlefields; that card (792) isn't
  registered until Task 9, so loading this deck through
  `DeckValidator` today would still error on the missing battlefield.
  This is expected per the task's explicit scope split and needs no
  action from Task 8 — flagging so Task 9's "the Tomb needs Task 9's
  mechanism to count as implemented" note is understood to also cover
  "the deck needs Task 9 to fully load."
- **Kennen's conquer grant `power_domain` fallback (Fury) is untested
  directly** — both trash-fodder spells used in test #15 print a real
  domain (Chaos), so the `def->domains.empty() ? Domain::Fury : ...`
  branch's fallback value is exercised by the ternary's structure and
  the `GrantedFlow` struct's own default, but no test drives a spell
  with zero printed domains through Kennen's conquer trigger
  specifically. No such spell exists in Tyler's decklist today, so I
  did not fabricate one — noting it here rather than silently leaving
  it uncovered.
- Per the controller's ruling, this report does not cover Sandswept
  Tomb (792) or deck-validator test #26 — those are Task 9's.

---

# Fix round 1

Commit `42b10e1` on top of the five above. Controller ruling: card/test-
level only, no engine/executor edits. Files touched:
`src/cards/spells/0790_lightning_rush.cpp`,
`src/cards/units/0789_kennen_storm_of_shuriken.cpp`,
`tests/cards/test_kennen_cards.cpp`. (Item (c) fit naturally alongside
the other card-specific tests in `test_kennen_cards.cpp`, not
`test_flow.cpp`, which stayed untouched.)

## 1. [Critical] Lightning Rush let the agent draw all three revealed

**Bug:** `revealAndChoose` asks an independent draw/skip choice per
revealed card (`effect_executor.cpp` ~1094-1134) — nothing stops the
agent answering "draw" three times. "You MAY choose A card" (CR-level
"a") is at most one.

**Fix:** Lightning Rush no longer calls `revealAndChoose` at all. Its
`onResolve` implements its own resumable look-and-choose, modelled on
Stacked Deck (`0183_stacked_deck.cpp`): a guarded peek step
(`ri.resume_point < 3`, run exactly once since `Card::pickMode` claims
resume_points 3-5 on its first invocation) pops up to 3 cards off the
deck and stashes their ids in `resume_data[2..]` (index 2 = count,
3+i = ids — indices 0/1 stay reserved for `pickXAmount`/`pickMode`'s
own bookkeeping per their doc comments in `card.h`, so nothing
collides). Then `pickMode(ctx, label, actual+1, labels)` offers
**exactly one** decision: one mode per revealed card plus a trailing
"None" mode (index == `actual`). On resume, the peek is skipped
(`resume_point` is already ≥ 3) and the same `resume_data` is read
back — a single mode selection, not one draw/skip toggle per card, so
"at most one" is now structural: there is no sequence of agent answers
that produces two draws.

RED (before the rewrite, tests written against `driveResumable` +
`pickMode`-shaped pickers first):
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.LightningRush*'
[  FAILED  ] KennenCardsTest.LightningRush_DrawsSecondOfThree_OtherTwoToTrashInOrder
[  FAILED  ] KennenCardsTest.LightningRush_AgentPicksNone_AllThreeToTrash
[  FAILED  ] KennenCardsTest.LightningRush_TwoCardDeck_RevealsTwo
[  PASSED  ] 2 tests. (EmptyDeck_NoOp and the new PrintedFlowCost test,
             which don't exercise onResolve's draw path)
```
(RED reason: `driveResumable` doesn't call `exec.setAgentQuery`, so the
old `revealAndChoose`-based card's `agent_query_` was null and it fell
into "no agent: put back on top" — nothing moved.)

GREEN after the rewrite:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.LightningRush*'
[==========] Running 5 tests from 1 test suite.
[  PASSED  ] 5 tests.
```

New/changed tests:
- `LightningRush_DrawsSecondOfThree_OtherTwoToTrashInOrder` (#18,
  rewritten) — now also asserts `picker_calls == 1`: the fixture's
  `driveResumable` invokes its `picker` callback once per suspend/
  resume cycle, so "exactly one decision" is a direct, executable proof
  of the fix, not just an inference from the final zone state.
- `LightningRush_AgentPicksNone_AllThreeToTrash` (#19, rewritten) —
  picks the last mode (`legal.back()`, index == 3 == "None"); asserts
  all three trashed, `draws_this_turn == 0`.
- `LightningRush_TwoCardDeck_RevealsTwo` (#20, rewritten).
- `LightningRush_EmptyDeck_NoOp` (new) — `EXPECT_NO_THROW`, all zones
  untouched.

## 2. [Important] "Look at" must be a private look, not a public Reveal

**Fix:** each peeked card now emits `CardRevealedEvent{card,
card_def_id, owner, revealed_to_all=false, revealed_to=controller,
ZoneType::MainDeck}` (Stacked Deck's own precedent, matching the Vision
keyword's private-reveal shape in `trigger_manager.cpp`), replacing the
public-reveal comment and the public reveal `revealAndChoose` used to
emit.

Covered by `LightningRush_DrawsSecondOfThree_OtherTwoToTrashInOrder`,
which subscribes `events.on_card_revealed` and asserts, for all 3
emitted events: `revealed_to_all == false`, `revealed_to == P1`,
`source_zone == MainDeck`.

## 3. [Important] Test #28 was vacuous

**Bug:** the original test built a bare `EffectExecutor` with no
`GameEngine`/`TriggerManager`/chain wired to the shared event bus at
all, then called `card->onResolve(ctx, {})` directly. `is_empowered`
would have stayed `false` regardless of what the card did — nothing
was subscribed to catch a `CardPlayedEvent` even if one had fired.

**Fix:** rewritten to drive Up from the Deep's actual PLAY through a
real `GameEngine` (`testHook_initSubsystems()` wires and subscribes a
real `TriggerManager` on the shared event bus, exactly as test #17
does for The Harrowing) — 3 ready Chaos runes, the spell in hand,
`generateLegalActions()` finds the `PlayCard` intent,
`testHook_executeIntent` runs it through `executePlaySpell` →
`onResolve` (creates the two tokens) → `runChain()`. A
`on_card_played` subscription counts total `CardPlayedEvent`s during
the whole play and asserts exactly 1 (the spell's own hand play) — if
token creation wrongly emitted a second, this assertion, not just
`is_empowered`, would catch it.

**RED-reachability proof** (temporary, reverted): added one line to
`0791_up_from_the_deep.cpp`'s `onResolve` —
`ctx.events.emit(CardPlayedEvent{ctx.source, ctx.controller,
CardType::Unit, 0, 0, Intent::PlaySource::Trash});` (non-Hand source,
so it would actually dispatch `WhenYouPlayFromNonHand`) — before the
`createToken` calls:
```
$ ninja -C build riftbound_tests -j4 && \
  RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.UpFromTheDeep_TokensDoNotEmpowerHeartOfTheTempest'
[  FAILED  ] KennenCardsTest.UpFromTheDeep_TokensDoNotEmpowerHeartOfTheTempest
  card_played_events: 2 (expected 1)
  is_empowered: true (expected false)
```
Both assertions failed as expected — the test is no longer vacuous.
Reverted immediately (`git diff` on the file is empty after revert);
GREEN again:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.UpFromTheDeep*'
[==========] Running 2 tests from 1 test suite.
[  PASSED  ] 2 tests.
```

## 4. [Important] Test #17's target pick was order-dependent

**Bug:** after The Harrowing replays `unit_in_trash`, that unit stays
on the board — a second legal target for Heart of the Tempest's "give
a unit Assault 2" action alongside `target_unit`. `FirstChoiceAgent`
picks `legal.front()`, whose identity depended on `state.objects`
(`unordered_map`) iteration order between the two units; the test
asserted on `target_unit` specifically and happened to pass.

**Fix:** after confirming the legend is empowered, the test now calls
`EffectExecutor::killObject(unit_in_trash)` (a real engine call, not a
test-only shortcut — same board-exit path `test_empower.cpp` exercises
for the empowered-clears-on-kill case) before creating `target_unit`,
and asserts `heart_card->enumerateLegalTargets(s, P1)` has size 1 and
equals `target_unit` BEFORE generating/executing the activate intent —
so the determinism claim is checked directly, not just implied by the
final result. Fixed the stale "the single legal unit target" comment
(now literally true).

GREEN:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.HeartOfTheTempest*'
[  PASSED  ] 1 test.
```

## Minors

**(a) `assault_value == 0` after expiration — NOT added, flagged for the reviewer.**
The keyword-bit clear (`GameEngine::expireTemporaryKeywords`) is
already asserted. The NUMERIC decrement (`obj.assault_value -=
obj.temp_assault_value`) lives in `GameEngine::doExpirationBody`
(`game_engine.cpp`), which is `private` with no `testHook_` wrapper —
confirmed by grepping every `testHook_` entry in `game_engine.h`; none
reach `expirationStep`/`doExpirationBody`. Reaching it requires either
a new `testHook_` (an engine edit, which this round's ruling explicitly
disallows) or a full `runGame()` turn loop (deck/mulligan setup far
beyond this unit test's scenario). I did not paper over this with a
hand-rolled `assault_value -= temp_assault_value` line in the test
itself — that would only prove subtraction works, not that the engine
calls it, and would be a misleading green. I found precedent for
leaving this exact class of gap: `test_jhin_deck.cpp`'s
`FrigidTouch_DebuffPersistsAcrossDecisionsThisTurn` documents that its
temp-might debuff "persists until expirationStep" and stops there,
asserting only survival across `recomputeMight()` calls, never that
expirationStep clears it. I documented the gap at the assertion site in
`test_kennen_cards.cpp` with the exact file/line pointers and flagged
it here — this is functionally real (recomputeMight applies
`assault_value` unconditionally whenever `combat_designation ==
Attacker`, regardless of the keyword bit — `core/game_object.h`), it is
exercised by every real game via the full turn loop, just not by this
unit test. Asking: is a minimal `testHook_expirationStep` (mirroring
the dozen existing one-line `testHook_` wrappers) in scope for a future
round despite this round's "no engine edits," or should this stay
disclosed-and-unaddressed at the unit-test level?

**(b) Kennen's conquer filter now excludes def-less trash objects —
FIXED**, with a RED-then-GREEN regression test.

RED (against the pre-fix filter):
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.Kennen_Conquer_DefLessSpellObjectInTrash_ExcludedFromCandidates'
[  FAILED  ] ... granted_flow.has_value(): true, expected false
```
(The old filter accepted any `CardType::Spell` regardless of
`card_def_id`, so a def-less "spell" WAS picked and granted a hollow
`{energy=0, power=0}` `GrantedFlow` — `has_value()` was true, just with
zero fields, which is exactly the bug the reviewer named.)

Fix: the filter now requires `card_def_id != kInvalidId`; the
downstream lookup changed from a nullable `const CardDef*` to a
non-null `const CardDef&` (the filter is now the sole invariant
guaranteeing every candidate has a real def).

GREEN:
```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='KennenCardsTest.Kennen_*'
[==========] Running 4 tests from 1 test suite.
[  PASSED  ] 4 tests.
```

**(c) One test per Flow card asserting printed cost + trash-offer — ADDED.**
`LightningRush_PrintedFlowCost_AndOfferedFromTrash` and
`UpFromTheDeep_PrintedFlowCost_AndOfferedFromTrash`: each asserts
`def.keywords.has(Flow)` + the exact `flow_energy`/`flow_power`/
`flow_any_domain` triple (790: 2/1/true; 791: 3/0/false), then builds a
real `GameEngine`, places the real card in trash with affordable runes,
and asserts `generateLegalActions()` offers it with
`play_source == Trash` and `flow_source == Printed` — the same
assertion shape as `test_flow.cpp`'s `OfferedFromTrashWhenFlowCost
Affordable`, ties these two real cards to Task 4's
`generateFlowPlayActions` mechanism directly (both were previously only
exercised by test-local synthetic Flow spells in `test_flow.cpp`).

## Coverage gate — a false-positive flag caught and fixed

After the Lightning Rush rewrite, `card_coverage.py` initially reported
**51** engine-gap flags (up from the pre-round baseline of 50) because
a comment in the new code said "...so this **cannot use**
EffectExecutor::revealAndChoose..." — the script's `STRONG` regex
matches the bare word "cannot" anywhere in the file, regardless of
context, and flags the whole card `engine-gap` even though nothing is
actually unimplemented. Reworded to "deliberately avoids"; verified the
count returned to the baseline 50:
```
$ python3 scripts/card_coverage.py
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50   # back to baseline
```
Also re-ran a grep for every STRONG/WEAK trigger word
(`cannot|...|todo|fixme|simplified|approximat|workaround|not yet|
placeholder|best-effort|for now`) across all four card files after
every edit in this round — zero matches.

## Full suite (final, this round)

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1105 tests from 119 test suites ran. (1911 ms total)
[  PASSED  ] 1105 tests.
  YOU HAVE 1 DISABLED TEST
```
1101 (prior state) + 4 net new (`Kennen_Conquer_DefLessSpellObjectIn
Trash_ExcludedFromCandidates`, `LightningRush_EmptyDeck_NoOp`,
`LightningRush_PrintedFlowCost_AndOfferedFromTrash`,
`UpFromTheDeep_PrintedFlowCost_AndOfferedFromTrash`) = 1105. Same 1
pre-existing disabled test as every prior report on this branch.

## Files touched this round

- `src/cards/spells/0790_lightning_rush.cpp` (rewritten onResolve)
- `src/cards/units/0789_kennen_storm_of_shuriken.cpp` (filter fix)
- `tests/cards/test_kennen_cards.cpp` (rewrote #17, #18-20, #28; added
  the def-less-spell regression, the empty-deck case, and both
  printed-flow-cost + trash-offer tests)
- `src/cards/legends/0788_heart_of_the_tempest.cpp` and
  `src/cards/spells/0791_up_from_the_deep.cpp`: **unchanged**
  (`git diff` on both is empty) — only their tests changed.

No engine or executor files touched, per this round's explicit ruling.

## Concerns carried forward

- Minor (a) above is genuinely unresolved at the unit-test level and
  needs the reviewer's call (add a `testHook_`, accept the disclosed
  gap, or drive a full `runGame()` turn — all three are real options,
  none of which fit "card/test-level only" as cleanly as the other
  fixes).
- All concerns from the original report (deck doesn't fully validate
  until Task 9's Sandswept Tomb; Kennen's `Domain::Fury` no-domain
  fallback is structurally exercised but not directly tested, since no
  such spell exists in either deck) still stand, unchanged by this
  round.
