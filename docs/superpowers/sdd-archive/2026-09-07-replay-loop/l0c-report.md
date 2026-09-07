# L0c — review follow-ups on L0b and the activation fix — report

Branch `replay-loop`, started at `81d1d59`. A concurrent session landed two
DOCS-ONLY commits (`b985f32`, `591967a`, both under
`docs/superpowers/smoke/2026-09-08-loop-iter0/`) while this work was in
flight, so the commit's parent is `591967a`, not `81d1d59`. No file overlap
with this change. **Commit `192e9e1`** —
`Engine: equip legality per target, no rune double-spend, activation status
reported explicitly`. One commit, staged file by file, nothing else in the
tree.

## 1. Files touched (11)

| File | What changed |
| --- | --- |
| `src/cards/card.h` | new `virtual bool canEquipTarget(const GameState&, PlayerId, GameObjectId) const` defaulting to `canEquip` |
| `src/cards/gear/equip_base.h` | `findPowerRune` MOVED here from `card_helpers.h` (one definition, both payers); `StandardEquipScan` now carries `power_rune` + `energy_runes` (ready runes EXCLUDING the power rune); new `scanEquipCost(state, player, optional<Domain>)` and `payEquipCost(ctx, scan, energy, label)`; `canStandardEquip` / `canUniversalEquip` / `standardEquip` / `UniversalEquipGear::onEquip` all run off that one scan |
| `src/cards/card_helpers.h` | `findPowerRune` removed (now included from `equip_base.h`); `canPayOnePower` / `payOnePower` unchanged in behaviour |
| `src/cards/gear/0748_hextech_gauntlets.cpp` | `energyFor(might)`, `canEquipTarget` (the target's Might), `canEquip` = "exists a friendly unit for which it holds"; its hand-rolled scan+payer replaced by `scanEquipCost`/`payEquipCost` |
| `src/cards/gear/0498_blade_of_the_ruined_king.cpp` | `payOnePower` moved BEFORE `killObject`; unit-exists check hoisted above the payment |
| `src/engine/game_engine.cpp` | generator's legacy per-unit branch gates each `(gear, unit)` on `canEquipTarget`; `executeIntent` returns `bool` with `executed = false` at every reject; new pre-check rejecting an unpayable energy cost before ANY state change; energy payment spends the floating pool first; `setActivateAbility` lambda is now `return executeIntent(intent)` and the resource fingerprint is DELETED |
| `src/engine/game_engine.h` | `executeIntent` → `bool` (private, doc'd); `testHook_executeIntent` forwards the bool |
| `src/agents/agent_spec.cpp` | new `numberOrThrow(obj, key, path, section)`; all 15 `.get<double>()` reads go through it |
| `tests/cards/test_equip_legality.cpp` | +5 tests, +3 engine-state builders, +1 test-local `UniversalEquipGear(1)` (def 930) |
| `tests/cards/test_closed_state_abilities.cpp` | +2 tests, +1 test-local gear (def 922, `[3][E]: [Reaction] — draw 1`), +`PassSetWatchingAgent` |
| `tests/test_prior_config.cpp` | +2 tests |

No card-name logic entered the engine — the generator gates on the
`Card::canEquipTarget` / `Card::canEquip` virtuals only.
`tests/cards/test_equipment.cpp` and `tests/cards/test_audit_fixes_*.cpp` are
**not in the commit** and are green.

## 2. RED evidence, per test (observed at 81d1d59)

### (1) `Boneshiver_OneRuneCannotPayBothEnergyAndPower`
Scope A (one ready Body rune, nothing else) — 5 failures:
```
Value of: c->canEquip(state, P1)   Actual: true   Expected: false
Value of: c->onEquip(ctx, unit)    Actual: true   Expected: false
Expected equality of these values:
  fingerprint(P1)  Which is: "trash[] deck[] runedeck[3 ] runes[3:1 ] xp=0"
  before           Which is: "trash[] deck[] runedeck[] runes[3:0 ] xp=0"
Value of: state.getObject(body).location.has_value()  Actual: false  Expected: true
Value of: state.getObject(gear).attached_to.has_value()  Actual: true  Expected: false
```
Rune 3 is simultaneously exhausted (`3:1`) for the `[1]` and in the rune deck
for the `[D]` — the double-spend, exactly as the review described.

**Honest caveat:** scope B (Body rune + Fury rune → two DISTINCT runes pay the
two halves) happened to PASS at HEAD on this run. `GameState::objects` is a
`std::unordered_map`, so which ready rune the old energy loop grabbed was
iteration-order luck; B is a determinism pin for the fixed payer, not RED
evidence. Scope A is deterministic and always RED.

### (2) `Generator_BoneshiverWithASingleRuneIsNotOffered`
```
Expected equality of these values:
  equipOffersFor(engine, gear).size()  Which is: 1
  0u
```

### (3) `UniversalEquip_EnergyOneWithASingleRuneIsUnpayable`
Scope A: `canEquip` true, `onEquip` true, fingerprint mutated, gear attached
(all expected false/unchanged). Scope B: `exhaustedRuneCount(P1)` 0 vs 1 and
`readyRuneCount(P1)` 1 vs 0 — the same rune paid both halves. Scope C
(generator): 1 offer, expected 0.

### (4) `HextechGauntlets_OnlyTheAffordableTargetIsOffered`
```
Expected equality of these values:
  offers.size()  Which is: 2
  1u
```
Two friendly units (Might 3 and Might 0), one ready rune: the Might-0 unit's
intent costs 3 energy and is unpayable, but the target-agnostic `canEquip`
answered for the cheapest unit and the per-unit loop emitted it anyway.

### (5) `BladeOfTheRuinedKing_PaysThePowerBeforeItKills`
```
Value of: rune_still_in_base_at_kill  Actual: true   Expected: false
Value of: ok                          Actual: false  Expected: true
Value of: state.getObject(gear).attached_to.has_value()  Actual: false  Expected: true
```
The victim was killed, the reaction on its death spent the Order rune,
`payOnePower` then failed and `onEquip` returned false — a dead friendly unit,
nothing paid, nothing attached.

### (6) `UnderpaidActivationIsRejectedAndKeepsThePassSet`
```
Value of: s.getObject(gear).is_exhausted  Actual: true  Expected: false
readyRunesIn(s, P2)  Which is: 0   vs 1
s.player(P2).main_deck.size()  Which is: 0   vs p2_deck_before (1)
Value of: agent2.p1_had_passed[1]  Actual: false  Expected: true
agent1.call_count  Which is: 3   vs 1
```
A hand-built activation for a `[3]` energy cost with ONE ready rune paid the
exhaust, spent the rune, drew the card, and reported success — so FEPR
restarted, `players_passed_priority` was cleared and P1 was re-queried twice
more. No warning was logged.

### (7) `PayableActivationExecutesAndResetsThePassSet`
**Green at HEAD by construction** — it is the control for (6) ("a legitimate
one still resets it"). It pins that the reject path did not cost the engine
its CR 337.1.b.3 bookkeeping: the ability executes, FEPR restarts, and P1 is
re-queried with the pass set cleared. Reported as a control, not as RED
evidence.

### (8)/(9) `LoadPriorConfig.NonNumeric{Family,Evaluator}ValueThrows…`
```
unknown file: Failure
C++ exception with description
  "[json.exception.type_error.302] type must be number, but is string"   thrown in the test body.
  "[json.exception.type_error.302] type must be number, but is boolean"  thrown in the test body.
```
`nlohmann::json::type_error` is not a `std::runtime_error`, so the `catch`
never fired and the exception escaped the test.

## 3. GREEN

```
RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='EquipLegalityTest.*'
[  PASSED  ] 11 tests.

RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='ClosedStateAbilitiesTest.*:LoadPriorConfig.*'
[  PASSED  ] 19 tests.

RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1255 tests from 136 test suites ran. (2075 ms total)
[  PASSED  ] 1255 tests.
  YOU HAVE 1 DISABLED TEST
```
Baseline 1246 / 1 disabled → **1255 / 1 disabled** (+9: 5 equip, 2
closed-state, 2 prior-config).

Gate:
```
python3 scripts/card_coverage.py
TOTAL                                          792
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```
Unchanged.

Untouched-and-green check:
```
git status --short tests/cards/test_equipment.cpp tests/cards/test_audit_fixes_*.cpp   # empty
RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='*Equipment*:*Equip*:*AuditFix*'
[  PASSED  ] 367 tests.
```

## 4. Smoke — seed 2000

```
./build/riftbound --agent1 mcts:sims=20 --agent2 mcts:sims=20 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt --seed 2000 2>&1 | tail -1
Game over: winner=2 | reason=P2 reached 8 points | turns=13
```
Ends on points, not a cap. Identical to L0b's post-fix result — expected, since
neither Kennen nor Rengar runs a gear with a non-zero equip energy cost.

## 5. Concerns and judgment calls

1. **Test (5) does not match the plan's literal wording, and I could not make
   it.** The plan asked for "drain the Order rune between check and pay
   through the test hook … no unit is killed". There is no such hook:
   `Card::pickTarget`'s suspend path publishes a choice and returns, `onEquip`
   returns false, and on re-entry `onEquip` runs from the TOP — so its
   `canEquip` re-check catches a drained rune before anything is killed, in
   BOTH the old and the new order. The only reachable window between the check
   and the payment is inside `killObject` itself, which is precisely the review's
   own scenario (CR 164.2.b's recycle is a `[Reaction]`), so the test drives it
   with a `UnitDied` handler that spends the Order rune. That IS deterministically
   RED at HEAD. The plan's literal outcome ("nothing is killed") is covered by
   scope B as a regression guard, which was green at HEAD.

2. **Energy payment now spends the floating rune pool first — a behaviour
   change beyond the literal ask.** Two readings were available. Checking only
   ready runes would newly reject activations the generators legitimately offer
   (they gate on `availableEnergy`, which counts the pool); checking
   `availableEnergy` without spending the pool would leave a pool-funded cost
   silently unpaid, i.e. the very underpayment being fixed. I took the third
   option and mirrored the engine's other two cost payers (`payCardCost`
   `:6007`, `payAdditionalCost` `:6169`), which both drain the pool before
   touching runes, so the pre-check and the payer share one accounting. Full
   suite is green, but this is the place to look if a downstream number moves.

3. **`ActivationCost::power` / `power_domain` are still never paid by
   `executeIntent`.** The activation path pays exhaust, disempower, energy,
   recycle-self, discard and XP; the power component of an activation cost is
   ignored, and my new pre-check does not cover it either. Pre-existing, out of
   this task's scope, and no shipped ability uses it today — but it is the same
   class of hole as the energy one and should get the same treatment.

4. **`payEquipCost` does not clear `is_exhausted` on the rune it recycles**,
   where `payAdditionalCost` does (`rune.is_exhausted = false` before it goes
   back to the rune deck). That asymmetry is pre-existing on both equip payers
   and on `payOnePower`; I left it alone rather than widen the diff, but a rune
   that returns to the rune deck exhausted is a latent bug the next channel
   could surface.

5. **`Card::canEquipTarget` defaults to `canEquip`**, so a future gear with a
   target-dependent cost that forgets to override it is offered against
   unaffordable targets again. There is no registry guard for that the way
   there is for `canEquip` (the bare-board guard) — the shape is hard to
   enumerate generically, since "the cost depends on the target" is not
   declared anywhere.

6. **`findPowerRune` moved out of `card_helpers.h` into `gear/equip_base.h`.**
   `card_helpers.h` includes `equip_base.h`, not the other way round, so this
   was the only way to give the equip scan and `payOnePower` one shared picker
   instead of two copies of the prefer-exhausted rule. Every existing caller of
   `canPayOnePower` / `payOnePower` compiles unchanged.

7. **Build hygiene note:** one of my rebuild invocations ran `ninja -C build`
   without `-j2` and briefly ran at full parallelism alongside the self-play
   batch, and for a few minutes two ninja instances shared the build directory.
   No process was killed; the tree was afterwards rebuilt to completion, `ninja`
   reports "no work to do", and the full suite links and passes, so no object is
   stale or corrupt. Every other build used `-j2`.
