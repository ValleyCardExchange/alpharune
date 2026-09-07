# Kennen (Heart of the Tempest) deck support — design spec

Date: 2026-09-07. Status: APPROVED in chat (design sections 1–7), awaiting
spec read-through before the implementation plan.

## Goal

Make alpharune play Tyler's exact 40-card Kennen list (`decks/kennen_tyler.txt`)
against the stock `decks/rengar_test.txt` in agent-vs-agent self-play, with
**full rules fidelity, no documented approximations** for the new material.

## Scope

In scope — only what the two decks need:

- Two new keywords the engine has no concept of: **Empower / Empowered /
  Disempower** (CR 441, 442, 827, 828) and **Flow** (CR 829).
- One new trigger condition: "When you play a card from anywhere other than
  your hand."
- One new cost-reduction effect: Sandswept Tomb's rainbow-rune discount.
- One new game action helper: **Burn N** (CR 440), including burn-out.
- Five new cards (all set VEN, none present in the 787-card registry):
  Heart of the Tempest (legend), Kennen — Storm of Shuriken (champion),
  Lightning Rush, Up from the Deep, Sandswept Tomb.
- Deck file fix: the legend line uses the engine's title-only legend naming.

Out of scope (explicitly not built): the `[Empowered] >` dependent-keyword
machinery for other cards; Flow interacting with Repeat; multiple Flow
instances on one spell; any agent heuristics; anything in the personal-ai
repo beyond a session-log entry and a vault pointer to the fork.

Audit basis: the other 18 unique cards in the list already exist and are
classified `implemented` by `scripts/card_coverage.py` (three carry the
repo's pre-existing approximations — Last Rites, The Harrowing, Nocturne —
which are not touched by this work). The Rengar list is fully covered.

## Verified engine facts this design relies on

- `Keyword` is a `uint32_t` bit index enum with `Count = 23`; `KeywordSet`
  is a 32-bit mask, so two more bits fit without widening.
- `GameObject` carries binary statuses (`is_exhausted`, `is_stunned`,
  `is_hidden`).
- `ActivationCost` has exhaust / energy / power / recycle_self / discard /
  xp components; the action generator gates on them at three sites
  (`generateActivateAbilityActions`, the showdown [Action] site, the
  reaction site) and pays them in one site in `executeActivateAbility`.
- `Card::triggerTypes()` supports multi-trigger cards; `CardContext::
  firing_trigger` tells `onTrigger` which fired. `TriggerManager::
  fireLegendTrigger` broadcasts a trigger type to a player's legend.
- `CardPlayedEvent` has no play-source field. `Intent::PlaySource` has
  Hand / Trash / Banishment / ChampionZone / Hidden / ChainZone (no deck-top
  value). `PlayerState::current_play_source` is only live during cost
  payment and is reset before the event is emitted.
- `Card::alternativePlayCost` + `Intent::use_alt_play_cost` implement
  Jhin's alternate cost: generator offers an extra intent, executor pays
  `payAdditionalCost(energy, power, domain)` instead of `payCardCost`.
  `canPayAdditionalCost` answers affordability for a (energy, power,
  domain) triple; rainbow `[A]` is handled by trying each domain.
- `executePlaySpell` already removes a card from `ps.trash` when
  `play_source == Trash`. Resolved spells are moved to trash in
  `ChainManager::stepResolve`; countered spells are moved to trash in
  `counterChainTop` (`src/cards/card_helpers.h`). `EffectExecutor::
  banishObject` and `PlayerState::banishment` exist.
- `PlayerState::transient_play_discount` is the precedent for a discount
  staged just before payment and consumed by `canAfford` /
  `beginCostPayment` / `payCardCost`. There is no power-cost discount path.
- Spells that opt into `needsPlayTimeTarget()` are offered as ONE intent
  with empty targets and pick their target at resolve time via
  `pickTarget` (action-vocabulary design). Ride the Wind does this.
- `EffectExecutor::createToken(controller, type, name, might, tags,
  keywords, location, enter_ready)`; unit tokens enter exhausted unless
  `enter_ready` (or Renata's aura). `giveTemporaryKeyword(target, kw,
  value)` exists; Assault is a keyword with a value.
- Burn-out (CR 431) exists only inline inside `GameEngine::drawCards`.
- Legends are registered under their title (`Blind Monk`, not `Lee Sin,
  Blind Monk`); `CardDB::findByName` resolves deck lines.
- Next free registry ids: 788–792. Registration is one `register_card_N`
  declaration + call in `src/cards/cards_init.cpp`.

## Section 1 — Empower / Empowered / Disempower

**State.** `GameObject::is_empowered` (bool, default false). Binary per CR
441.1.a. Survives turn boundaries; cleared when the object leaves the board
(same places `is_stunned` is reset).

**Actions (EffectExecutor).**
- `empowerObject(id)`: if already empowered, nothing (CR 441.1.c);
  otherwise set the flag, log `EMPOWER: <name>`, emit a new
  `ObjectEmpoweredEvent{object, controller}` (CR 441.2.a — an event other
  effects may reference; no consumer in scope, but the event is the
  faithful shape).
- `disempowerObject(id)`: if not empowered, nothing (CR 442.1.a.1);
  otherwise clear the flag and log `DISEMPOWER: <name>`.

**Cost component.** `ActivationCost::disempower_self` (bool). Semantics:
- Offer gate (all three generator sites): skip the ability if
  `disempower_self` and the source is not empowered.
- Payment (`executeActivateAbility`): after the exhaust component, call
  `disempowerObject(source)`; log as `ACTIVATE_COST: disempower`.

**Rendering.** `state_renderer` prints ` [EMPOWERED]` after the legend
name when set, and the HTML renderer shows a ⚡ marker on the legend card.

## Section 2 — "Play from anywhere other than your hand"

**Event.** `CardPlayedEvent::play_source` (`Intent::PlaySource`, default
Hand). Set at every emit site in `game_engine.cpp` (four today) from the
executing intent. `Intent::PlaySource` gains `Deck` for cards played from
the top of the main deck (Nocturne's off-the-top play).

**Free-play helper.** `EffectExecutor::playIgnoringCost` derives the source
from the card's zone at call time (MainDeck → Deck, Trash → Trash,
ChampionZone → ChampionZone, Banishment → Banishment, Hand → Hand,
BattlefieldZone-hidden → Hidden) and the resulting `CardPlayedEvent`
carries it. No caller changes.

**Trigger.** `TriggerType::WhenYouPlayFromNonHand`. In
`TriggerManager::onCardPlayed`, when `e.play_source != Hand`, broadcast to
the player's on-board cards that fire on it (same loop shape as
`WhenYouPlayASpell`) and to the legend via `fireLegendTrigger`. The played
card is the chain item's triggering subject.

Sources that count, all of which occur in Tyler's deck: Flow plays,
Fizz's trash replay, The Harrowing, Last Rites, Nocturne off the top,
hidden-card plays (Tideturner, Switcheroo), and the champion from the
champion zone.

## Section 3 — Flow (alternate-cost subsystem)

**Keyword.** `Keyword::Flow = 23`, `Count = 24`, `toString` entry
`"Flow"`.

**Printed flow cost.** `CardDef` gains `flow_energy`, `flow_power`,
`flow_any_domain`; `Card::flowCost()` (virtual) returns a `FlowCost{valid,
energy, power, power_domain, any_domain}` — the default implementation
reads the def and is valid only when the def's keywords include Flow.
Power domain for a printed non-rainbow flow cost is the card's first
domain.

**Granted flow (Kennen champion).** On `GameObject`: `granted_flow`
(optional struct: energy, power, power_domain, any_domain, `valid_on_turn`
= the turn number it was granted). "This turn" expiry is evaluated, not
scheduled: the grant is live iff `valid_on_turn == state.turn.turn_number`.

**Effective flow cost.** `GameEngine::effectiveFlowCost(obj)` → granted
flow if live, else printed flow if the object has the keyword, else
invalid. Both may exist; granted wins for the turn because it is the more
recent, specific permission (CR 829.1.c.3 lets the controller choose when
multiple instances exist; in scope only Kennen's grant ever coexists with a
printed Flow, and its cost equals the printed base cost, so preferring the
grant never removes an option the player would take).

**Offer.** `GameEngine::generateFlowPlayActions(player, action_ok,
reaction_ok, actions)`, called next to `generateTrashReplayActions` at both
call sites. For each spell object in `ps.trash` with a valid effective flow
cost:
- Timing gate identical to `generateSpellActions` (neutral-open /
  showdown-open / closed-state rules on the Action and Reaction keywords) —
  CR 829.1.b.2.
- Respect `cant_play_cards_this_turn` / `cant_play_spells_this_turn`.
- Affordability via `canPayAdditionalCost` against the flow cost (loop over
  domains when `any_domain`).
- Target shape identical to the hand-spell path (`needsPlayTimeTarget`,
  `hasLegalTargets`, `enumerateLegalTargets`, optional targets).
- Emit the same intent types as hand spells (`PlayCard` /
  `PlayActionCard` / `PlayReaction`) with `play_source = Trash` and a new
  `Intent::use_flow_cost = true`.

`generateTrashReplayActions` (Death from Below grants) is unchanged and
must not double-offer: a trash spell with BOTH a replay grant and a flow
cost yields two distinct intents (one per cost), which is CR-correct.

**Execution.** In `executePlaySpell`, when `use_flow_cost`: the existing
Trash-zone removal runs; payment is `payAdditionalCost` with the effective
flow cost (choose the first payable domain when `any_domain`) instead of
`payCardCost`; the `TrashReplayGrant` path is not consulted; log
`FLOW: <name> played from trash for [E<n>][P<n>]`. The chain item created
for the spell gets `ChainItem::banish_on_leave = true`. Additional-cost and
optional-cost riders (Brazen-style) are skipped on a flow play exactly as
they are on an alternate-cost play.

**Banish (CR 829.1.b.1).** Two disposal sites branch on
`banish_on_leave`:
- `ChainManager::stepResolve` — instead of the trash move, call the banish
  path: zone Banishment, `ps.banishment.push_back`, log
  `FLOW: <name> banished`, still emit `SpellResolvedEvent` and a
  `LeftBoardEvent` whose destination zone is Banishment.
- `counterChainTop` / `revertCounteredPlay` in `card_helpers.h` — a
  countered flow spell goes to banishment, not trash.
A granted flow is consumed on play (the object leaves trash), so no
explicit clear is needed beyond the turn check.

## Section 4 — Sandswept Tomb rune discount

Text: "Each spell that chooses one or more units here that are friendly to
it costs [A] less."

**Battlefield flag.** `BattlefieldState::friendly_spell_power_discount`
(int, default 0). Sandswept Tomb's `applyPassiveAura` sets it to 1 on its
own battlefield (same pattern as Altar of Blood's `death_recall_for_pay`).
Reset to 0 in the aura-recompute reset step with the other BF auras.

**Discount staging.** `PlayerState::transient_power_discount` (int, reset in
`resetTurnTracking`, mirrors `transient_play_discount`). `canAfford`,
`beginCostPayment` and `payCardCost` subtract it from the power requirement
(clamped at 0) at the same points they subtract the energy discount. The
domain of the power cost is unchanged — a rune of the card's domain is
simply not recycled.

**Choosing units "here".** Because play-time-target spells pick at resolve,
the generator emits a second, restricted intent when the discount could
apply:
- New `Intent::target_battlefield_restriction` (optional `BattlefieldId`).
- In `generateSpellActions` (hand) and `generateFlowPlayActions` (trash):
  for a spell whose target requirements include a friendly unit, and for
  each battlefield with `friendly_spell_power_discount > 0` where the
  player has at least one legal friendly-unit target, emit an additional
  intent carrying the restriction. Its affordability check uses the
  discounted power cost. Spells with `power_cost == 0` get no restricted
  intent (nothing to discount).
- For spells that take targets at play time (`needsPlayTimeTarget ==
  false`), no restricted intent is needed: the discount is applied
  directly when any chosen target is a friendly unit at a flagged
  battlefield.
- Execution: when the restriction is present, stage
  `transient_power_discount` before payment and clear after; the chain
  item records the restriction; `pickTarget` at resolve filters the legal
  list to units at that battlefield (a resolve-time choice can therefore
  never dodge the discount's condition). If no legal target remains at
  resolve, the spell resolves with no target exactly as a normal spell
  whose target vanished.

Only spells in the two decks that choose friendly units are affected
(Ride the Wind, Star-Crossed, Switcheroo, Flash, Rengar's Challenge-style
picks); the mechanism is generic.

## Section 5 — Burn N

`EffectExecutor::burnCards(player, n)`: move the top card of the main deck
to trash `n` times, logging `BURN: <name>`. If the deck is empty before all
`n` are burned, run burn-out (the block currently inline in
`GameEngine::drawCards`, factored into a shared `burnOut(player)` that both
callers use; behaviour of drawing is unchanged) and continue burning from
the reshuffled deck (CR 440.4). If deck and trash are both empty, stop.

## Section 6 — The five cards

All defs carry set_code `VEN`, set_name `Vendetta`, the Riftcodex
`riftbound_id` as `def_id`, official `public_code`, verbatim ability text,
and the CDN image URL from the card DB.

**788 — Heart of the Tempest** (Legend; Order + Chaos; tags Yordle, Kennen).
- `triggerTypes()` = { WhenYouPlayFromNonHand } → `empowerObject(self)`.
- `activatedAbilities()` = one ability: `is_action = true`, cost
  `{exhaust = true, disempower_self = true}`, targets `{count 1,
  must_be_unit}` (any unit — the text says "a unit"),
  `needs_activation_time_target = true`. `onActivate` picks the unit via
  `pickTarget` and calls `giveTemporaryKeyword(unit, Assault, 2)`.
- Deck file line: `1 Heart of the Tempest`.

**789 — Kennen, Storm of Shuriken** (Unit, Champion; Chaos; 3E / 1P; 4 M;
tags Yordle, Kennen).
- `triggerTypes()` = { WhenYouPlayMe, WhenIConquer }.
- WhenYouPlayMe → `burnCards(controller, 2)`.
- WhenIConquer → if the trash holds at least one spell, the controller
  chooses one (resumable `pickTarget` over trash spells, mandatory — the
  text is not "may"); set its `granted_flow` = the spell's printed
  `{energy_cost, power_cost, first domain, any_domain = false}` with
  `valid_on_turn` = current turn. Log `KENNEN: <spell> gains Flow [cost]
  this turn`. No spell in trash → nothing.

**790 — Lightning Rush** (Spell, Signature; Order + Chaos; 1E; keywords
Flow; flow cost 2E + 1 power any-domain; tags Kennen).
- `onResolve` (resumable): reveal the top up-to-3 cards to the controller
  only (`CardRevealedEvent`, private); offer a choice among them plus
  "none"; the chosen card goes to hand AS A DRAW — via the executor's
  existing look-at-top-N-and-choose helper, the one that increments
  `draws_this_turn` and emits `CardsDrawnEvent` for the chosen cards
  (`effect_executor.cpp` ~1095–1153) — so `WhenYouDrawACard` consumers see
  it; the rest go to trash in their revealed order (this differs from
  Stacked Deck, which recycles the rest to the bottom). Fewer than 3 cards
  → operate on what exists.

**791 — Up from the Deep** (Spell; Chaos; 3E; keywords Flow; flow cost 3E;
no power).
- `onResolve`: `createToken` twice: Unit, name `Tentacle`, might 1, tags
  {Tentacle, Bilgewater}, no keywords, location = controller's base,
  `enter_ready = false` (units enter exhausted unless an effect readies
  them — engine convention, matches Altar to Unity / Emperor's Dais).

**792 — Sandswept Tomb** (Battlefield; colorless).
- `applyPassiveAura` sets `friendly_spell_power_discount = 1` on the
  battlefield whose card object is this Tomb.

## Section 7 — Deck file and validation

`decks/kennen_tyler.txt`: legend line becomes `Heart of the Tempest`; the
rest is unchanged (champion `Kennen, Storm of Shuriken`; 40 main-deck
cards; battlefields Zaun Warrens, Minefield, Sandswept Tomb; 9 Chaos + 3
Order runes). The validator must accept it: legend domains Order + Chaos
cover every card; Lightning Rush is the legend's Signature; rune domains are
subsets of the legend's.

## Section 8 — Tests (each must be watched failing before its code exists)

Test files under `tests/cards/` using `CardTestFixture` unless noted.

`test_empower.cpp`
1. Empowering an object sets the status; empowering again is a no-op and
   emits no second event.
2. Disempowering a non-empowered object leaves it non-empowered and emits
   nothing.
3. An ability with `disempower_self` is absent from the offered intents
   while the source is not empowered, present when it is, and activating
   it leaves the source disempowered and exhausted.

`test_play_from_non_hand.cpp`
4. Playing a spell from hand does not fire `WhenYouPlayFromNonHand` on the
   legend; playing the same spell via `playIgnoringCost` from trash does,
   and the event's `play_source` is Trash.
5. Playing the champion from the champion zone fires it (`play_source =
   ChampionZone`).

`test_flow.cpp`
6. A printed-Flow spell in trash is offered exactly once as a flow intent
   when the flow cost is affordable and not offered when only the printed
   cost would be affordable.
7. A flow intent is not offered in the closed state for a spell without
   Reaction, and is offered for one with Reaction (timing parity with hand
   spells).
8. Executing a flow play pays exactly the flow cost (ready-rune / recycled-
   rune deltas) and leaves the printed cost unpaid.
9. After a flow spell resolves it is in banishment, not trash, and
   `SpellResolvedEvent` fired.
10. A flow spell countered by Hard Bargain is in banishment.
11. A granted flow is offered on the turn it was granted and absent after
    the turn number advances.
12. A trash spell with both a Death-from-Below grant and a printed flow
    cost yields two distinct intents.

`test_kennen_cards.cpp`
13. Kennen's play trigger moves the top 2 deck cards to trash in order.
14. With a 1-card deck and a non-empty trash, Kennen's burn trashes the
    last card, burn-out reshuffles and awards the opponent 1 point, and
    the second burn comes from the reshuffled deck.
15. Kennen's conquer trigger with two spells in trash lets the agent choose
    the second and sets its granted flow to that spell's printed cost.
16. Kennen's conquer trigger with no spells in trash changes nothing.
17. Heart of the Tempest becomes empowered when a card is played from
    trash, and its action gives the chosen unit Assault 2 that expires at
    the turn's expiration step.
18. Lightning Rush: agent draws the second of three revealed cards; the
    other two are in trash in revealed order; hand size +1.
19. Lightning Rush: agent picks "none"; all three are in trash.
20. Lightning Rush with a 2-card deck reveals two.
21. Up from the Deep creates two exhausted 1-might Tentacle units in the
    controller's base.

`test_sandswept_tomb.cpp`
22. The Tomb's aura sets the discount flag on its own battlefield only.
23. Ride the Wind with a friendly unit at the Tomb is offered twice (normal
    and restricted); the restricted intent is affordable with one fewer
    matching rune.
24. Executing the restricted intent recycles one fewer rune for power and
    the resolve-time picker offers only units at the Tomb.
25. Ride the Wind with no friendly unit at the Tomb is offered once.

`tests/test_deck_validator.cpp` (existing file, new case)
26. `decks/kennen_tyler.txt` loads and validates with zero errors.

Guard
27. `toString(Keyword::Flow)` returns "Flow" and `Keyword::Count` equals
    the number of `toString` cases (extend the existing keyword-vocabulary
    test if one exists, else add it in `tests/test_types.cpp`).

Suite-level: full `riftbound_tests` green; `scripts/card_coverage.py`
reports the five new cards as `implemented` with no engine-gap marker;
smoke run `riftbound --agent1 mcts:sims=50 --agent2 mcts:sims=50 --deck1
decks/kennen_tyler.txt --deck2 decks/rengar_test.txt --render-html on`
reaches game over without an exception and its log contains at least one
`FLOW:` and one `EMPOWER:` line across a small batch of seeds (a smoke
check, not a test — the batch is reported, not asserted).

## Section 9 — Where it lives

Branch `kennen-tyler-deck` in Tyler's fork of `chorlick/alpharune` (fork
pending on GitHub; until it exists the branch is container-local). This
spec and the implementation plan live under `docs/superpowers/` on that
branch. personal-ai receives only a session-log entry and a one-line vault
pointer.
