# Task 5 report — Flow: banish on leave (resolve and counter)

Branch `kennen-tyler-deck`, base HEAD `2b3a27a`.

## Implemented

Files: `src/engine/chain_manager.cpp`, `src/cards/card_helpers.h`,
`src/cards/spells/0045_defy.cpp`, `src/cards/spells/0457_hard_bargain.cpp`,
`tests/cards/test_flow.cpp`. No headers other than `card_helpers.h`
(header-only helpers, no core header touched).

### Counter-disposal sites found

Grepped `src/cards` for `counterChainTop` / `COUNTER:` / `trash.push_back`
inside counter cards. Full inventory:

| File | Shape | Action taken |
|---|---|---|
| `card_helpers.h` (`counterChainTop`) | shared helper, used by the 6 cards below | routed through the new `disposeCounteredSpell` helper |
| `spells/0064_wind_wall.cpp` | calls `counterChainTop(ctx)` | none needed — covered by the shared helper |
| `spells/0368_not_so_fast.cpp` | calls `counterChainTop(ctx)` | none needed |
| `spells/0520_riposte.cpp` | calls `counterChainTop(ctx)` | none needed |
| `spells/0668_repulse.cpp` | calls `counterChainTop(ctx)` | none needed |
| `spells/0750_lilting_lullaby.cpp` | calls `counterChainTop(ctx)` | none needed |
| `spells/0045_defy.cpp` | self-disposing (inline pop + trash push) | edited: capture `banish_on_leave` before pop, call `disposeCounteredSpell` |
| `spells/0457_hard_bargain.cpp` | self-disposing, **two** sites (case 0 "can't afford" auto-counter; case 1 "declined to pay") | both edited: capture `banish_on_leave` before pop, call `disposeCounteredSpell` |
| `spells/0693_abandon.cpp` | self-disposing, but redirects to **hand**, not trash ("Return it to its owner's hand instead of putting it in their trash") | **left unchanged** — see note below |

**Abandon (693) excluded, deliberately.** Its own text overrides the
trash destination outright — the countered spell never goes to trash at
all, Flow-played or not. CR 829.1.b.1 replaces a trash-bound disposal with
Banishment; Abandon's disposal was never trash-bound, so there is nothing
for the Flow rule to redirect. Routing it through `disposeCounteredSpell`
(which only knows trash-or-banish) would either silently drop the
hand-return behavior or require a third branch nobody asked for. Flagging
this for the controller in case the ruling should be explicit rather than
inferred.

### `card_helpers.h` — the shared helper

```cpp
inline void disposeCounteredSpell(CardContext& ctx, GameObjectId spell,
                                   bool banish_on_leave) {
    if (!ctx.state.objectExists(spell)) return;
    auto& obj = ctx.state.getObject(spell);
    obj.location = std::nullopt;
    if (banish_on_leave) {
        obj.zone = ZoneType::Banishment;
        obj.is_empowered = false;  // CR 441.1.a
        ctx.state.player(obj.owner).banishment.push_back(spell);
        ctx.events.logTrace("FLOW: " + obj.name + " banished (countered)");
    } else {
        obj.zone = ZoneType::Trash;
        ctx.state.player(obj.owner).trash.push_back(spell);
        ctx.events.logTrace("COUNTER: " + obj.name + " countered -> trash");
    }
}
```

`counterChainTop` now captures `top.banish_on_leave` **before** popping the
item (the item, and its flag, are gone once popped) and calls the helper.
`0045_defy.cpp` and both sites in `0457_hard_bargain.cpp` do the same —
each capture point replaces a card-specific inline trash-move with a call
to the shared helper. No `LeftBoardEvent` is emitted on counter disposal
(pre-existing behavior — none of the six `counterChainTop` cards, nor Defy
nor Hard Bargain, ever emitted one on counter; the helper doesn't add one).
Log-line wording for the trash branch is unified across all sites
(`COUNTER: <name> countered -> trash`), dropping each card's bespoke
suffix ("by Defy", "controller declined to pay", …); no test asserted the
old wording.

### `ChainManager::stepResolve` — the resolve branch

Branches on `resolved.banish_on_leave`, mirroring
`EffectExecutor::banishObject`'s zone/location/`is_empowered` handling
inline (chosen over calling the executor, since `executor_` can be null on
some `ChainManager` construction paths — several tests build a bare
`ChainManager` with no executor set):

- `false` (unchanged): zone `Trash`, `ps.trash.push_back`.
- `true`: zone `Banishment`, `is_empowered = false` (CR 441.1.a),
  `ps.banishment.push_back`, `logTrace("FLOW: <name> banished")`.

Both branches still emit `SpellResolvedEvent{resolved.source,
resolved.controller}` and a `LeftBoardEvent` whose `destination` now
reflects the real zone (`ZoneType::Trash` or `ZoneType::Banishment`)
instead of being hardcoded to `Trash`.

## TDD — RED/GREEN per test

Build: `cmake --build build --target riftbound_tests -j$(nproc)` (the
`card_helpers.h` edit triggered the expected ~10-15 min partial rebuild).
Run: `RIFTBOUND_ROOT=.. ./riftbound_tests --gtest_filter='FlowTest.*'`
from `build/`; full suite `RIFTBOUND_ROOT=. ./build/riftbound_tests` from
the repo root.

### Step 1–2 — #9, RED

```
[ RUN      ] FlowTest.ResolvingAFlowPlayedSpellBanishesItInsteadOfTrashing
test_flow.cpp:532: Failure
Expected equality of these values:
  s.getObject(spell).zone       Which is: 1-byte object <04>   (Trash)
  ZoneType::Banishment          Which is: 1-byte object <05>
test_flow.cpp:535: Failure  banishment.find != end()   — not found
test_flow.cpp:538: Failure  trash.find != end()        — found (it WAS in trash)
[  FAILED  ] FlowTest.ResolvingAFlowPlayedSpellBanishesItInsteadOfTrashing
```

Exactly the brief's predicted failure: the flow-played spell resolved into
trash, pre-Task-5.

### Step 3–4 — implement the `stepResolve` branch; GREEN

```
[ RUN      ] FlowTest.ResolvingAFlowPlayedSpellBanishesItInsteadOfTrashing
[       OK ] FlowTest.ResolvingAFlowPlayedSpellBanishesItInsteadOfTrashing (2 ms)
```

### Step 5–6 — #10 (+ the counterChainTop case), RED

Two tests written to cover both counter shapes:

- `HardBargainCountersAFlowSpellToBanishmentWhenTargetCantAfford` — a
  self-disposing card (drives the real chain via
  `driveThroughChain`/`ChainManager::processFEPR`, so Hard Bargain's own
  ordinary resolve-to-trash and its target's disposal are both real engine
  behavior, not asserted by hand).
- `RepulseCountersAFlowSpellToBanishmentViaCounterChainTop` — the shared
  `counterChainTop` helper (precedent:
  `tests/cards/test_counter_spells.cpp`'s `RepulseTest.CountersOnResolve`).

```
[ RUN      ] FlowTest.HardBargainCountersAFlowSpellToBanishmentWhenTargetCantAfford
test_flow.cpp:570: Failure  zone Which is: <04> (Trash) vs Banishment
test_flow.cpp:574: Failure  banishment.find != end() — not found
test_flow.cpp:577: Failure  trash.find != end() — found
[  FAILED  ]

[ RUN      ] FlowTest.RepulseCountersAFlowSpellToBanishmentViaCounterChainTop
test_flow.cpp:604: Failure  zone Which is: <04> (Trash) vs Banishment
test_flow.cpp:606: Failure  banishment.find != end() — not found
test_flow.cpp:608: Failure  trash.find != end() — found
[  FAILED  ]
```

### Step 6 — implement `counterChainTop` + the two Hard Bargain sites + Defy; GREEN

```
[ RUN      ] FlowTest.HardBargainCountersAFlowSpellToBanishmentWhenTargetCantAfford
[       OK ] (2 ms)
[ RUN      ] FlowTest.RepulseCountersAFlowSpellToBanishmentViaCounterChainTop
[       OK ] (2 ms)
```

Also re-ran the full pre-existing counter-spell suite
(`WindWallTest`, `NotSoFastTest`, `LullabyTest`, `DefyTest`, `AbandonTest`,
`RepulseTest`, `HardBargainTest`/`LegalTargetsFixture`, `JhinDeckTest`) —
62 tests, all green, no regressions from the wording/helper change.

### Step 7–8 — #11, verified

```
[ RUN      ] FlowTest.GrantedFlowExpiresWhenTurnNumberAdvances
[       OK ] FlowTest.GrantedFlowExpiresWhenTurnNumberAdvances (2 ms)
```

No production change needed: Task 4 already implemented the evaluated
`valid_on_turn == turn.turn_number` expiry check consulted by
`GameEngine::liveFlowCosts` (`game_engine.cpp:3048`). This test is a
confirmatory addition per the brief's Step 9's own framing ("cheap, matches
the plan") — it passed on first run, both the sanity assertion (grant
offered when stamped with the current turn) and the real assertion (not
offered once `turn.turn_number` is incremented).

### Final full suite (pre-commit)

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1084 tests from 118 test suites ran. (1874 ms total)
[  PASSED  ] 1084 tests.
  YOU HAVE 1 DISABLED TEST
```

1080 pre-existing + 4 new (`ResolvingAFlowPlayedSpellBanishesItInsteadOfTrashing`,
`HardBargainCountersAFlowSpellToBanishmentWhenTargetCantAfford`,
`RepulseCountersAFlowSpellToBanishmentViaCounterChainTop`,
`GrantedFlowExpiresWhenTurnNumberAdvances`), 1 pre-existing disabled test.
No regressions.

## Files changed

- `src/engine/chain_manager.cpp` — `stepResolve` disposal branch.
- `src/cards/card_helpers.h` — new `disposeCounteredSpell` helper;
  `counterChainTop` routed through it.
- `src/cards/spells/0045_defy.cpp` — routed through `disposeCounteredSpell`.
- `src/cards/spells/0457_hard_bargain.cpp` — both self-disposal sites
  routed through `disposeCounteredSpell`.
- `tests/cards/test_flow.cpp` — tests #9, #10 (Hard Bargain + Repulse), #11.

## Self-review

- **Every counter disposal site routed through the one helper** — the 6
  `counterChainTop` users get it for free via the shared helper; Defy and
  both Hard Bargain sites edited directly. Abandon deliberately excluded
  (never trash-bound — see note above) and flagged for the controller. ✔
- **Resolve branch emits the same events with the right destination** —
  `SpellResolvedEvent` fires on both branches; `LeftBoardEvent::destination`
  is `Trash` or `Banishment` per `resolved.banish_on_leave`, everything
  else on the event unchanged. ✔
- **No extra behaviour** — no new events on counter disposal (matches
  pre-existing zero-event counter behavior); each card's non-disposal logic
  (Hard Bargain's rune-exhaustion rescue, Defy's cost gate, Repulse's
  friendly-target check, `revertCounteredPlay`'s accounting) untouched. ✔
- **Tests assert zones on real objects** — every new test checks
  `state.getObject(id).zone` plus membership in the concrete
  `PlayerState::banishment`/`trash` vectors, not a return value or a mock. ✔
- **`executor_` nullability** — `stepResolve`'s branch does the
  zone/location/`is_empowered`/list-push work inline rather than calling
  `EffectExecutor::banishObject`, since `executor_` can be null on some
  `ChainManager` construction paths (several existing tests build a bare
  `ChainManager`). Mirrors `banishObject`'s logic exactly except it doesn't
  emit a second `LeftBoardEvent` (the resolve branch already emits its own).
- **`card_helpers.h` rebuild** — confirmed and accepted (~10-15 min ninja
  rebuild for the ~10-15 dependent card files), as the brief flagged.

## Concerns

1. **Abandon (693)** is the one self-disposing counter card left outside
   the shared helper, because its printed text redirects the countered
   spell to hand rather than trash. If a Flow-played spell is ever
   countered by Abandon, it currently goes to hand (Abandon's printed
   effect), never to Banishment. I read this as correct per CR 829.1.b.1
   (there's no trash-bound leaving event for the rule to intercept) but it
   is a judgment call, not something the brief called out explicitly —
   worth a one-line ruling if it matters for tournament-accuracy audits.
2. **Log-line wording changed** for the trash branch on Defy and Hard
   Bargain (their old bespoke suffixes — "by Defy", "controller declined to
   pay" — are gone from the *disposal* line; Hard Bargain's own
   `HARD BARGAIN: countered …` context line is kept, immediately above the
   generic `COUNTER: … -> trash` line from the helper). No test depended on
   the old wording; flagging in case a human reads trace logs for QA.
3. Per Task 4's own note (now resolved by this task): the flag-consulted
   disposal sites are exactly the two the spec named — no third site was
   found once Abandon's hand-redirect is excluded from consideration.

---

# Fix round 1 — route every counter disposal through the Flow-aware helper

Commit on top of `d37ace7`. Files: `src/cards/spells/0606_flurry_of_feathers.cpp`,
`src/cards/spells/0693_abandon.cpp`, `tests/cards/test_flow.cpp`.

## Finding 1 (Critical) — Flurry of Feathers (606) missed entirely

`onResolve`'s mode-0 branch ("Counter a spell") popped the chain top and
moved the victim to trash inline, with no `banish_on_leave` capture and no
`disposeCounteredSpell` call — a first-pass miss (I grepped `counterChainTop`
/ `COUNTER:` / `trash.push_back` inside counter cards, but Flurry's disposal
block has none of those three substrings verbatim: it says
`"FLURRY OF FEATHERS: countered spell"` and assigns `sp.zone = ZoneType::Trash`
directly — the grep for `trash.push_back` *did* match it, but I read past it
in the first pass without checking whether it routed through the helper).

**Fix:** capture `victim.banish_on_leave` before `chain.items.pop_back()`,
call `disposeCounteredSpell(ctx, victim.source, banish_on_leave)`. This also
incidentally fixes a latent gap in the pre-existing code: it never set
`obj.location = std::nullopt` and it silently dropped the disposal if
`owner == PlayerId::None` (a defensive guard no test ever exercised, since
`pushSpellOnChain` always sets a real owner) — both now match every sibling
counter site via the shared helper. `revertCounteredPlay` was and remains
NOT called by Flurry (pre-existing; out of this fix's scope — not something
the review asked for, and changing it would be an untested behavior change
on a card the review didn't flag for that).

## Finding 2 (Important — controller ruling) — Abandon (693) banish overrides hand-return

Re-read CR 829.1.b.1 per the ruling: it replaces the LEAVE-THE-CHAIN event
itself, not merely a trash-bound destination. Abandon's own text
("Return it to its owner's hand instead of putting it in their trash")
still describes leaving the chain, and that leaving is instructed by
*Abandon's* execution, not the countered spell's own — so a Flow-played
victim is banished exactly as CR 829.1.b.1 says, superseding Abandon's
redirect.

**Fix:** capture `top.banish_on_leave` before the pop; if set, call
`disposeCounteredSpell(ctx, countered_source, /*banish_on_leave=*/true)`
(always banishes, since the flag is true); otherwise Abandon's existing
hand-return code runs unchanged, byte-for-byte.

## Finding 3 (Important) — exhaustive second-pass inventory

Grepped `src/cards` for every chain-item-removal shape and every
counter-flavored card, independent of the first pass's narrower grep:

**`chain.items.pop_back` / `chain.items.erase` in `src/cards`:**

| Site | Verdict |
|---|---|
| `card_helpers.h:201` (`counterChainTop`) | routed (Task 5 main pass) |
| `0045_defy.cpp:51` | routed (Task 5 main pass) |
| `0457_hard_bargain.cpp:62,135` | routed, both sites (Task 5 main pass) |
| `0606_flurry_of_feathers.cpp:37` (was; now via helper) | **routed this round** |
| `0693_abandon.cpp:42` (was; now via helper for the banish branch) | **routed this round** |
| `chain.items.erase` anywhere in `src/cards` | zero hits — the only `.erase(` on chain items is in `src/engine/chain_manager.cpp` (`stepFinalize`, permanent resolution — unrelated to countering) |

That is every `pop_back`/`erase` on `chain.items` inside `src/cards` —
5 sites, all now routed.

**`chain.resuming` manipulation in `src/cards`:** ~90 hits, all
`resume_point` bookkeeping for ordinary resumable resolve/trigger loops
(discard-then-act, predict, pick-target yield points, etc.) on cards with
no counter text at all (Mindsplitter, Jax, Rek'Sai, Hwei, Sabotage, Stacked
Deck, Party Favors, Whirlwind, Ezreal, and ~80 more `resume_point == 7/10/12`
target-yield checks). None of these remove or dispose of a *different*
chain item the way a counter does — they read/write the executing card's
own resuming slot. Verdict: **not a counter disposal** for every hit; no
action.

**Every spell/ability whose text says "Counter":** grepped
`"Counter a "`, `"Counter an "`, `"Counter that "`, `"counters it"`,
`"is Countered"`, `"counter it (put it in"` across `src/cards` (broader
than the first pass's file-level `-i "counter"` grep, which caught 53 files
most of which just mention "encounter" or "counterattack" in flavor text)
and cross-checked with a plain `"Counter "` grep filtered for those two
false-positive words:

| Card | Disposal shape | Verdict |
|---|---|---|
| Wind Wall (64) | `counterChainTop` | routed |
| Not So Fast (368) | `counterChainTop` | routed |
| Riposte (520) | `counterChainTop` | routed |
| Repulse (668) | `counterChainTop` | routed |
| Lilting Lullaby (750) | `counterChainTop` | routed |
| Defy (45) | self-disposing | routed (main pass) |
| Hard Bargain (457) | self-disposing, 2 sites | routed (main pass) |
| Flurry of Feathers (606) | self-disposing, modal | **routed this round** |
| Abandon (693) | self-disposing, hand-redirect | **routed this round (banish overrides)** |
| Counter Strike (510) | name only — "Counter Strike: prevent next damage"; no spell-counter effect at all | not a counter disposal — false positive, no action |

Nine real counter cards total. All nine now route every disposal through
`disposeCounteredSpell` (Abandon's non-flow hand-return path is the one
deliberate exception, and it is exactly what CR 829.1.b.1 does NOT touch —
a non-flow spell has no leave-the-chain override to apply).

## Tests — RED then GREEN

Three tests added to `tests/cards/test_flow.cpp`:
`FlurryOfFeathersCountersAFlowSpellToBanishment`,
`AbandonBanishesAFlowSpellInsteadOfReturningItToHand` (RED against the
pre-fix cards), and `AbandonStillReturnsANonFlowSpellToHand` (guard —
already green against the pre-fix code, since that path is unchanged; it
stays green after the fix too, proving the override is scoped to
`banish_on_leave` only).

RED verified by stashing just the two card files (`git stash push --
src/cards/spells/0606_flurry_of_feathers.cpp
src/cards/spells/0693_abandon.cpp`), rebuilding, and running:

```
$ RIFTBOUND_ROOT=.. ./riftbound_tests --gtest_filter='FlowTest.*'
[ RUN      ] FlowTest.FlurryOfFeathersCountersAFlowSpellToBanishment
test_flow.cpp:639: Failure  zone Which is: <04> (Trash) vs Banishment
test_flow.cpp:643: Failure  banishment.find != end() — not found
test_flow.cpp:645: Failure  trash.find != end() — found
[  FAILED  ]
[ RUN      ] FlowTest.AbandonBanishesAFlowSpellInsteadOfReturningItToHand
test_flow.cpp:674: Failure  zone Which is: <02> (Hand) vs Banishment
test_flow.cpp:678: Failure  banishment.find != end() — not found
test_flow.cpp:680: Failure  hand.find != end() — found
[  FAILED  ]
[ RUN      ] FlowTest.AbandonStillReturnsANonFlowSpellToHand
[       OK ] (guard passes unchanged, as expected pre-fix)
[  PASSED  ] 16 tests.  [  FAILED  ] 2 tests
```

`git stash pop` restored the fixes; rebuild; GREEN:

```
$ RIFTBOUND_ROOT=.. ./riftbound_tests --gtest_filter='FlowTest.*'
[==========] Running 18 tests from 1 test suite.
[  PASSED  ] 18 tests.
```

Re-ran the full counter-spell surface for regressions (107 tests:
`WindWallTest`, `NotSoFastTest`, `LullabyTest`, `DefyTest`, `AbandonTest`,
`RepulseTest`, `HardBargainTest`/`LegalTargetsFixture`, `JhinDeckTest`,
`AuditFix2Test` (Flurry of Feathers' own suite), `test_ivern_deck.cpp`'s
Flurry tests) — all green, no regressions from either card edit.

### Full suite (pre-commit)

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1087 tests from 118 test suites ran. (1874 ms total)
[  PASSED  ] 1087 tests.
  YOU HAVE 1 DISABLED TEST
```

1084 prior + 3 new (`FlurryOfFeathersCountersAFlowSpellToBanishment`,
`AbandonBanishesAFlowSpellInsteadOfReturningItToHand`,
`AbandonStillReturnsANonFlowSpellToHand`), 1 pre-existing disabled. No
regressions.

## Self-review

- Both flagged cards now route through `disposeCounteredSpell` (or, for
  Abandon's non-flow path, are explicitly and correctly left out of it). ✔
- Exhaustive second pass covers every `pop_back`/`erase` on `chain.items`
  in `src/cards` (5 sites, all routed) and every card whose text says
  "Counter" (9 cards, all accounted for, with a verdict for each). ✔
- No new behavior beyond the banish override: Flurry's non-counter mode
  (spawn 4 birds) and Abandon's Predict-1 side effect are untouched;
  `revertCounteredPlay` accounting is unchanged on both cards. ✔
- Tests assert real zones and real vector membership on both the RED and
  GREEN sides, plus a guard proving the non-flow path is byte-identical. ✔

## Concerns

1. Flurry of Feathers now sets `obj.location = std::nullopt` on counter
   (via the helper) where the pre-existing code did not — a latent
   inconsistency fix, not requested by name, but a direct and unavoidable
   consequence of "route through the helper." No test depended on the
   stale `location` value surviving a counter.
2. The `owner != PlayerId::None` defensive guard the old Flurry code had is
   gone (the helper has no equivalent). No sibling counter site
   (`counterChainTop`, Defy, Hard Bargain) had this guard either, and no
   test constructs a chain-item victim with `owner == None`, so this
   matches the established convention rather than diverging from it — but
   flagging it since it is a narrowing of defensive coverage on this one
   card, not something previously ruled on.
