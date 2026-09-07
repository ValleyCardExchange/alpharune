# L0b — equip legality (spec addendum #16) — fix report

Branch `replay-loop`, started at `b406465`. **Commit `b1f2a89`** —
`Engine: equip offers gated by Card::canEquip; rejected equips warn; power
recycle per CR 164.2.b`. One commit, nothing else in the tree.

## 1. Files touched (15, all staged explicitly — no `git add -A`)

| File | What changed |
| --- | --- |
| `src/cards/card.h` | new `virtual bool canEquip(const GameState&, PlayerId) const { return true; }` beside `hasEquipAbility` / `onEquip`, with the doc contract ("onEquip MUST consult this first") |
| `src/cards/gear/equip_base.h` | `StandardEquipScan` + `scanStandardEquip` + `canStandardEquip` extracted from `standardEquip`'s pre-check (one scan, used by both); new `canUniversalEquip`; `SimpleEquipGear` and `UniversalEquipGear` gain `canEquip` and call it first in `onEquip` |
| `src/cards/card_helpers.h` | `payOnePower` loses the ready-only condition (CR 164.2.b) and prefers an EXHAUSTED rune; new `findPowerRune` + `canPayOnePower` predicate |
| `src/cards/gear/0365_brutalizer.cpp` | `canEquip = canStandardEquip(0, Calm)` |
| `src/cards/gear/0382_svellsongur.cpp` | `canEquip = canStandardEquip(1, Calm)` |
| `src/cards/gear/0412_the_zero_drive.cpp` | `canEquip = canStandardEquip(1, Mind)`; its **private clone of `standardEquip`** (`payEquipAndAttach`, 57 lines) deleted — now calls the canonical helper |
| `src/cards/gear/0460_edge_of_night.cpp` | `canEquip = canStandardEquip(0, Chaos)` |
| `src/cards/gear/0471_last_rites.cpp` | `canEquip = trash >= 2 AND canStandardEquip(0, Chaos)`; `onEquip` checks it BEFORE recycling the two trash cards (was a partial payment) |
| `src/cards/gear/0498_blade_of_the_ruined_king.cpp` | `canEquip = canPayOnePower(Order) AND >= 2 friendly on-board units`; hand-rolled Order recycle replaced by `payOnePower`; hand-rolled pre-checks removed |
| `src/cards/gear/0508_rabadon_s_deathcrown.cpp` | `canEquip = canPayOnePower(nullopt)` |
| `src/cards/gear/0601_soul_sword.cpp` | `canEquip = canPayOnePower(Calm)` |
| `src/cards/gear/0720_shepherd_s_heirloom.cpp` | `canEquip = xp >= 1`; duplicated `if (ps.xp < 1)` removed |
| `src/cards/gear/0748_hextech_gauntlets.cpp` | target-agnostic `canEquip` (a rune to recycle for [A] + enough ready runes for the CHEAPEST legal target, i.e. the Mightiest friendly unit); the target-specific energy check stays in `onEquip` because the cost varies with the chosen unit's Might |
| `src/engine/game_engine.cpp` | `generateMainPhaseActions`: `if (!gear_card->canEquip(state_, player)) continue;` placed BEFORE the `needsEquipTimeTarget` branch, so it covers both that path and the legacy per-unit enumeration. `executeIntent`: the `else` arm now `logWarn`s `"EQUIP: <gear> rejected — offered but unpayable (canEquip/onEquip disagree)"`; no other control-flow change |
| `tests/cards/test_equip_legality.cpp` | NEW, 6 tests |

No card-name logic entered the engine — it gates on the `Card::canEquip`
virtual only.

## 2. Gear that received a `canEquip` predicate — 36 in total

The registry guard enumerates them from `CardRegistry::classDefIds()`, so
this list is what the test proves, not what I remembered to grep.

- **via `SimpleEquipGear`** (23): 332 Serrated Dirk, 339 Recurve Bow,
  345 Long Sword, 353 Skyfall of Areion, 356 Doran's Shield,
  374 Guardian Angel, 379 Sterak's Gage, 387 Cloth Armor,
  396 Experimental Hexplate, 408 World Atlas, 417 Doran's Blade,
  424 Hexdrinker, 430 Warmog's Armor, 437 Trinity Force, 439 Boneshiver,
  445 Doran's Ring, 454 Boots of Swiftness, 455 Cull,
  474 Eye of the Herald, 482 B.F. Sword, 493 Sacred Shears,
  581 Blighted Battleaxe, 658 Hunter's Machete
- **via `UniversalEquipGear`** (3): 504 Spinning Axe, 507 Forgefire Cape,
  509 Shurelya's Requiem — **these three attached FOR FREE** on an empty
  base before this change; the base class had no pre-check at all
- **hand-written** (10): 365 Brutalizer, 382 Svellsongur,
  412 The Zero Drive, 460 Edge of Night, 471 Last Rites,
  498 Blade of the Ruined King, 508 Rabadon's Deathcrown, 601 Soul Sword,
  720 Shepherd's Heirloom, 748 Hextech Gauntlets

`grep -rn "hasEquipAbility() const override { return true"` over `src/cards`
finds exactly the 10 hand-written files plus the two base classes; there is
no `src/cards/manual/` directory on this branch and no equip gear outside
`src/cards/gear/`.

## 3. RED evidence (observed, not assumed)

`Card::canEquip` was added to `card.h` FIRST with the default `true`, so the
guard could compile and name the offenders rather than fail to build.
Build of the test file alone, then:

```
RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='EquipLegalityTest.*'
[==========] 6 tests from 1 test suite ran. (39 ms total)
[  PASSED  ] 0 tests.
[  FAILED  ] 6 tests, listed below:
[  FAILED  ] EquipLegalityTest.RegistryGuard_EveryEquipGearIsUnpayableOnABareBoard
[  FAILED  ] EquipLegalityTest.RegistryGuard_RejectedOnEquipLeavesTheBoardUntouched
[  FAILED  ] EquipLegalityTest.LastRites_LegalityAndAtomicPaymentAcrossThreeBoards
[  FAILED  ] EquipLegalityTest.Generator_UnpayableEquipIsNotOfferedButPayableOneIs
[  FAILED  ] EquipLegalityTest.SoulSword_EquipsOffAnExhaustedCalmRuneAndPrefersIt
[  FAILED  ] EquipLegalityTest.Executor_RejectedEquipIntentLogsAWarningAndPaysNothing
```

**Guard #1 — 36 named failures**, one per equip gear:

```
Value of: c->canEquip(state, P1)
  Actual: true
Expected: false
#748 Hextech Gauntlets declares hasEquipAbility() but reports canEquip == true
with no runes, no trash and 0 XP. Every equip gear must override
Card::canEquip with its real affordability check (Card::canEquip defaults to
true).
```

(the same message for #332 Serrated Dirk, #339 Recurve Bow, #345 Long Sword,
#353 Skyfall of Areion, #356 Doran's Shield, #365 Brutalizer,
#374 Guardian Angel, #379 Sterak's Gage, #382 Svellsongur, #387 Cloth Armor,
#396 Experimental Hexplate, #408 World Atlas, #412 The Zero Drive,
#417 Doran's Blade, #424 Hexdrinker, #430 Warmog's Armor,
#437 Trinity Force, #439 Boneshiver, #445 Doran's Ring,
#454 Boots of Swiftness, #455 Cull, #460 Edge of Night, #471 Last Rites,
#474 Eye of the Herald, #482 B.F. Sword, #493 Sacred Shears,
#498 Blade of the Ruined King, #504 Spinning Axe, #507 Forgefire Cape,
#508 Rabadon's Deathcrown, #509 Shurelya's Requiem,
#581 Blighted Battleaxe, #601 Soul Sword, #658 Hunter's Machete,
#720 Shepherd's Heirloom — 36 total)

**Guard #2 — the free-attach bug**, the three `UniversalEquipGear` cards:

```
Value of: ok
  Actual: true
Expected: false
#504 Spinning Axe equipped on a board with no runes, no trash and 0 XP — its
cost cannot have been paid.
Value of: state.getObject(gear).attached_to.has_value()
  Actual: true
Expected: false
#504 Spinning Axe attached itself without paying.
```
(identically for #507 Forgefire Cape and #509 Shurelya's Requiem)

**Last Rites** — scenarios B and C RED (A already passed, because the
energy+domain payer never required a ready rune):

```
Value of: c->canEquip(state, P1)   Actual: true  Expected: false
Google Test trace: B: 2 trash + no Chaos rune
Value of: c->canEquip(state, P1)   Actual: true  Expected: false
Google Test trace: C: 1 trash + Chaos rune
```

**Generator**:

```
Expected equality of these values:
  equipOffers()  Which is: 1
  0
an unpayable Last Rites must not be offered — the agent re-picks a
legal-but-inert intent until the main-phase action cap (497 repeats on seed
2000 before this fix).
```

**Soul Sword (CR 164.2.b)**:

```
Value of: c->onEquip(ctx, unit)   Actual: false  Expected: true
CR 164.2.b's power recycle has no readiness condition, and the engine's
canonical payer already allows exhausted runes.
Google Test trace: only exhausted Calm runes
```

**Executor**:

```
Value of: warned   Actual: false  Expected: true
executeIntent must LOG a warning naming the gear when onEquip returns false
```

## 4. GREEN

```
RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='EquipLegalityTest.*'
[  PASSED  ] 6 tests.

RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1246 tests from 136 test suites ran. (2134 ms total)
[  PASSED  ] 1246 tests.
  YOU HAVE 1 DISABLED TEST
```

Baseline was 1240 / 1 disabled → **1246 / 1 disabled** (+6, exactly the new
file). `tests/cards/test_equipment.cpp` and `tests/cards/test_audit_fixes_*.cpp`
are **unmodified** (not in the commit) and green. Nothing asserted the
ready-only power rule, so nothing had to be reported as a blocked edit.

Coverage gate:

```
python3 scripts/card_coverage.py
TOTAL                                          792
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

Unchanged from the pre-change baseline (792 / INCOMPLETE 0 / 50 engine-gap).

## 5. Smoke — seed 2000

```
./build/riftbound --agent1 mcts:sims=20 --agent2 mcts:sims=20 \
  --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
  --seed 2000 --render-html on 2>&1 | tail -2
Game over: winner=2 | reason=P2 reached 8 points | turns=13
Replay saved to replays/20260907-211919/replay.html

grep -o "src=Last Rites" replays/20260907-211919/replay.html | wc -l
1
```

**497 → 1**, and the surviving one is a real equip
(`src=Last Rites) tgt=[Kennen, Storm of Shuriken]`). Game length **13 turns**,
completed on points rather than on the action cap. No
`EQUIP: … rejected` warnings appear in the replay — expected, since the
generator no longer offers an unpayable equip for the executor to reject.

Two extra seeds as a sanity check (no hangs, no cap-outs):
`--seed 2001` → winner=1, turns=15; `--seed 2002` → winner=1, turns=13.

## 6. Concerns / judgment calls the controller should check

1. **`canEquip` is target-agnostic by design** (the plan's signature), but
   two gears have target-dependent costs:
   - **748 Hextech Gauntlets** — energy is `3 − target Might`. `canEquip`
     answers for the CHEAPEST legal target (the Mightiest friendly unit),
     so the generator can still offer the gear against a low-Might unit it
     cannot afford; `onEquip` rejects that one and the executor warns. This
     is a narrowing of the bug, not a full close. Closing it needs a
     per-target legality hook the plan does not authorise.
   - **498 Blade of the Ruined King** — I used "≥ 2 friendly on-board
     units" as the target-agnostic form of "a killable friendly unit other
     than the target". With ≥ 2 units, every possible target leaves a
     victim, so this is exact, not an approximation.
2. **`0498`'s `pickTarget` suspension path** returns `false` from `onEquip`
   and would therefore trigger the new warning. In practice the equip path
   runs with no `chain.resuming` context, so `Card::pickTarget` takes its
   direct-invocation branch and returns the first legal target instead of
   suspending — the warning cannot fire from there today. Worth knowing if
   equip is ever driven from inside a chain resolve.
3. **`0412 The Zero Drive` now calls the canonical `standardEquip`** instead
   of its own byte-identical private copy. The only behavioural delta is
   two extra `logTrace` lines (`EQUIP_COST: recycled …`, `EQUIP: … ->`).
4. **`standardEquip`'s power recycle still does not prefer an exhausted
   rune** the way `payOnePower` now does — it takes the first matching-domain
   rune in map order, which is pre-existing behaviour and outside this
   task's ruling. Where energy and power both come from a single rune
   (e.g. Svellsongur with exactly one ready Calm rune) that rune pays both;
   also pre-existing, also untouched.
5. **Last Rites' "play a unit from trash for free"** engine gap in
   `onEquippedTrigger` is untouched, per the task.
6. `Card::canEquip` defaulting to `true` means a future gear that forgets to
   override it is offered unpayably again — the registry guard is the only
   thing standing between that and another 497-repeat turn. It enumerates
   from the registry, so it catches new gear automatically.
