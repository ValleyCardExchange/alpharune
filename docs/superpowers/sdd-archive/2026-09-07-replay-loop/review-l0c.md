# Review: L0c equip-legality/rune-double-spend/activation-status (591967a..192e9e1)

**Verdict: Approved with notes**

All three L0b findings are fixed correctly and agree with the engine's own
canonical payer. Two of the implementer's seven concerns need a closer look
(below); neither blocks this commit.

## Verified correct (no issues found)

- **Equip scan/payment** (`equip_base.h:33-136`): `findPowerRune` picks the
  power rune first (prefer exhausted), `scanEquipCost` excludes that exact
  id from `energy_runes`, `payEquipCost` skips it when exhausting and always
  recycles it. `standardEquip` / `UniversalEquipGear::onEquip` build one
  `scan` and pass the same object to `payable()` and `payEquipCost` — check
  and payment can't diverge. Universal variant's `power_rune valid &&
  energy_runes >= energy_cost` is `max(1,energy)` distinct runes, correctly.
  Hextech Gauntlets (`0748_hextech_gauntlets.cpp:29-58`) reuses the same
  pair with no state change between `canEquipTarget` and `onEquip`.
- **`executeIntent` bool** (`game_engine.cpp:1169-1437`): every reject
  (`:1250,1275,1318,1339,1433`) sets `executed=false` before any mutation —
  confirmed the energy pre-check (`:1331`) runs before exhaust/disempower/
  `addAbility` (`:1342+`). Floating-pool-first payment (`:1354-1369`)
  matches `availableEnergy` (`:5525`, pool+ready runes) and
  `payAdditionalCost` (`:6181`, same order) — no double-spend, since
  `availableEnergy >= act_cost.energy` guarantees `readyRunes >=` whatever
  the pool didn't cover.
- **Chain consumer** (`chain_manager.cpp:378-418`): false → `logWarn` +
  `continue` at the same `current`, pass set untouched; true → `return true`
  → `processFEPR` re-enters `stepExecuteAndPass`, which clears
  `players_passed_priority` on its own entry (`:204`). Only an executed
  activation restarts FEPR. Matches tests (6)/(7).
- **Blade of the Ruined King** (`0498...cpp:58-68`): power paid before
  `killObject`. Report's claim that the plan's literal "drain between check
  and pay via a hook" scenario is unreachable is right — re-entry after a
  `pickTarget` suspend re-runs `canEquip` from the top either order; the
  real window is inside `killObject`'s death event, which the new order
  closes and test (11) exercises correctly.
- **`numberOrThrow`**: all 15 `.get<double>()` reads in `agent_spec.cpp`
  route through it; `is_number()` correctly excludes bool.
- House rules clean: only the 11 listed files touched, no card-name logic,
  trailers present, no model identifiers.

## [MEDIUM] Concern #3's "no shipped ability uses power costs" is false

`game_engine.cpp:3796-3805` (generator) and `:1314-1388` (`executeIntent`
payment) both check exhaust/disempower/energy/discard/xp — neither ever
reads `act_cost.power`/`power_domain`. The report calls this pre-existing
and dormant; it is pre-existing but **not dormant**: `ActivationCost{.power=
1, ...}` is shipped on Treasure Trove (186, "[P],[E]: Kill this"), Assembly
Rig (342), Azir Ascendant (373), Ezreal Dashing (404), Xerath Freed (588).
Concretely, with **zero** Chaos power available, the generator still offers
Treasure Trove's activation and `executeIntent` executes it paying nothing
for the `[P]`. Correctly out of L0c's stated scope (spec addendum #17 named
only disempower/energy) — not a blocker — but the "no shipped ability"
framing understates it; file a follow-up before self-play exercises these
five cards. Fix: add a `power`/`power_domain` gate to both sites, mirroring
the energy gate this commit just added.

## [LOW] Concern #4 is observable, not just latent

`payEquipCost` (`equip_base.h:131`) and `payOnePower` (`card_helpers.h:42`)
recycle a rune that may be exhausted (preferred) without clearing
`is_exhausted` — unlike `payAdditionalCost`, which does
(`game_engine.cpp:6228`). `GameEngine::channelRunes` (`:5492-5509`) moves a
rune from `rune_deck` to base copying only `zone`/`location`, never resets
`is_exhausted`, despite logging `"(ready)"`. Channel Phase runs after that
turn's own Awaken ready step (`:712-740`, readies only the current turn
player's objects), so a rune recycled exhausted → reshuffled → channeled
back enters base still exhausted and stays unusable until this player's
*next* Awaken Phase (the rest of this turn + the opponent's whole turn).
Pre-existing, but this diff widens where prefer-exhausted applies, so it
fires more often now. Fix: `is_exhausted = false` before the `rune_deck`
push, in both functions, matching `payAdditionalCost`.

## Everything else in the report's 7 concerns

#1, #2, #5, #6, #7: judgment calls checked against the diff and code shape
— all sound, no action needed.
