# Review: L0b equip legality (b406465..b1f2a89)

**Verdict: Changes required**

Registry-guard/generator/Last-Rites/Soul-Sword tests are real and green,
and the burst-detector Python is solid. But the equip-cost predicates
diverge from the engine's own canonical cost payer, and the generator gate
doesn't cover target-dependent gear — both are live, not theoretical.

## 1. [HIGH] Same rune pays BOTH energy and domain power — double-spend

`src/cards/gear/equip_base.h:37-128` (`scanStandardEquip`/`canStandardEquip`/
`standardEquip`) and `:65-76,162-198` (`canUniversalEquip`/
`UniversalEquipGear::onEquip`), plus the hand-rolled duplicate in
`src/cards/gear/0748_hextech_gauntlets.cpp:24-89`.

`domain_rune` (or `any_rune`) is picked from the pre-payment scan with no
regard for which runes the energy loop will exhaust, and the energy loop
doesn't exclude it. Compare the engine's own canonical additional-cost
payer, `GameEngine::payAdditionalCost` (`src/engine/game_engine.cpp:6172`,
`canPayAdditionalCost` at `:6126`): it explicitly puts recycled runes in a
`recycled` set and skips them when exhausting for energy — one physical
rune cannot pay both halves of one cost there.

Concrete: Boneshiver (`#439`, `SimpleEquipGear(Domain::Body, 1)`, i.e.
"[1][D]") with exactly **one** ready Body rune in base and nothing else.
`canStandardEquip`: `ready_runes=1`, `domain_rune=`that rune,
`payable(1)` → true. `standardEquip` exhausts that same rune for the `[1]`
energy, then recycles it (now exhausted) for the `[D]` power — a full
equip paid off a single rune. Run the same shape through
`canPayAdditionalCost(energy=1, power=1, Body)`: `matching_ready=1`,
`exhausted_matching=0`, `recycle_from_ready=1`, `energy_available =
1-1=0 < e_need(1)` → **false**. The two payers disagree on the identical
cost shape. Also live: Skyfall of Areion (353), Blighted Battleaxe (581),
Svellsongur (382), The Zero Drive (412), Hextech Gauntlets (748, e.g.
Might-2 unit + one ready rune: energy=1, `[A]`=1, same collision). Latent
in `UniversalEquipGear` (Spinning Axe/Forgefire Cape/Shurelya's Requiem
are all `energy_cost=0` today, so dormant).

Minimal fix: exclude the chosen domain/universal rune from the ready-rune
count used for energy (mirror `payAdditionalCost`'s `recycled` exclusion),
in `scanStandardEquip`, `canUniversalEquip`, and Hextech Gauntlets' local
scan. Also make `domain_rune`/`any_rune` selection prefer an already-
exhausted rune (like `payOnePower`/`findPowerRune` already does) so the
common case doesn't even reach the collision.

## 2. [HIGH] Target-dependent gear can still be offered-then-rejected forever

`src/engine/game_engine.cpp:2856` gates the generator by gear-level
(target-agnostic) `canEquip`, but **no gear overrides
`needsEquipTimeTarget()`** (confirmed: zero hits outside the default in
`card.h:483`) — every equip goes through the legacy per-unit enumeration
at `:2866-2878`, which emits one Intent per friendly unit with no
per-target affordability check.

Concrete: Hextech Gauntlets (748) on a board with two friendly units, A
(Might 3, energy 0) and B (Might 0, energy 3), and exactly one ready rune.
Gear-level `canEquip` is true (via A, the cheapest). The generator still
emits "equip 748 to B". `onEquip(B)`'s target-specific check
(`ready_count(1) < energy_cost(3)`) fails, `executeIntent` now logs the
new warning instead of silently dropping it — but the Intent targeting B
is unchanged and still legal on the next decision, so the agent can
re-pick "equip to B" every turn, reproducing the exact frozen-burst shape
(iteration 0's 497-repeat Last Rites) this commit set out to kill, just
audibly. `Generator_UnpayableEquipIsNotOfferedButPayableOneIs` only tests
a single-unit board, so this is untested.

Minimal fix: give Hextech Gauntlets `needsEquipTimeTarget()=true` (resolve
the unit via `pickTarget` inside `onEquip`, as the comment at
`game_engine.cpp:2836` already anticipates for exactly this shape), or add
a target-aware `canEquip` overload the legacy per-unit loop calls before
emitting each Intent.

## 3. [MEDIUM] Blade of the Ruined King kills before it knows the power is payable

`src/cards/gear/0498_blade_of_the_ruined_king.cpp:48-60`. `canEquip` is
rechecked at entry (`:36`), but `pickTarget` (`:49`) is a documented
suspend point ("re-entry will resume" — `card.h:281`), and `killObject`
(`:56`) fires death triggers/reactions before `payOnePower` (`:60`) runs.
CR 164.2.b's rune-recycle is itself a `[Reaction]`, so a reaction opened by
the kill can spend the same Order rune `canEquip` counted. If it does,
`payOnePower` fails at `:60` and `onEquip` returns false — with the
friendly unit already dead and nothing paid. Minimal fix: pay
`payOnePower` (no side effects of its own) before `killObject`, i.e. swap
the two commit blocks so the irreversible action is last.

## 4. [LOW/NOTE] Frozen-burst fingerprint is blind to reaction/priority depth

`scripts/analyze_batch.py` `_state_fingerprint` (actor, chosen, scores,
players, battlefields, phase, legal_count) has no field tracking
chain/priority-stack depth — nothing in the decision schema
(`is_closed`/`phase_name`/`get_decisions_list`) carries one either. A
legitimate long run of same-actor "pass" decisions through a stack of
reactions that individually don't move scores/battlefields would be
indistinguishable from the real no-op bug this catches, at the same
threshold (10) that flags real bugs. Boundary logic itself is correct
(verified: first/last decision, exact-threshold run, and two adjacent
distinct bursts all handled right in `find_frozen_bursts`). Not a code
defect, but "treat as engine evidence until fixed" overclaims certainty
until this is checked against a batch with a real long reaction chain, or
a stack-depth field is added to the fingerprint.

## 6. House rules — clean

Only the listed files changed; no card-name logic added to the engine
(one generic `Card::canEquip` virtual); commit trailers present on both
commits; no AI-model identifiers anywhere in the diff.
