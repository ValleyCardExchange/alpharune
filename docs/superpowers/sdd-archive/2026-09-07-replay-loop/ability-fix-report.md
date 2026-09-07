# Closed-state ability activations — fix report

Branch `replay-loop`. Started at `f657e57`; the concurrent agent landed
`ec2a706` mid-session, so this work is committed on top of that.

## 1. What I found — the diagnosis held, but it was only HALF the hole

**Confirmed as briefed.**

- `ChainManager::stepExecuteAndPass` (`src/engine/chain_manager.cpp:196-356`
  pre-fix) handled exactly two intent types: `PassPriority` (`:222`) and
  `PlayReaction` (`:241`). Everything else fell past the `else if` chain onto
  the bare comment at `:352` — *"Other intent types (ActivateReactionAbility
  etc.) are Phase 3+"* — and the `for` loop simply went round again.
- The closed-state generator does offer them: `GameEngine::generateClosedStateActions`
  emits `IntentType::ActivateReactionAbility` per `[Reaction]` activated
  ability at `src/engine/game_engine.cpp:3125-3190` (pre-fix numbering),
  gated on `Card::isReactionAbility()` and on the ability's cost being
  payable — including `if (ab.cost.exhaust && obj.is_exhausted) continue;`.
- Seal of Discord (204) `src/cards/gear/0204_seal_of_discord.cpp` is exactly
  that shape: `getActivationCost() == {.exhaust = true}`, `isReactionAbility()
  == true`, `onActivate` → `addFloatingPower(controller, Chaos, 1)`. Three
  copies in `decks/kennen_tyler.txt`.
- The burst is reproducible and its length is not arbitrary: the RED run of
  test (4) produced **exactly 10** activations in one closed-state window,
  which is `kMaxPriorityPasses` at `src/engine/chain_manager.cpp:208`. Nothing
  was exhausted, no power was added, and the same intent was re-offered every
  iteration. That matches the 10–93-decision frozen-state runs in the logs (a
  long run is several consecutive windows).

**Correction #1 — the brief's "second executor" framing was right, but adding
the third callback alone does NOT fix the bug.** `GameEngine::executeIntent`'s
activation case listed only

```
case IntentType::ActivateAbility:
case IntentType::ActivateActionAbility: {
```

`ActivateReactionAbility` was **not** in it, so it fell to `default: break`
(`src/engine/game_engine.cpp:1243-1244` pre-fix). I verified this empirically:
with the ChainManager branch wired and `executeIntent` untouched, all four
tests stayed RED with the identical 10-take burst. Both edits are load-bearing;
I proved each by stashing it and re-running (see §2).

**Correction #2 — a second live hole, same root cause, different path.**
`GameEngine::resolveShowdownDecision` (`src/engine/game_engine.cpp:4034-4046`)
*already* listed `case IntentType::ActivateReactionAbility:` in its dispatch
switch and forwarded it to `executeIntent` — where it hit `default: break`. So
a `[Reaction]`-timing activation during a **showdown** was silently discarded
too: focus passed and `players_passed_focus` was cleared while nothing
happened. This is the same finding shape as
`tests/cards/test_combat_showdown_dispatch.cpp`'s PlayReaction one. The single
`case` label fixes both paths; I added test (5) to guard it.

**Correction #3 — the immediate-resolution case does not exist in this engine
today.** The brief assumed Seal of Discord "resolves immediately … no chain
item is added". Per CR 429.2 it should; per the code it does not. Every
activation, `[Add]` abilities included, goes `executeIntent` → `addAbility`
(`chain_manager.cpp:71`) → `runChain()`, and inside the closed-state loop
`runChain` returns immediately (`isProcessing()`), so the item stays on the
chain and `onActivate` runs later at `stepResolve`. Observable proof: test (3)
sees a `ChainItemFinalizedEvent` and a `ChainItemResolvedEvent` for the
ability's source. I did **not** change that (out of scope, and it is a
CR-faithfulness question about Add abilities generally, not about this bug) —
but I implemented the immediate-resolution case explicitly anyway, so the FEPR
loop is correct if/when an ability ever resolves without a chain item. See §3.

**Correction #4 — test (3)'s registry search came up empty.** I checked every
card overriding `isReactionAbility()` (27 files). All of them are `[Add]`
abilities: Seals 40/81/120/163/204/245 and second printings 536/538/541/542/
545/549, Energy Conduit 98, Lux Crownguard 312, Dragonsoul Sage 655, Gold
326/564, Ancient Henge 438, and Legends 294/296/506/786 (the three spells that
mention it — 95/334/428 — are spells, not activated abilities). **There is no
registry card whose `[Reaction]` ability has a non-Add on-resolve effect.**
Rather than skip the coverage I registered a test-local gear (def 921,
`[E]: [Reaction]` — draw 1) whose effect can only land if the chain item is
finalized and resolved, and asserted the finalize/resolve events plus the
draw. Noted here so nobody thinks a shipped card is under test.

## 2. RED → GREEN, per test

New file: `tests/cards/test_closed_state_abilities.cpp` (5 tests, all driving
the real FEPR loop the way `tests/cards/test_closed_state_plays.cpp` does).

| # | Test | RED at HEAD | GREEN after fix |
|---|------|-------------|-----------------|
| 1 | `ClosedStateSealActivationExhaustsAndAddsPower` | `is_exhausted` false (expected true); Chaos power 0 (expected 1) | OK |
| 2 | `ExhaustedSealIsNotOfferedAgain` | offer #1 still lists the activation for the un-exhausted Seal | OK |
| 3 | `ChainAddingReactionAbilityRestartsFeprFromFinalize` | 0 finalize events, 0 resolve events, deck unchanged, hand 0 vs 1 | OK |
| 4 | `RepeatedActivationChoiceDoesNotBurst` | **10** activations taken vs ≤3; Chaos power 0 vs 3; all 3 Seals ready | OK |
| 5 | `ShowdownReactionActivationExecutes` | `is_exhausted` false; Chaos power 0 vs 1 | OK |

Load-bearing check for each half of the fix (each stashed, rebuilt, re-run):

- `src/engine/game_engine.cpp` changes stashed → test 5 RED (both assertions).
- `src/engine/chain_manager.cpp` branch stashed (header + engine wiring kept)
  → tests 1–4 RED, identical 10-take burst.
- Full HEAD (`git stash push -- src/engine/`) → all 5 RED.

## 3. The fix

Three files, no core-header edits (`chain_manager.h` is the allowed one).

**`src/engine/chain_manager.h`** — new injected executor, sibling of
`setPlaySpell` / `setPlayCard`:

```
using ActivateAbility = std::function<bool(const Intent&)>;
void setActivateAbility(ActivateAbility act);
```

The callback returns **"did the activation actually execute"**, deliberately
not "did the chain grow". The engine can see what its own executor did; the
ChainManager cannot, and guessing from the chain is precisely what would
mis-handle an immediate resolution (false "executed nothing" warn) or a
legitimate rejection (false FEPR restart).

**`src/engine/chain_manager.cpp`** — `stepExecuteAndPass` grows a branch for
`ActivateReactionAbility` / `ActivateAbility` / `ActivateActionAbility`
replacing the "Phase 3+" comment:

- executor injected and it returns true → `return true` (restart FEPR from
  Finalize). This covers **both** shapes. Chain grew → the new item must be
  finalized before anyone gets priority again (CR 337.1.b.3). Chain unchanged
  (immediate resolution, CR 429.2) → nothing to finalize, but a player *acted*,
  so the accumulated passes are stale; restarting is what clears
  `players_passed_priority` and re-seats priority on the newest item's
  controller — exactly the bookkeeping a play gets. The no-chain-item case
  logs a `logTrace` so it is visible, and explicitly does not warn.
- executor returns false → `logWarn` "executed nothing — intent rejected" +
  `continue`, mirroring the existing play path.
- no executor injected (bare-ChainManager unit tests) → `logWarn` +
  `continue`, mirroring the permanent-play rejection. No hand-rolled cost
  payment: `ActivationCost` means exhaust, `[Disempower]` (with the CR 828
  legality re-check), energy off ready runes, recycle-self, discard and XP,
  plus per-ability cost lookup and the aura-granted-ability indirection.

**`src/engine/game_engine.cpp`** — two changes:

1. `initSubsystems` wires `setActivateAbility` to a lambda that calls
   `executeIntent` and answers the contract. It reports success on: chain grew;
   or the controller's resource fingerprint changed (pool energy, universal
   power, per-domain power, XP, hand size, deck size — this is what catches an
   `[Add]` ability that resolved without a chain item); or the source
   disappeared (Gold 326 kills itself as part of its cost); or the source's
   `is_exhausted` / `is_empowered` flipped. A rejection — token source with no
   `CardDef`, or a `[Disempower]` cost on a non-Empowered source (CR 828) —
   changes none of them and correctly reports false.
2. `executeIntent`'s activation case gains
   `case IntentType::ActivateReactionAbility:`. This is the line that actually
   pays the cost and dispatches `onActivate`, for **both** the closed-state and
   the showdown paths. Cost payment and dispatch are timing-independent — the
   timing lives in the generators and in the priority bookkeeping around the
   call — so all three intent types share one body, as the two already did.

## 4. Suite + coverage

```
$ cmake --build build            # clean, no warnings from these files
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1240 tests from 135 test suites ran. (2031 ms total)
[  PASSED  ] 1240 tests.
  YOU HAVE 1 DISABLED TEST
```

Baseline was 1202 + 1 disabled. 1240 = 1202 + my 5 + 33 added by the
concurrent agent's `ec2a706` (`tests/test_prior_config.cpp` and additions to
`tests/test_agent_spec.cpp` / `tests/test_corpus_evaluator.cpp`). The one
disabled test is the pre-existing one.

```
$ python3 scripts/card_coverage.py
implemented 716 / engine-handled 7 / metadata-only 0 / stub 0 / vanilla 69
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

Unchanged — no card files were touched.

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='ClosedStateAbilit*'
[  PASSED  ] 5 tests.
```

## 5. Files

- `src/engine/chain_manager.h` — `setActivateAbility` + `activate_ability_`.
- `src/engine/chain_manager.cpp` — the activation branch in
  `stepExecuteAndPass`; `<string>` include.
- `src/engine/game_engine.cpp` — `setActivateAbility` wiring in
  `initSubsystems`; `ActivateReactionAbility` case label in `executeIntent`.
- `tests/cards/test_closed_state_abilities.cpp` — new, 5 tests.

Untouched, as instructed: `src/agents/*`, `src/main.cpp`,
`tests/test_agent_spec.cpp`, `tests/test_corpus_evaluator.cpp`,
`tests/test_prior_config.cpp`, `docs/superpowers/priors/`.

## 6. Concerns

1. **CR 429.2 is still not modelled.** `[Add]` abilities ("Abilities that add
   resources can't be reacted to") go on the chain in this engine and resolve
   in LIFO order like anything else, so an opponent gets a priority window
   between a Seal activation and its resolution, and the power arrives later
   than it should. Every `[Reaction]` ability in the registry is an `[Add]`
   ability, so this is the whole class. My fix makes the FEPR loop handle the
   immediate case correctly *if* someone changes that, but changing it is a
   separate ruling — it would alter when power is available for a reaction
   play and is well beyond "make the activation execute".
2. **The success fingerprint is a heuristic, not a proof.** A hypothetical
   zero-cost ability that resolved immediately and changed *nothing* the
   fingerprint watches would be reported as "executed nothing" and warn. No
   such ability exists; the honest fix if one appears is for `executeIntent`
   to return a status, which needs a `game_engine.h` edit (out of scope here).
3. **Test (3) uses a test-local card**, for the registry reason in §1
   correction #4. If a non-Add `[Reaction]` ability ever ships, that test
   should be re-pointed at it.
4. **`generateClosedStateActions` gates affordability but `executeIntent`
   re-validates only the `[Disempower]` component.** A hand-built intent whose
   energy cost is unpayable will still exhaust the source and spend whatever
   runes exist. Pre-existing; unchanged by this commit; noted because the new
   callback widens who can reach that path (the ChainManager now can).
5. **The `[Reaction]` timing itself is enforced only by the generators.**
   `executeIntent` will happily run an `ActivateReactionAbility` at any time it
   is handed one. That matches how `ActivateActionAbility` already behaved, so
   I kept the shape rather than inventing a timing check in the executor.
