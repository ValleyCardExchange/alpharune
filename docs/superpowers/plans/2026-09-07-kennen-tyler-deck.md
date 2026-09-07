# Kennen (Heart of the Tempest) deck support — implementation plan

> **For agentic workers:** execute task-by-task with strict TDD (write the
> failing test, watch it fail for the right reason, write the minimum,
> watch it pass, run the full suite, commit). Steps use `- [ ]` checkboxes.
> House rule from Tyler's CLAUDE.md overrides the superpowers template:
> **this plan carries NO literal code** — it names files, interfaces,
> behaviours and test cases; code is written against the checkout.

**Goal:** alpharune plays `decks/kennen_tyler.txt` vs `decks/rengar_test.txt`
with full-fidelity Empower, Flow, the non-hand-play trigger, Sandswept Tomb,
Burn N, and the five VEN cards.

**Architecture:** two new engine mechanics (Empower status + Flow alternate
cost) threaded through the existing action-generator → executor → chain
disposal pipeline; one new trigger dispatched from the card-played event;
one new power-discount staging path mirroring the existing energy one; five
card classes following the repo's per-card pattern.

**Tech stack:** C++20, CMake+Ninja (build dir `build/`), GoogleTest via
`RIFTBOUND_ROOT=. ./build/riftbound_tests`, Python coverage gate
`scripts/card_coverage.py`.

**Spec:** `docs/superpowers/specs/2026-09-07-kennen-tyler-deck-design.md`
(read it first; its dated addendum overrides inline text where marked).

## Global constraints

- Environment verified 2026-09-07 before this plan: full build `BUILD_OK`
  (0 errors), suite **1057 passed**, smoke game random vs mcts:sims=50 ran
  to game-over with an HTML replay.
- Header edits to `types.h`, `game_state.h`, `game_object.h`, `card.h`,
  `effect_types.h`, `intent.h`, `events.h` trigger a near-full rebuild
  (~35 min on 4 cores). **Batch every header change into Task 1** so the
  rest of the plan is fast incremental builds.
- Build only the test target during the loop: `cmake --build build --target
  riftbound_tests`; build `riftbound` once at the end for the smoke run.
- Run a single new test file with `--gtest_filter=<Suite>.*` while
  iterating; the FULL suite before every commit.
- Every new test is watched failing first. A test that passes on first run
  is rewritten until it fails against the pre-change code.
- Never weaken an existing test or the coverage gate. Never add
  card-specific `if (name == …)` logic in the engine.
- Commit per task, small, message prefix `Kennen:`; author = Tyler's git
  identity as used for the spec commits; trailer as on those commits.
- Nothing from this work is copied into the personal-ai repo.

## File map

Create:
- `src/cards/legends/0788_heart_of_the_tempest.cpp`
- `src/cards/units/0789_kennen_storm_of_shuriken.cpp`
- `src/cards/spells/0790_lightning_rush.cpp`
- `src/cards/spells/0791_up_from_the_deep.cpp`
- `src/cards/battlefields/0792_sandswept_tomb.cpp`
- `tests/cards/test_empower.cpp`, `tests/cards/test_play_from_non_hand.cpp`,
  `tests/cards/test_flow.cpp`, `tests/cards/test_kennen_cards.cpp`,
  `tests/cards/test_sandswept_tomb.cpp` (test globbing is automatic —
  `CMakeLists.txt:344-350`; re-run cmake configure once so the glob picks
  up new files).

Modify (headers, Task 1 only):
- `src/core/types.h` — `Keyword::Flow`, `Count`; `src/core/types.cpp` —
  `toString`.
- `src/core/game_object.h` — `is_empowered`; `granted_flow` optional.
- `src/core/game_state.h` — `BattlefieldState::friendly_spell_power_discount`;
  `PlayerState::transient_power_discount` (+ reset in `resetTurnTracking`);
  `ChainItem::banish_on_leave`, `ChainItem::target_battlefield_restriction`.
- `src/core/intent.h` — `Intent::flow_source` enum {None, Printed, Granted};
  `Intent::target_battlefield_restriction` (optional BattlefieldId).
- `src/core/events.h` — `CardPlayedEvent::play_source`;
  `ObjectEmpoweredEvent`.
- `src/effects/effect_types.h` — `TriggerType::WhenYouPlayFromNonHand`;
  `ActivationCost::disempower_self`.
- `src/core/card_db.h` — `CardDef::flow_energy / flow_power /
  flow_any_domain`.
- `src/cards/card.h` — `Card::flowCost()` returning `FlowCost`; struct
  `FlowCost{valid, energy, power, power_domain, any_domain}`.
- `src/engine/effect_executor.h` — `empowerObject`, `disempowerObject`,
  `burnCards`, `burnOut`, `revealAndChoose(player, count, rest_destination)`.
- `src/engine/game_engine.h` — `liveFlowCosts`, `generateFlowPlayActions`,
  `playSourceFor`.

Modify (.cpp, later tasks):
- `src/engine/effect_executor.cpp`, `src/engine/game_engine.cpp`,
  `src/engine/chain_manager.cpp`, `src/engine/trigger_manager.cpp`,
  `src/cards/card_helpers.h` (header-only helpers, inline; counts as a
  header for rebuild purposes — do its edit in Task 1 too),
  `src/io/state_renderer.cpp`, `src/cards/cards_init.cpp`,
  `decks/kennen_tyler.txt`, `tests/test_deck_validator.cpp`,
  `tests/test_types.cpp`.

---

### Task 1: Header scaffolding (one rebuild) + keyword guard test

**Files:** every header in the file map; `src/core/types.cpp`;
`src/cards/card_helpers.h`; `tests/test_types.cpp`.

**Interfaces produced (exact names later tasks rely on):**
- `Keyword::Flow` (bit 23), `Keyword::Count == 24`, `toString(Keyword::Flow)
  == "Flow"`.
- `GameObject::is_empowered` (bool); `GameObject::granted_flow`
  (`std::optional<GrantedFlow>` with `energy`, `power`, `power_domain`,
  `any_domain`, `valid_on_turn`).
- `ActivationCost::disempower_self` (bool).
- `TriggerType::WhenYouPlayFromNonHand`.
- `CardPlayedEvent::play_source` (`Intent::PlaySource`, default Hand);
  `ObjectEmpoweredEvent{object, controller}`.
- `Intent::FlowSource {None, Printed, Granted}`; `Intent::flow_source`;
  `Intent::target_battlefield_restriction` (`std::optional<BattlefieldId>`).
- `ChainItem::banish_on_leave` (bool); `ChainItem::target_battlefield_restriction`.
- `CardDef::flow_energy`, `flow_power`, `flow_any_domain`; `Card::FlowCost`;
  `virtual FlowCost Card::flowCost() const` (default: from def, valid only
  when `def().keywords.has(Keyword::Flow)`; `power_domain` = first domain).
- `BattlefieldState::friendly_spell_power_discount` (int);
  `PlayerState::transient_power_discount` (int, reset per turn).
- Executor declarations: `empowerObject(GameObjectId)`,
  `disempowerObject(GameObjectId)`, `burnCards(PlayerId, int)`,
  `burnOut(PlayerId)`, `revealAndChoose(PlayerId, int, RestDestination)`
  with `enum class RestDestination {Recycle, Trash}` and the existing
  two-argument call sites still compiling (default `Recycle`).
- Engine declarations: `std::vector<FlowOffer> liveFlowCosts(GameObjectId)`
  where `FlowOffer{Intent::FlowSource source; Card::FlowCost cost;}`;
  `generateFlowPlayActions(PlayerId, bool action_ok, bool reaction_ok,
  std::vector<Intent>&) const`; `Intent::PlaySource playSourceFor(const
  GameObject&) const`.
- `card_helpers.h`: `counterChainTop` / `revertCounteredPlay` branch on
  `banish_on_leave` — declaration only compiles now; the behaviour is
  implemented in Task 5 (it is header-only, so the branch body lands here
  but stays unexercised until Task 5's test).

- [ ] Step 1: In `tests/test_types.cpp`, add test **#27**: `toString` of
  every `Keyword` value below `Count` is non-empty and unique, and
  `toString(Keyword::Flow)` is "Flow". (Before the enum change, `Flow`
  does not compile — so the RED here is a compile failure naming the
  missing enumerator; record it.)
- [ ] Step 2: Build the test target; confirm the failure is "no member
  named Flow".
- [ ] Step 3: Make every header change listed above (declarations only;
  executor/engine bodies may be empty stubs that compile).
- [ ] Step 4: Build (full rebuild — start it and continue reading the
  spec while it runs). Run the suite: 1057 + 1 pass.
- [ ] Step 5: Commit `Kennen: header scaffolding for Empower, Flow,
  non-hand trigger, Tomb discount`.

### Task 2: Empower / Disempower actions and the disempower cost

**Files:** `src/engine/effect_executor.cpp`, `src/engine/game_engine.cpp`
(three offer gates at the `ab.cost.exhaust && obj.is_exhausted` checks —
`generateActivateAbilityActions`, the showdown [Action] site, the reaction
site — plus the payment block in `executeActivateAbility` after the exhaust
component), `src/io/state_renderer.cpp`; test `tests/cards/test_empower.cpp`.

**Interfaces consumed:** Task 1 declarations. **Produces:** working
`empowerObject`/`disempowerObject`; abilities with `disempower_self` gated
and paid.

- [ ] Step 1: Write tests **#1** and **#2** (idempotent empower; no-op
  disempower) using `CardTestFixture` + an `EffectExecutor` bound to the
  fixture state; count `ObjectEmpoweredEvent`s via an `EventBus` subscriber.
- [ ] Step 2: Run `--gtest_filter=EmpowerTest.*`; both fail (status never
  set / event never emitted).
- [ ] Step 3: Implement the two executor methods per spec §1.
- [ ] Step 4: Run; pass. Full suite; pass.
- [ ] Step 5: Write test **#3**: a test-only card class is not needed —
  use Heart of the Tempest's id? Not registered yet. Instead register a
  minimal legend in the test file via the fixture's registry (the fixture
  exposes `card_registry`; `registerCard` with a local subclass whose one
  activated ability has `disempower_self = true` and `exhaust = true`).
  Drive `GameEngine` action generation for the controller with the legend
  in `legend_zone`; assert no `ActivateActionAbility` intent for it while
  `is_empowered == false`, one when true; execute it; assert
  `is_empowered == false` and `is_exhausted == true`.
- [ ] Step 6: Run; fails (intent offered regardless / status not
  consumed).
- [ ] Step 7: Add the gate at the three sites and the payment in
  `executeActivateAbility`; add the ` [EMPOWERED]` text in
  `state_renderer` where the legend name is printed.
- [ ] Step 8: Run; pass. Full suite; pass.
- [ ] Step 9: Commit `Kennen: Empower/Disempower status, event, and
  disempower activation cost`.

### Task 3: Play source on the card-played event + the non-hand trigger

**Files:** `src/engine/game_engine.cpp` (`playSourceFor`; the four
`CardPlayedEvent{` emit sites — lines ~1363, ~1535, ~2035, ~2042 at plan
time), `src/engine/effect_executor.cpp` (`playIgnoringCost` → emit with
the derived source), `src/engine/trigger_manager.cpp` (`onCardPlayed`
dispatch); test `tests/cards/test_play_from_non_hand.cpp`.

- [ ] Step 1: Write test **#4** with a local test legend registered in the
  fixture whose `triggerTypes()` = {WhenYouPlayFromNonHand} and whose
  `onTrigger` sets a flag on the object (e.g. increments `buff_count`, or
  calls `empowerObject`). Play a spell from hand through the engine's
  play path; assert not fired. Move the spell to trash; play it via
  `playIgnoringCost`; assert fired and that a subscribed
  `CardPlayedEvent` had `play_source == Trash`.
- [ ] Step 2: Run; fails (play_source stays Hand; trigger never
  dispatched).
- [ ] Step 3: Implement `playSourceFor` (zone → source mapping from
  addendum #2), set `play_source` at the four emit sites and in
  `playIgnoringCost`, dispatch `WhenYouPlayFromNonHand` in `onCardPlayed`
  (board cards of the player + `fireLegendTrigger`).
- [ ] Step 4: Run; pass. Full suite; pass.
- [ ] Step 5: Write test **#5**: put a champion in `champion_zone`,
  generate actions, execute its play intent; assert the trigger fired and
  the event's source was `ChampionZone`.
- [ ] Step 6: Run; if it passes immediately, the champion emit site is
  already covered by Step 3 — acceptable ONLY if Step 5's assertion on
  `play_source == ChampionZone` was seen failing when Step 3's mapping is
  temporarily reverted (do that revert/restore and record it).
- [ ] Step 7: Commit `Kennen: play_source on CardPlayedEvent +
  WhenYouPlayFromNonHand trigger`.

### Task 4: Flow — offer and pay

**Files:** `src/engine/game_engine.cpp` (`liveFlowCosts`,
`generateFlowPlayActions` called beside both `generateTrashReplayActions`
call sites; `executePlaySpell` payment branch on `flow_source`); test
`tests/cards/test_flow.cpp`.

Test cards: register a local Flow spell in the test (a `SpellCard` subclass
with `Keyword::Flow`, `flow_energy = 2`, `flow_power = 1`,
`flow_any_domain = true`, printed 1E, `Action` keyword) and a second one
with `Reaction`.

- [ ] Step 1: Write test **#6** (offered once from trash when the flow cost
  is affordable; not offered when only the printed cost is affordable —
  e.g. 1 ready rune) and **#7** (timing parity in the closed state).
- [ ] Step 2: Run; fail (no flow intents exist).
- [ ] Step 3: Implement `liveFlowCosts` and `generateFlowPlayActions` per
  spec §3 (+ addendum #1: one intent per live cost).
- [ ] Step 4: Run; pass. Full suite; pass.
- [ ] Step 5: Write test **#8**: execute the flow intent; assert rune
  deltas equal the flow cost (2 exhausted for energy + 1 recycled for
  power in any domain) and that the printed cost was not additionally
  charged; the card is no longer in trash and is on the chain.
- [ ] Step 6: Run; fails (payment path charges printed cost or asserts).
- [ ] Step 7: Implement the `executePlaySpell` branch: trash removal (existing),
  `payAdditionalCost` with the intent's cost, skip the grant path, set
  `banish_on_leave` on the chain item, log line.
- [ ] Step 8: Run; pass. Full suite; pass.
- [ ] Step 9: Write test **#12** (Death-from-Below grant + printed Flow →
  two distinct intents) and **#29** (granted + printed → two flow intents,
  each pays its own cost).
- [ ] Step 10: Run; #12 may pass already if both generators are
  independent — verify RED by temporarily disabling one generator; #29
  fails until `liveFlowCosts` returns both. Implement; pass; suite; pass.
- [ ] Step 11: Commit `Kennen: Flow alternate cost — offer from trash and
  pay`.

### Task 5: Flow — banish on leave (resolve and counter)

**Files:** `src/engine/chain_manager.cpp` (`stepResolve` disposal),
`src/cards/card_helpers.h` (already edited in Task 1: implement the
branch bodies if left as stubs); test `tests/cards/test_flow.cpp`.

- [ ] Step 1: Write test **#9**: drive a flow-played spell through
  `driveThroughChain`; assert it is in `ps.banishment`, not trash, and
  `SpellResolvedEvent` fired.
- [ ] Step 2: Run; fails (in trash).
- [ ] Step 3: Implement the `stepResolve` branch.
- [ ] Step 4: Run; pass. Suite; pass.
- [ ] Step 5: Write test **#10**: flow-played spell on chain, opponent
  counters with Hard Bargain (id 457) via `driveThroughChain`; assert the
  flow spell is in banishment and Hard Bargain in trash.
- [ ] Step 6: Run; fails. Implement the `counterChainTop` /
  `revertCounteredPlay` branch. Run; pass. Suite; pass.
- [ ] Step 7: Write test **#11**: set `granted_flow` with
  `valid_on_turn = turn_number`; assert offered; increment
  `turn.turn_number`; assert not offered.
- [ ] Step 8: Run; verify RED by asserting first with a mismatched turn
  stamp; then pass.
- [ ] Step 9: Commit `Kennen: Flow banish-on-leave and granted-flow expiry`.

### Task 6: Burn N and burn-out refactor

**Files:** `src/engine/effect_executor.cpp` (`burnCards`, `burnOut`),
`src/engine/game_engine.cpp` (`drawCards` calls the shared `burnOut`);
test `tests/cards/test_kennen_cards.cpp` (burn cases only).

- [ ] Step 1: Write tests **#13** (top 2 → trash, order preserved) and
  **#14** (1-card deck, non-empty trash: burn-out reshuffle, opponent +1
  point, second burn from the reshuffled deck) calling `burnCards`
  directly.
- [ ] Step 2: Run; fail (stub does nothing).
- [ ] Step 3: Implement per spec §5; move the burn-out block out of
  `drawCards` into `burnOut` and call it from both.
- [ ] Step 4: Run; pass. Suite; pass (the existing burn-out/scoring tests
  in `test_burn_out_and_scoring.cpp` are the regression guard for the
  refactor).
- [ ] Step 5: Commit `Kennen: Burn N helper with shared burn-out`.

### Task 7: `revealAndChoose` rest-destination

**Files:** `src/engine/effect_executor.cpp`; test in
`tests/cards/test_kennen_cards.cpp` (Lightning Rush cases use it — write
the helper test first as its own case).

- [ ] Step 1: Write a test: `revealAndChoose(P1, 3, Trash)` with a scripted
  agent choosing index 1 → chosen in hand, other two in trash in revealed
  order, `draws_this_turn == 1`, one `CardsDrawnEvent`.
- [ ] Step 2: Run; fails (rest recycled to bottom).
- [ ] Step 3: Implement the parameter; default `Recycle` keeps every
  existing caller's behaviour.
- [ ] Step 4: Run; pass. Suite; pass. Commit `Kennen: revealAndChoose
  rest-destination (Recycle|Trash)`.

### Task 8: The five cards + registration + deck file

**Files:** the five card files; `src/cards/cards_init.cpp` (declare + call
`register_card_788..792`); `decks/kennen_tyler.txt` (legend line);
`tests/cards/test_kennen_cards.cpp`; `tests/test_deck_validator.cpp`.

Card defs carry: `id`, `def_id` (`ven-155-166`, `ven-113-166`,
`ven-156-166`, `ven-100-166`, `ven-164-166`), `name`, `set_code VEN`,
`set_name Vendetta`, `public_code` (`VEN-155/166` etc.), `collector_number`,
`artist` (Six More Vodka for Kennen; Kudos Productions for the other
three), `card_type`, `super_type`, `domains`, `tags`, costs, might,
`rarity`, verbatim `ability_text` from `docs/knowledge/riftbound/reference/
cards/VEN.json` in personal-ai (copy the text, not the file), `image_url`,
and for the two Flow spells `keywords.set(Keyword::Flow)` + `flow_*`.

- [ ] Step 1: Write test **#26** (deck validates) — RED: legend name
  unresolved / five cards missing.
- [ ] Step 2: Write tests **#15–#21, #17, #28** per spec §8 / addendum,
  one at a time in the order: Kennen play-burn (#13 already covers the
  helper — #15/#16 conquer grant), Heart of the Tempest empower-on-trash-
  play + Assault action (#17), Lightning Rush (#18–#20), Up from the Deep
  (#21, #28), then Sandswept Tomb's flag (#22).
- [ ] Step 3: For each card: run its tests (RED for "card not
  registered"), write the card file, register it, build, run (GREEN),
  full suite, commit `Kennen: <card name> (VEN-nnn)`.
- [ ] Step 4: Update the deck file's legend line; run #26; GREEN; commit
  `Kennen: deck file uses engine legend naming`.
- [ ] Step 5: `python3 scripts/card_coverage.py` — total 792, the five
  classified `implemented`, INCOMPLETE still 0.

### Task 9: Sandswept Tomb discount and restricted intent

**Files:** `src/engine/game_engine.cpp` (`generateSpellActions`,
`generateFlowPlayActions`, `executePlaySpell`, `canAfford`,
`beginCostPayment`, `payCardCost`), `src/engine/game_engine.cpp` aura reset
step (BF flags), the `pickTarget` filter path (`src/cards/card.cpp` or
`card_helpers.h`, wherever the resolve-time legal list is built), test
`tests/cards/test_sandswept_tomb.cpp`.

- [ ] Step 1: Test **#22** (aura sets the flag on its own BF only) — RED;
  implement `applyPassiveAura` on the Tomb + reset; GREEN.
- [ ] Step 2: Test **#23** (Ride the Wind offered twice with a friendly
  unit at the Tomb; the restricted intent affordable with one fewer
  matching rune) — RED; implement the restricted-intent emission and the
  discounted affordability; GREEN.
- [ ] Step 3: Test **#24** (executing the restricted intent recycles one
  fewer rune; the resolve-time picker offers only Tomb units) — RED;
  implement staging of `transient_power_discount`, consumption in the
  three cost sites, chain-item restriction, picker filter; GREEN.
- [ ] Step 4: Test **#25** (no friendly unit at the Tomb → offered once) —
  RED by temporarily removing the eligibility check; restore; GREEN.
- [ ] Step 5: Full suite; commit `Kennen: Sandswept Tomb rune discount via
  restricted target intent`.

### Task 10: Smoke self-play and reporting

- [ ] Step 1: Build `riftbound`; run `./build/riftbound --agent1 mcts:sims=50
  --agent2 mcts:sims=50 --deck1 decks/kennen_tyler.txt --deck2
  decks/rengar_test.txt --render-html on` for 5 seeds; record winner,
  turns, and whether `FLOW:` / `EMPOWER:` / `BURN:` lines appear.
- [ ] Step 2: Report the batch honestly (it is a smoke check, not a
  measurement). Any exception or assertion → back to the failing task with
  a new regression test.
- [ ] Step 3: Final full suite; `card_coverage.py`; commit
  `Kennen: smoke self-play notes`.

## Self-review against the spec

- Spec coverage: §1 → T2; §2 → T3; §3 → T4/T5; §4 → T9; §5 → T6; §6 → T8
  (+T7 for Lightning Rush's helper); §7 → T8; §8 tests #1–#29 all placed
  (#27 in T1); §9 → branch discipline in Global constraints. Addendum
  #1–#9 → T4 (#1), T3 (#2, #3), T7 (#4), T9 (#5), T4 wording (#6), T8 (#7),
  T2 (#8 — HTML marker checked at T2 Step 7: only if the legend card is
  rendered in `play_index_html.cpp`), spec wording (#9).
- Placeholder scan: none.
- Name consistency: `flow_source`, `liveFlowCosts`, `playSourceFor`,
  `burnCards`, `burnOut`, `revealAndChoose(…, RestDestination)`,
  `transient_power_discount`, `friendly_spell_power_discount`,
  `banish_on_leave`, `target_battlefield_restriction`, `is_empowered`,
  `granted_flow`, `disempower_self`, `WhenYouPlayFromNonHand` — used
  identically in every task.
