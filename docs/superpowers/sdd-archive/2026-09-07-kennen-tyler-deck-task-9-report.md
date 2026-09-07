# Task 9 report — Sandswept Tomb: the rune discount and the target-restricted intent

Branch `kennen-tyler-deck`, base `42b10e1`.
Commits: `591300f` (mechanism + card + tomb tests), `4d8bb2a` (deck-validator #26).

Status: **DONE**, with one deliberate, documented under-application escalated
below (pair-pick spells get no restricted offer). No `// APPROX` markers were
added anywhere.

---

## What was implemented

### 1. The card — `src/cards/battlefields/0792_sandswept_tomb.cpp`

`BattlefieldCard`, def data verbatim from
`task-8-carddata.md` (def_id `ven-164-166`, VEN-164/166, collector 164, Kudos
Productions, Uncommon, the CDN image URL, verbatim ability text). Domains left
empty — the convention every other colourless battlefield uses (checked against
`0762_altar_of_blood.cpp`, which declares none either).

`applyPassiveAura` sets `friendly_spell_power_discount = 1` on the battlefield
whose `card_object_id` is this card — Altar of Blood's exact pattern.
Registered in `cards_init.cpp` after 791 (declaration at :797, call at :1591).

### 2. Aura reset — `GameEngine::recalculateAuras`

`bf.friendly_spell_power_discount = 0` added to the per-BF aura-flag reset loop
next to `death_recall_for_pay`, so the flag is re-asserted (or dropped) on every
recompute. Applied by step 1c, the battlefield-card pass — BF card objects have
no `location`, so the object loop above it never sees them.

### 3. Discount consumption — the cost sites

There are **two** places that compute a card's base cost, not three:

- `canAfford` (:5297 region) — `power_needed -= ps.transient_power_discount`,
  clamped at 0, placed **before** the ready/exhausted rune partition, because
  that partition only bothers computing `matches_domain` while power is still
  owed. The domain of any remaining power is untouched: a rune of the card's
  domain is simply not recycled.
- `beginCostPayment` — the same two lines, at the point the energy discount is
  applied, so the cursor is initialised with the discounted `power_remaining`.
- `payCardCost` is a **bridge**: its whole body is
  `beginCostPayment` + the `queryAgent` loop over `resolveCostPaymentDecision`.
  It has no independent cost computation, so it is covered by the
  `beginCostPayment` change. (The brief's "~:5096-5115" region is
  `payRepeatCost`, which prices [Repeat] tranches — an *additional* cost paid on
  top of the spell's cost, not the spell's cost. Deliberately **not**
  discounted; `payAdditionalCost` likewise.)

Test #24 and the flow test both assert the real rune deltas (recycled count,
`rune_deck.size()`, exhausted/ready counts), so a discount that reached the
offer but not the payment is caught.

### 4. Staging — `executePlaySpell`

A single `tomb_power_discount` is computed at the very top, before anything
mutates and before the Flow validation block reads it:

- restricted intent → read straight off the named battlefield's flag. A
  hand-built intent claiming a restriction on a battlefield with no Tomb reads
  0, so no discount can be invented;
- otherwise → `tombDiscountForTargets(state, player, intent.targets)`, i.e.
  whatever the play's already-chosen play-time targets earn.

Then:

- **flow path** — `flow_cost.power` is reduced by it on the local copy, right
  after the offer is matched and before the payability check, so the
  `canPayAdditionalCost` probe, the `payAdditionalCost` charge and the trace
  line all agree. No staging field is involved here (that path prices through
  explicit arguments), so there is nothing to leak on the early-return
  rejection paths above it.
- **normal path** — `ps.transient_power_discount = tomb_power_discount;`
  immediately before `payCardCost`, cleared immediately after, in the same
  straight-line block as the existing Irelia energy staging. No branch or early
  return sits between the two.
- **trash-replay-grant path** — untouched. `generateTrashReplayActions` emits no
  restricted intents, so no discount is ever offered or charged there;
  offer and payment stay consistent.

The chain item gets `item.target_battlefield_restriction =
intent.target_battlefield_restriction` in the existing by-id lookup that sets
`banish_on_leave`.

### 5. The offer — both generators

`generateSpellActions` (hand) and `generateFlowPlayActions` (trash) each:

- **moved the affordability gate after the legal targets are known** and priced
  it at `bestTombDiscount(...)`, an upper bound on what any single offer for
  that card could earn. A single up-front `canAfford` at the printed price
  would have dropped offers that are legal at *their own* discounted price
  (test #23b, the flow test and the Last Stand test all fail without this).
- **re-price every emission** with the discount it actually earns — in
  `generateSpellActions` inside the existing `emit` lambda, in
  `generateFlowPlayActions` inside a new `emitPlay`. With no Tomb on the board
  the discount is 0 and the check is byte-for-byte the old one, so the change
  is invisible on an ordinary board (asserted by `NoRestrictedOfferWithoutATomb`
  and by the 1105 pre-existing tests).
- **emit the restricted intent** for a resolve-time-target spell with
  `power > 0`: one extra intent per flagged battlefield where the player has at
  least one legal friendly-unit target, carrying
  `target_battlefield_restriction` and checked for affordability at the
  discounted power.

The hand path stages the discount around `canAfford` via an RAII
`StagedPowerDiscount`; the flow path passes the reduced power to
`canPayAdditionalCost` explicitly.

### 6. The resolve-time picker — `Card::pickTarget`

One filter, in the one place every resolve-time single-target pick goes
through. When the resuming chain item carries a restriction, the legal list is
narrowed to **friendly units at that battlefield** (unit + controller +
location, all three) before the choices are published. If nothing survives, the
pre-existing "no legal targets at resolve time" branch handles it exactly like a
target that vanished — which is what the spec asks for. No individual card was
touched.

### 7. Test #26 — `tests/test_deck_validator.cpp`

`KennenTylerDeckLoadsAndValidates`: `decks/kennen_tyler.txt` loads through
`DeckValidator::loadFromDeckList` and validates with zero errors, modelled on
`LoadFromDeckListAndValidate` but with a hard `ASSERT_TRUE(exists)` instead of a
skip — the file is the deliverable, so a missing file must fail, not pass
quietly.

---

## TDD record — red before green, every test

| # | Test | RED evidence | GREEN |
|---|------|--------------|-------|
| 22 | `AuraSetsDiscountFlagOnItsOwnBattlefieldOnly` | `C++ exception "Card ID not found: 792"` — card did not exist | after the card file + registration + aura reset |
| 23a | `RideTheWindOfferedTwiceWithFriendlyUnitAtTheTomb` | `offers.size()` 1, expected 2 | after the restricted emission |
| 23b | `RestrictedIntentIsAffordableWithOneFewerMatchingRune` | `offers.size()` 0, expected 1 | after `canAfford` honours the discount + the moved gate |
| 24 | `RestrictedPlayRecyclesOneFewerRuneAndPicksOnlyAtTheTomb` | `rune_deck.size()` 1 (expected 0), `countExhausted` 1 (expected 2), `chain_restriction.has_value()` false | after `executePlaySpell` staging + chain stamp + `pickTarget` filter |
| 24b | `UnrestrictedPlayStillPaysThePowerAndSeesEveryTarget` | (control — passed from the start; it is the "nothing regressed" half of #24) | — |
| 25 | `OfferedOnceWhenNoFriendlyUnitIsAtTheTomb` | proven red by deleting the `hasFriendlyUnitTargetAt` eligibility check: 2 offers, expected 1; check restored, green | ✅ |
| 25b | `NoRestrictedOfferWithoutATomb` | (guard: the mechanism must be invisible with no Tomb) | ✅ |
| flow | `SandsweptTombFlowTest.RestrictedFlowIntentIsOfferedAndPaysOneLessPower` | with the restricted-flow emission removed: `offers.size()` 0, expected 1 | after `generateFlowPlayActions` emits it and `executePlaySpell` reduces the flow power |
| opp | `OpponentOfTheTombsControllerGetsTheDiscountToo` | proven red by scoping the discount to the battlefield's controller (`if (bf.controller && *bf.controller != player) continue;`): 0 offers, expected 1; scoping removed, green | ✅ |
| play-time | `PlayTimeTargetOfferIsPricedWithTheDiscountItEarns` (Last Stand, 69) | proven red by dropping the `StagedPowerDiscount` from `emit`: 0 offers, expected 1; restored, green | ✅ |
| 26 | `DeckValidatorTest.KennenTylerDeckLoadsAndValidates` | proven red by commenting out `register_card_792`; restored, green | ✅ |

Every temporary red-proof edit was reverted from a byte-identical backup and the
suite re-run; `grep -c "TEMP RED PROOF"` over the tree returns 0.

## Suite and coverage

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1116 tests from 121 test suites ran.
[  PASSED  ] 1116 tests.
  YOU HAVE 1 DISABLED TEST
```

1105 at HEAD + 10 Tomb tests + #26 = 1116; the 1 disabled test is pre-existing.

```
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

792 total, INCOMPLETE 0, engine-gap unchanged at 50 (the Tomb is not flagged).

## Files touched

- `src/cards/battlefields/0792_sandswept_tomb.cpp` (new)
- `src/cards/cards_init.cpp`
- `src/engine/game_engine.cpp`
- `src/cards/card.cpp`
- `tests/cards/test_sandswept_tomb.cpp` (new)
- `tests/test_deck_validator.cpp`

No headers were modified.

---

## Self-review against the checklist

- **Discount consumed at all three cost sites** — `canAfford` ✅,
  `beginCostPayment` ✅, `payCardCost` ✅ *by delegation* (it is a bridge over
  `beginCostPayment`; it computes no cost of its own — verified by reading the
  whole function). The flow cost is discounted at its own site because Flow
  replaces the base cost. Asserted end-to-end by real rune deltas in #24 and
  the flow test.
- **Restricted intent in BOTH generators** — ✅ hand (`generateSpellActions`)
  and trash (`generateFlowPlayActions`), each with its own affordability
  pricing. Both are covered by tests.
- **Picker filter in one place** — ✅ `Card::pickTarget` only; no card file was
  touched.
- **Staging cleared on every exit path** — ✅. The generators use an RAII guard
  that restores the previous value in its destructor (so an early `continue` or
  `return` cannot leak it). `executePlaySpell` sets and clears in one
  straight-line block with nothing between. The flow path never stages at all
  — it reduces a local `flow_cost.power` — so its four early-return rejection
  paths have nothing to clear.
- **No `// APPROX`** — ✅ (the one hit in `game_engine.cpp` is the pre-existing
  Irelia comment at :1672, untouched).
- **Tests assert real rune deltas and offered-intent shapes** — ✅: recycled
  count via `rune_deck.size()`, exhausted/ready counts, offer counts, the
  presence/absence and value of `target_battlefield_restriction`,
  `flow_source`/`play_source`, and the exact contents of the resolve-time
  target prompt (captured by a recording agent).

---

## Concerns for the controller

1. **Pair-pick spells get no restricted offer — a deliberate under-application,
   and the one place this is not fully faithful.** Star-Crossed (690, power 1)
   and Switcheroo (466, power 2) choose a friendly unit at resolve through
   `Card::pickTargetPair`, so by the card text they *should* be discountable at
   the Tomb (addendum #9 lists them as affected). I did not offer them a
   restricted intent, because `pickTargetPair` has no restriction filter and
   more than twenty cards call it, each building its two lists differently
   (list A is the friendly one for Star-Crossed and Flash, but not necessarily
   for Bone Skewer / Clash of Giants / Gentlemen's Duel). Every generic filter I
   could write either breaks some of those cards or lets a pair that chooses no
   friendly unit at the Tomb keep the discount — i.e. an **unearned** discount,
   a rules break in the illegal direction. Excluding them is the sound choice:
   the affected spells simply pay full price, exactly as they do today. It is
   documented in a comment at the emission site, not marked APPROX, and I am
   flagging it here rather than silently shipping it. If you want it closed, the
   shape I would propose is a `pickTargetPair` filter that narrows list A to
   friendly-units-at-BF when A contains one and otherwise wraps `legal_b_fn`
   with the same narrowing — plus a per-caller audit of the twenty-odd cards.
   That is a task of its own, not a fold-in.

2. **Multi-Tomb stacking is generous for play-time targets and conservative for
   restricted ones.** `tombDiscountForTargets` sums over *distinct* flagged
   battlefields, so a play-time-target spell choosing friendly units at two
   Tombs gets [A][A] off — which is what two independent "costs [A] less"
   abilities say. The restricted intent names one battlefield and therefore
   grants one Tomb's worth even if a second could theoretically apply. That
   asymmetry is inherent in the ratified restricted-intent design (addendum #5),
   it errs against the player, and there is no second Tomb in either deck in
   scope.

3. **`const_cast` in the generators.** `canAfford` is `const` and the staging
   field lives on `PlayerState`, so `StagedPowerDiscount` writes through a
   `const_cast<GameState&>` and restores in its destructor. The engine object is
   never actually const (only these query methods are), nothing runs between the
   set and the restore, and the alternative — a discount parameter on
   `canAfford` — would have required a header edit, which the brief forbids. It
   is the one thing in this change I would want a second pair of eyes on; if you
   prefer the header edit, the guard collapses to a defaulted argument.

4. **Pre-existing, not introduced: `pickTarget` does not validate the agent's
   answer against the published list.** At `resume_point == 7` the picked id is
   taken verbatim, so a hand-built agent could name a target that was never
   offered — including one outside the restricted battlefield, after paying the
   discount. This is engine-wide behaviour for every card that picks a target,
   not specific to the Tomb, and hardening it would change resolution for all of
   them. Worth a separate look.

5. **Cost of the offer, as ratified.** One extra legal action per affected
   resolve-time-target spell per Tomb with an eligible friendly unit — addendum
   #5's accepted cost. In the Tyler deck that is Ride the Wind (3 copies) and
   the single Sandswept Tomb, so at most one extra action per Ride the Wind in
   hand.

---

# Fix round 1 — review response

Commit `b33a1ad` "Kennen: Tomb discount re-validates the restriction; pair-pick
spells covered". Files: `src/engine/game_engine.cpp`, `src/cards/card.cpp`,
`tests/cards/test_sandswept_tomb.cpp`. No headers touched.

## Important #1 — the restriction is re-validated before payment

`executePlaySpell` took the discount from the battlefield flag alone. It now
treats `target_battlefield_restriction` the way it already treats
`flow_source`: as a claim to be re-earned against live state, at the top of the
function before anything mutates. It re-enumerates the spell's
requirement-level legal targets and requires both `flag > 0` and
`hasFriendlyUnitTargetAt(state_, intent.player, legal, *restriction)`; on
failure it emits `logWarn("TOMB: illegal restricted intent for <card> — no
eligible friendly unit at battlefield N")` and returns, mutating nothing.

The legal list used is deliberately the same requirement-level superset the
generators gate the offer on, so offer, payment and picker cannot disagree
about who is eligible.

**RED** — `HandBuiltRestrictionWithNoEligibleUnitIsRejected`: a plain Ride the
Wind offer, hand-edited to claim `target_battlefield_restriction = 0` on a Tomb
with the only friendly unit at the other battlefield.
`Value of: played.empty() / Actual: false`, plus exhausted 2 (expected 0).
**GREEN** after the re-validation; the test also asserts 3 ready runes, 0
exhausted, empty `rune_deck`, the spell still in hand and no target prompt.

Its counterpart, `RestrictedPlayKeepsItsDiscountWhenTheTombUnitDiesFirst`
(minor 3a), pins the behaviour the fix must NOT become: a *legitimate*
restricted play whose Tomb unit dies between payment and resolution keeps the
discount and fizzles (CR — the cost is locked at play). It passed on
introduction, as a characterization test should; it was proven to bite by
disabling the `pickTarget` filter (`legal_p = &restricted` commented out), which
turns three of its expectations red.

## Important #2 — pair-pick spells covered (controller ruling)

`Card::pickTargetPair` now enforces the commitment with the two-branch rule,
implemented exactly as ruled and with no per-card knowledge:

- **A step** — keep `a` when `a` is a friendly unit at the restricted
  battlefield, or when some `b ∈ legal_b_fn(a)` is (`legal_b_fn` is already
  available at that point). Everything from which the commitment can never be
  met is dropped.
- **B step** — if the chosen A already satisfies the commitment, B is untouched
  (the enemy half must not be forced to stand at the Tomb); otherwise B is
  narrowed to friendly units there.

Both generators then dropped the `!needsPlayTimeTargetPair()` exclusion;
eligibility for the restricted offer is unchanged ("some legal target is a
friendly unit at the Tomb"), which is the conservative equivalent the ruling
allows: it is the superset of whatever A/B lists the card builds, it is what
`executePlaySpell` now re-checks, and in the residual case where a card's own
lists cannot reach the eligible unit the pair filter empties and the play
fizzles at its own expense — never an unearned advantage.

Three tests, red first:

| Test | RED | GREEN |
|---|---|---|
| `StarCrossedRestrictedOfferNarrowsTheFriendlyList` — 3 Order runes so only the discounted play is payable | `offers.size()` 0, expected 1 | restricted offer emitted; nothing recycled for the [1] power; A prompt narrowed to the Tomb friendly; B prompt left as the enemy; both bounced |
| `SwitcherooGetsNoRestrictedOfferWithOnlyEnemiesAtTheTomb` | passed on introduction (no pair offers existed yet); proven to bite by dropping the controller check from `hasFriendlyUnitTargetAt` → 2 offers, expected 1 | 1 unrestricted offer |
| `SwitcherooRestrictedPlayNarrowsTheSecondPickWhenTheFirstIsEnemy` — two enemies + one friendly at the Tomb, one of each elsewhere | `unit_prompts.size()` 1, expected 2 | A list = {both Tomb enemies, Tomb friendly}, far units dropped; after picking an enemy A, B = {Tomb friendly} only; exactly one of Switcheroo's [2] power recycled |

The recording agent gained a `prefer` list so the Switcheroo test can drive the
A pick down the enemy branch deterministically.

## Folded minors

- **(4)** `emit` (hand) and `emitPlay` (flow) skip the per-offer affordability
  re-check when `tomb_best` / `flow_tomb_best` is 0. Provably identical: a zero
  best means no target drawn from the legal list is a friendly unit at a
  flagged battlefield, so every per-offer discount is 0 and the up-front gate —
  which already passed — computed the same answer. Restores the pre-change cost
  of the MCTS hot path.
- **(5)** `StagedPowerDiscount` always writes, zero included, and always
  restores; the constructor no longer early-returns. A state cloned mid-payment
  carries a live non-zero staging value, and the old guard would have let it
  leak into every undiscounted offer generated from that clone.
- **(7)** One-line comment at the trash-replay-grant payment site: those flat
  override costs are intentionally undiscounted, `generateTrashReplayActions`
  emits no restricted offers, and neither deck in scope contains such a grant.
- **(9)** `RestrictedIntentIsAffordableWithOneFewerMatchingRune` →
  `RestrictedIntentIsTheOnlyOfferWhenOnlyTheDiscountedCostIsPayable`.

Parked items (OpenSpiel action-id aliasing, the duplicated emission loop, file
size) were left alone as instructed.

## Verification

```
$ cd /home/user/chorlick/alpharune && RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1121 tests from 121 test suites ran. (1939 ms total)
[  PASSED  ] 1121 tests.

  YOU HAVE 1 DISABLED TEST
```

1116 before this round + 5 new = 1121; the 1 disabled test is pre-existing.
`--gtest_filter='SandsweptTomb*'` → 15 passed.

```
$ python3 scripts/card_coverage.py
TOTAL                                          792
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

`grep -c "TEMP RED PROOF"` over all three files returns 0 — every mutation used
to record a RED was reverted from a byte-identical backup and the suite re-run.

## Remaining concerns after this round

The pair-pick gap is closed, so concern 1 from the original report is
withdrawn. Concerns 2 (multi-Tomb asymmetry — the restricted intent names one
battlefield), 3 (`const_cast` in the const generators) and 4 (pre-existing:
`pickTarget`/`pickTargetPair` accept the agent's answer without checking it
against the published list — now the only remaining way to dodge a paid-for
commitment, and engine-wide rather than Tomb-specific) stand unchanged.
