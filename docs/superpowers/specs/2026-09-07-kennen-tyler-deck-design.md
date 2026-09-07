# Kennen (Heart of the Tempest) deck support — design spec

Date: 2026-09-07. Status: APPROVED in chat (design sections 1–7); AUDITED
2026-09-07 per Tyler's audit-the-plan process — all nine findings folded on
his ruling (see the dated addendum at the end; superseded text is marked
inline with `[SUPERSEDED → addendum #N]` and left in place).

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
`[SUPERSEDED → addendum #10]` ~~(same places `is_stunned` is reset)~~ — in the
executor's board-exit paths (kill, bounce to hand, banish, recycle-from-
board); see addendum #10.

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
name when set. `[SUPERSEDED → addendum #8]` ~~and the HTML renderer shows a
⚡ marker on the legend card.~~ The HTML marker is conditional — see
addendum #8.

## Section 2 — "Play from anywhere other than your hand"

**Event.** `CardPlayedEvent::play_source` (`Intent::PlaySource`, default
Hand). `[SUPERSEDED → addendum #2]` ~~Set at every emit site in
`game_engine.cpp` (four today) from the executing intent. `Intent::
PlaySource` gains `Deck` for cards played from the top of the main deck
(Nocturne's off-the-top play).~~ Set at every emit site from a shared
zone-derivation helper — see addendum #2. No `Deck` value is added.

**Free-play helper.** `EffectExecutor::playIgnoringCost` uses the same
zone-derivation helper (addendum #2) so the resulting `CardPlayedEvent`
carries the true source. No caller changes. `[SUPERSEDED → addendum #2]`
~~(MainDeck → Deck, …)~~ — the mapping is defined once in the addendum.

**Trigger.** `TriggerType::WhenYouPlayFromNonHand`. In
`TriggerManager::onCardPlayed`, when `e.play_source != Hand`, broadcast to
the player's on-board cards that fire on it (same loop shape as
`WhenYouPlayASpell`) and to the legend via `fireLegendTrigger`. The played
card is the chain item's triggering subject.

Sources that count, all of which occur in Tyler's deck: Flow plays,
Fizz's trash replay, The Harrowing, Last Rites, Nocturne (which banishes
itself from the deck top and is played from banishment — verified in
`0194_nocturne_horrifying.cpp`), hidden-card plays (Tideturner,
Switcheroo), and the champion from the champion zone (addendum #3).
Token plays do NOT count: tokens are not cards (CR 185, 350.2) and the
engine's `createToken` emits no `CardPlayedEvent` (addendum #7).

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

**Effective flow costs.** `[SUPERSEDED → addendum #1]` ~~`GameEngine::
effectiveFlowCost(obj)` → granted flow if live, else printed flow if the
object has the keyword, else invalid. Both may exist; granted wins for the
turn because it is the more recent, specific permission (… preferring the
grant never removes an option the player would take).~~
`GameEngine::liveFlowCosts(obj)` returns BOTH costs when both are live —
the printed one (keyword present) and the granted one (turn stamp matches)
— and the controller chooses between them (CR 829.1.c.3). See addendum #1.

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
  `Intent::flow_source` (`None | Printed | Granted`) — one intent per live
  flow cost (addendum #1).

`generateTrashReplayActions` (Death from Below grants) is unchanged and
must not double-offer: a trash spell with BOTH a replay grant and a flow
cost yields two distinct intents (one per cost), which is CR-correct.

**Execution.** In `executePlaySpell`, when `flow_source != None`: the
existing Trash-zone removal runs; payment is `payAdditionalCost` with the
selected flow cost (printed or granted per the intent; choose the first
payable domain when `any_domain`) instead of `payCardCost`; the
`TrashReplayGrant` path is not consulted; log `FLOW: <name> played from
trash for [E<n>][P<n>] (<printed|granted>)`. The chain item created for
the spell gets `ChainItem::banish_on_leave = true`. `[SUPERSEDED →
addendum #6]` ~~Additional-cost and optional-cost riders (Brazen-style) are
skipped on a flow play exactly as they are on an alternate-cost play.~~
Flow replaces the BASE cost only (CR 829.1.c.1); additional costs would
still apply, but no Flow spell in either deck has one, so the rider code
path is left untouched and unexercised (addendum #6).

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

`[SUPERSEDED → addendum #9]` ~~Only spells in the two decks that choose
friendly units are affected (Ride the Wind, Star-Crossed, Switcheroo, Flash,
Rengar's Challenge-style picks); the mechanism is generic.~~ The mechanism
is generic; the exact list of affected spells in the two decks is fixed in
addendum #9. Both players benefit from the Tomb — "friendly" is relative
to the spell's controller, and the flag lives on the battlefield.

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
  "none"; the chosen card goes to hand AS A DRAW — via
  `EffectExecutor::revealAndChoose` (`effect_executor.cpp:1065–1153`), the
  helper that increments `draws_this_turn` and emits `CardsDrawnEvent` for
  the chosen cards — so `WhenYouDrawACard` consumers see it.
  `[SUPERSEDED → addendum #4]` ~~the rest go to trash in their revealed
  order~~ → the helper today RECYCLES the non-chosen cards to the bottom;
  Lightning Rush needs them in TRASH, so the helper gains a rest-destination
  parameter (addendum #4). Fewer than 3 cards → operate on what exists.

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
   when its FLOW cost is affordable; when the player can afford the
   spell's printed (hand) cost but NOT its flow cost, no flow intent is
   offered (reworded per addendum #9).
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

## Addendum 2026-09-07 — audit-the-plan findings, folded on Tyler's ruling

Audit run per `.claude/skills/audit-the-plan` (personal-ai). Tyler ruled
"fold all nine as recommended". Each item below overrides the inline text
it is linked from.

1. **Both Flow costs are offered (CR 829.1.c.3).** Replaces "granted wins".
   `GameEngine::liveFlowCosts(obj)` returns up to two costs: printed (the
   object has `Keyword::Flow`) and granted (`granted_flow.valid_on_turn ==
   turn.turn_number`). `Intent::flow_source` ∈ {None, Printed, Granted}
   replaces the `use_flow_cost` bool everywhere it was mentioned; the
   generator emits one intent per live cost that is affordable; execution
   pays the cost named by the intent. New test: **#29** — a spell with a
   live granted flow AND printed Flow yields two flow intents, one per
   cost, and each pays its own cost.
2. **Play source is derived from the card's zone, in one helper.** Replaces
   "set from the executing intent" (the engine sets `play_source` only on
   the Death-from-Below path; champion-zone and hidden plays never tag their
   intent — verified by grep). `GameEngine::playSourceFor(obj)` maps the
   object's zone at the moment of execution: Hand → Hand, Trash → Trash,
   ChampionZone → ChampionZone, Banishment → Banishment, hidden-at-
   battlefield → Hidden, Chain → ChainZone. All four `CardPlayedEvent` emit
   sites and `EffectExecutor::playIgnoringCost` call it. No `Deck` value:
   no in-scope card is played from the deck top (Nocturne banishes first).
3. **Playing the champion from the champion zone empowers the legend.**
   Tyler's ruling on the CR reading (champion zone ≠ hand; no errata entry
   for any of the five cards). Test #5 stays and is the explicit witness.
4. **`revealAndChoose` gains a rest-destination parameter** (`Recycle` —
   today's behaviour and the default, so no caller changes — or `Trash`).
   Lightning Rush passes `Trash`; the non-chosen cards go to trash in their
   revealed order.
5. **Sandswept Tomb keeps the restricted-intent mechanism.** Cost accepted:
   +1 offered action per affected spell per Tomb with an eligible friendly
   unit; no cheaper faithful design exists.
6. **Flow replaces the base cost only.** Wording fixed in §3; the
   additional-cost rider path is untouched and unexercised in scope.
7. **Tokens are not cards.** New test **#28** — resolving Up from the Deep
   (two Tentacle tokens) does not empower Heart of the Tempest.
8. **HTML ⚡ marker is conditional.** `state_renderer` text marker is
   required; the HTML marker is added only if `play_index_html.cpp`
   already renders the legend card (checked in the plan, not assumed).
9. **Wording.** Test #6 reworded (above). Affected spells for §4 in the two
   decks, from the card texts in the registry: Tyler — Ride the Wind
   (friendly unit), Star-Crossed (friendly + enemy), Switcheroo (friendly +
   enemy), Flash (two friendly units); Rengar — Repulse (chooses a spell,
   not a unit → unaffected), Challenge (enemy unit → unaffected), Thrill of
   the Hunt (friendly unit → affected). Any spell whose target requirements
   allow a friendly unit is eligible; the list is descriptive, the rule is
   generic.

10. **(added during Task 2 review, 2026-09-07)** "Same places `is_stunned`
    is reset" was wrong: stun decays per turn at the Ending Step (CR
    423.1.a.2), which is not a board-exit event. Empowered is cleared in the
    executor's board-exit paths — `killObject`, `bounceToHand`,
    `banishObject`, and any recycle that takes an object off the board.
    Tests **#30** (empowered unit killed → not empowered) and **#31**
    (empowered unit bounced → not empowered).

11. **(added during Task 8 review, 2026-09-07)** `revealAndChoose` asks
    draw/skip PER card with no cap, so routing Lightning Rush through it
    (addendum #4) would let an agent draw all three — a rules break under
    the fully-faithful ruling. Lightning Rush therefore implements its own
    resumable look-and-choose in the card (Stacked Deck's pattern plus a
    decline mode: exactly one card or none), does the draw bookkeeping
    itself (`draws_this_turn`, `CardsDrawnEvent`), and sends the rest to
    trash in looked-at order. "Look at" is PRIVATE (CR 128.4): the card
    emits `CardRevealedEvent{revealed_to_all=false, revealed_to=controller}`
    per card, never a public reveal. Addendum #4's helper parameter stays
    (implemented and tested in Task 7) but has no in-scope caller.

12. **(added during Tasks 4 and 9 review, 2026-09-07) Known limitation —
    OpenSpiel action-id aliasing.** The engine offers distinct intents for a
    printed vs a granted Flow cost (addendum #1) and for a restricted vs a
    plain Tomb offer (§4), but `src/openspiel/action_vocab.cpp` keys the
    whole Play family on the card slot alone, so an OpenSpiel/MCTS agent
    sees one action id and `decodeAction` returns the FIRST matching legal
    intent. Engine-internal agents that pick from the `Intent` list are
    unaffected. Consequence for self-play evidence: the MCTS agent cannot
    deliberately choose the cheaper granted Flow or the Tomb-discounted
    play when the plain offer is also legal, so results UNDERSTATE the
    value of Kennen's grant and of the Tomb. Pre-existing vocabulary
    shape (per-target Play variants already alias); out of this plan's
    scope; a follow-up would add the flow_source / restriction bits to the
    action key.

13. **(added at the final whole-branch review, 2026-09-07) Two scope
    boundaries stated so "no approximations" stays literally true.**
    (a) Flow is implemented for SPELLS only (`generateFlowPlayActions`
    gates on `isSpell()`); CR 829.1.a says Flow is present on spells, and
    no non-spell Flow card exists in either deck. (b) Up from the Deep's
    Tentacle tokens are played to the controller's base; CR 355.2.a lets a
    controller play a unit to a battlefield they control — the engine's
    convention for every existing token-maker is base, and the agent may
    move them next turn. Both are boundaries of the engine's play-location
    model, not card-text narrowing; a follow-up could offer the
    battlefield choice for token plays engine-wide.
14. **(final review)** The closed-state reaction path executed spells
    through `ChainManager` rather than `executePlaySpell`, so Flow /
    Tomb reaction offers were generated but mis-executed. Fixed in the
    final fix wave by routing the reaction play through the one executor
    via an injected callback; tests execute a closed-state Flow play, a
    granted-Flow reaction, and a Tomb-restricted reaction end-to-end.
    Minefield's "top 2 to trash" now uses `burnCards` (burn-out parity
    with Kennen). The live hidden-reveal path now emits
    `PlayedFromFacedownEvent`; the dead `executePlayFromHidden` is
    removed.

Tests added by this addendum: #28 (tokens don't empower), #29 (both Flow
costs offered and each pays its own), #30–#31 (Empowered clears on board
exit); #28 rewritten per addendum #11's review to assert no
`CardPlayedEvent` through a real engine resolve.

## Section 9 — Where it lives

Branch `kennen-tyler-deck` in Tyler's fork of `chorlick/alpharune` (fork
pending on GitHub; until it exists the branch is container-local). This
spec and the implementation plan live under `docs/superpowers/` on that
branch. personal-ai receives only a session-log entry and a one-line vault
pointer.
