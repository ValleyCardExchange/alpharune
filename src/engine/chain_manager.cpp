#include "chain_manager.h"

#include "effect_executor.h"

#include <algorithm>
#include <cassert>

namespace riftbound {

ChainManager::ChainManager(GameState& state, EventBus& events,
                           const CardDB& card_db)
    : state_(state), events_(events), card_db_(card_db) {}

ChainItemId ChainManager::addSpell(GameObjectId spell_obj, PlayerId controller,
                                    const std::vector<GameObjectId>& targets) {
    auto& chain = state_.chain;
    bool was_empty = !chain.exists();

    ChainItem item;
    item.id = chain.allocateId();
    item.status = ChainItemStatus::Pending;
    item.source = spell_obj;
    item.card_def_id = state_.getObject(spell_obj).card_def_id;
    item.controller = controller;
    item.targets = targets;
    item.is_spell = true;

    chain.items.push_back(item);

    // Move spell object to chain zone
    auto& obj = state_.getObject(spell_obj);
    obj.zone = ZoneType::Chain;

    // Close the state (CR 354)
    state_.turn.oc_state = OpenClosedState::Closed;

    if (was_empty) {
        events_.emit(ChainCreatedEvent{item.id, controller});
    }

    return item.id;
}

ChainItemId ChainManager::addPermanent(GameObjectId card_obj,
                                         PlayerId controller,
                                         const std::vector<GameObjectId>& targets) {
    auto& chain = state_.chain;
    bool was_empty = !chain.exists();

    ChainItem item;
    item.id = chain.allocateId();
    item.status = ChainItemStatus::Pending;
    item.source = card_obj;
    item.card_def_id = state_.getObject(card_obj).card_def_id;
    item.controller = controller;
    item.targets = targets;
    item.is_permanent = true;

    chain.items.push_back(item);

    // Card goes to Chain zone temporarily (CR 354: step 1)
    auto& obj = state_.getObject(card_obj);
    obj.zone = ZoneType::Chain;

    if (was_empty) {
        events_.emit(ChainCreatedEvent{item.id, controller});
    }

    return item.id;
}

ChainItemId ChainManager::addAbility(GameObjectId source, PlayerId controller,
                                      CardDefId def_id,
                                      const std::vector<GameObjectId>& targets,
                                      bool is_activated,
                                      int ability_index) {
    auto& chain = state_.chain;
    bool was_empty = !chain.exists();

    ChainItem item;
    item.id = chain.allocateId();
    item.status = ChainItemStatus::Pending;
    item.source = source;
    item.card_def_id = def_id;
    item.controller = controller;
    item.targets = targets;
    item.is_ability = true;
    item.is_activated_ability = is_activated;
    item.ability_index = ability_index;

    chain.items.push_back(item);

    // Source stays on board — do NOT move to Chain zone

    // Close state when chain is created
    state_.turn.oc_state = OpenClosedState::Closed;

    if (was_empty) {
        events_.emit(ChainCreatedEvent{item.id, controller});
    }

    return item.id;
}

void ChainManager::processFEPR(
    AgentQuery query_agent,
    std::function<void(const ChainItem&)> resolve_permanent,
    std::function<void(const ChainItem&)> resolve_spell,
    ClosedActionsGen gen_closed_actions) {

    // Mark the loop live for the duration (see ChainManager::isProcessing —
    // a routed closed-state spell play re-enters GameEngine::runChain from
    // inside stepExecuteAndPass). RAII so the two early returns below can't
    // leave the flag stuck set.
    struct ProcessingGuard {
        bool& flag;
        explicit ProcessingGuard(bool& f) : flag(f) { flag = true; }
        ~ProcessingGuard() { flag = false; }
    } processing_guard(processing_);

    constexpr int kMaxIterations = 100; // safety
    int iterations = 0;

    while (state_.chain.exists() && iterations < kMaxIterations) {
        iterations++;

        // Step 1: Finalize
        // Permanents resolve immediately here (CR 337.1.c).
        // If finalize removed all items, chain is empty → done.
        bool finalized_something = stepFinalize(resolve_permanent);
        if (!state_.chain.exists()) {
            // Chain emptied during finalization
            state_.turn.oc_state = OpenClosedState::Open;
            events_.emit(ChainEmptiedEvent{});
            return;
        }

        // If there are still pending items, loop back to finalize
        if (state_.chain.hasPending()) continue;

        // Steps 2-3: Execute and Pass
        // Controller of newest item gets Priority.
        // If someone adds an item → restart from Finalize.
        bool item_added = stepExecuteAndPass(query_agent, gen_closed_actions);
        if (item_added) continue; // restart FEPR

        // Step 4: Resolve top item
        stepResolve(query_agent, resolve_spell);

        // After resolve, check chain state:
        if (!state_.chain.exists()) {
            state_.turn.oc_state = OpenClosedState::Open;
            events_.emit(ChainEmptiedEvent{});
            return;
        }
        // If pending items exist (from triggers), loop back to Finalize
        // If no pending, loop back to Execute (newest item controller gets priority)
    }
}

bool ChainManager::stepFinalize(
    std::function<void(const ChainItem&)> resolve_permanent) {

    bool finalized_any = false;

    // Finalize pending items in order (oldest pending first)
    // Use index loop since vector may be modified
    for (size_t i = 0; i < state_.chain.items.size(); ++i) {
        auto& item = state_.chain.items[i];
        if (item.status != ChainItemStatus::Pending) continue;

        item.status = ChainItemStatus::Finalized;
        finalized_any = true;

        events_.emit(ChainItemFinalizedEvent{item.id, item.source,
                                              item.controller});

        // CR 337.1.c: Permanents resolve immediately on finalize.
        // They leave the chain and become game objects on the board.
        if (item.is_permanent) {
            ChainItem resolved_item = item; // copy before erasing
            state_.chain.items.erase(state_.chain.items.begin() +
                                      static_cast<ptrdiff_t>(i));
            --i; // adjust index

            resolve_permanent(resolved_item);

            events_.emit(ChainItemResolvedEvent{resolved_item.id,
                resolved_item.source, resolved_item.controller});
        }
    }

    return finalized_any;
}

bool ChainManager::stepExecuteAndPass(AgentQuery query_agent,
                                      ClosedActionsGen gen_closed_actions) {
    // Controller of newest finalized item gets Priority first (CR 337.1.b.3)
    auto* top = state_.chain.newest();
    assert(top && "stepExecuteAndPass called with empty chain");

    PlayerId first_priority = top->controller;
    state_.turn.players_passed_priority.clear();

    PlayerId current = first_priority;

    constexpr int kMaxPriorityPasses = 10; // safety (2 players × some margin)

    for (int pass = 0; pass < kMaxPriorityPasses; ++pass) {
        state_.turn.priority_holder = current;
        events_.emit(PriorityGrantedEvent{current, false});

        // Generate legal actions — use injected generator if available
        // (handles targeting + affordability), fallback to our own
        auto actions = gen_closed_actions
            ? gen_closed_actions(current)
            : generateClosedStateActions(current);

        // Query agent
        auto chosen = query_agent(current, actions);

        if (chosen.type == IntentType::PassPriority) {
            state_.turn.players_passed_priority.insert(current);

            // Step 3: Check if ALL players have passed
            bool all_passed = true;
            for (auto pid : {PlayerId::Player1, PlayerId::Player2}) {
                if (state_.turn.players_passed_priority.find(pid) ==
                    state_.turn.players_passed_priority.end()) {
                    all_passed = false;
                    break;
                }
            }

            if (all_passed) {
                return false; // All passed → proceed to Resolve
            }

            // Pass to next player in turn order
            current = opponent(current);
        } else if (chosen.type == IntentType::PlayReaction) {
            auto& card = state_.getObject(chosen.card);
            auto& ps = state_.player(current);

            // ── SPELLS: one executor owns every spell play ──
            //
            // GameEngine::executeIntent has no PlayReaction case, so a
            // closed-state spell offer used to be executed by the local path
            // below — a second, thinner copy of executePlaySpell that paid
            // via payCardCost only (no [Flow], no Sandswept Tomb staging),
            // looked for the card in `ps.hand` alone (a trash-replay or
            // [Flow] play was therefore never removed from the trash, and the
            // disposal below pushed a DUPLICATE trash entry), never set
            // `banish_on_leave`, never consumed a granted Flow and never
            // stamped `target_battlefield_restriction`. Routing through
            // GameEngine::executePlaySpell fixes all of those at once,
            // including the facedown reveal (which executePlaySpell now
            // handles: facedown-zone removal, the is_hidden clear,
            // PlayedFromFacedownEvent and play_source = Hidden).
            //
            // executePlaySpell adds the chain item itself and re-enters
            // runChain, which returns immediately while this loop is live
            // (ChainManager::isProcessing). If it added nothing the intent
            // was rejected as illegal — it pays nothing on that path, so
            // priority simply stays where it is rather than restarting FEPR
            // on an unchanged chain.
            if (play_spell_ && card.isSpell()) {
                const size_t before = state_.chain.items.size();
                play_spell_(chosen);
                if (state_.chain.items.size() != before) {
                    return true; // Item added → restart FEPR from Finalize
                }
                events_.logWarn("CHAIN: reaction play of " + card.name +
                                " executed nothing — intent rejected");
                continue;
            }

            // ── NON-SPELL reactions ──
            //
            // Quick-Draw gear, [Ambush] / Rengar units and facedown
            // PERMANENTS revealed as reactions. Play source is derived from
            // the card's zone/hidden-status (Kennen spec §2/addendum #2)
            // BEFORE is_hidden is cleared below. ChainManager can't call
            // GameEngine::playSourceFor, so it uses the shared
            // playSourceForZone helper directly, same as
            // EffectExecutor::playIgnoringCost.
            Intent::PlaySource event_play_source =
                playSourceForZone(card.zone, card.is_hidden);

            if (card.is_hidden) {
                // Playing from facedown — remove from BF facedown zone, no cost
                for (auto& bf : state_.battlefields) {
                    auto fit = std::find(bf.facedown.begin(), bf.facedown.end(),
                                          chosen.card);
                    if (fit != bf.facedown.end()) {
                        bf.facedown.erase(fit);
                        break;
                    }
                }
                card.is_hidden = false;
                card.hidden_at = kInvalidId;
            } else {
                // Normal reaction from hand
                auto it = std::find(ps.hand.begin(), ps.hand.end(), chosen.card);
                if (it != ps.hand.end()) ps.hand.erase(it);

                // Pay cost
                if (pay_cost_) {
                    pay_cost_(current, chosen.card);
                }
            }

            // Track play
            ps.cards_played_this_turn++;
            int energy_spent = (card.card_def_id != kInvalidId)
                ? card_db_.get(card.card_def_id).energy_cost : 0;
            events_.emit(CardPlayedEvent{chosen.card, current,
                card.card_type, ps.cards_played_this_turn, energy_spent,
                event_play_source});

            // Add to chain with targets
            addSpell(chosen.card, current, chosen.targets);

            return true; // Item added → restart FEPR from Finalize
        }
        // Other intent types (ActivateReactionAbility etc.) are Phase 3+
    }

    return false;
}

void ChainManager::stepResolve(
    AgentQuery query_agent,
    std::function<void(const ChainItem&)> resolve_spell) {

    assert(!state_.chain.items.empty() && "stepResolve called with empty chain");

    // Pop the newest (top) item — LIFO. Counter spells (Abandon, Wind Wall,
    // Repulse, Not So Fast, Lilting Lullaby) rely on `items.back()` being
    // the previous item on the chain (their counter target) — keep that
    // invariant by popping FIRST. The popped item is held in
    // `state_.chain.resuming` so a resumable Card can read/write its
    // resume_point across iterations.
    ChainItem resolved = state_.chain.items.back();
    state_.chain.items.pop_back();
    state_.chain.resuming = resolved;

    // Resumable resolution pump. Single-shot cards (the common case) loop
    // exactly once: resolve_spell -> Card::onResolve returns without a
    // pending choice, the while condition fails, we drop through to
    // disposal. Resumable cards (e.g. discard, predict, opponentDiscards)
    // publish a pending choice via `EffectExecutor::requestChoice` and set
    // `resuming->resume_point`; we then query the agent, record the choice
    // for the Card to read on re-entry, and re-call resolve_spell.
    //
    // Factored into a lambda because EVERY execution of the item — the base
    // resolution and each paid [Repeat] tranche below — needs it. The Repeat
    // loop used to call resolve_spell directly, so a resumable card only ever
    // ran its `case 0` branch on a tranche: the effect never happened AND the
    // choice it published stayed active in the executor, to be consumed by
    // whatever card resolved next. Hard Bargain (457) — `[Reaction]` +
    // `[Repeat] [2]`, live in the Closed State — and Called Shot (443) are
    // the shipped cards with that shape.
    auto runResolutionPump = [&]() {
        constexpr int kMaxResumeIterations = 16;
        int iter = 0;
        while (true) {
            if (++iter > kMaxResumeIterations) {
                // Safety bound — a Card stuck in a yield loop should never
                // reach this. We bail out rather than spinning forever.
                assert(false && "ChainManager::stepResolve: resume loop overflow");
                break;
            }

            // Re-invoke resolve_spell from `resuming`. The engine's
            // resolveSpell dispatches through Card::onResolve / onTrigger
            // based on is_spell / is_ability — both paths are reachable here.
            if (state_.chain.resuming->is_spell ||
                state_.chain.resuming->is_ability) {
                resolve_spell(state_.chain.resuming.value());
            }

            if (!executor_ || !executor_->hasPendingChoice()) break;

            auto pending = executor_->consumePendingChoice();
            // Surface the labeled choice request in the trace BEFORE the
            // agent picks. Pairs with the on_decision-callback-driven CHOSE
            // logging downstream; gives a reader the WHY of a MakeChoice
            // decision (e.g. "discard 1 (Lunar Boon)") instead of just the
            // WHAT (e.g. "pick=[Hard Bargain(id=12)]"). Cards that don't pass
            // a label fall back to a generic line.
            events_.logTrace(
                std::string("CHOICE-REQUEST: ") +
                (pending.label.empty() ? "MakeChoice" : pending.label) +
                " [" + std::to_string(pending.legal.size()) + " options] (" +
                toString(pending.player) + ")");
            Intent choice = query_agent(pending.player, pending.legal);
            executor_->recordChoice(std::move(choice));
            // Loop back — resolve_spell re-invokes onResolve / onTrigger; the
            // Card reads the recorded choice via `executor.takeChoice()` in
            // its case ≥1 branch and continues past resume_point.
        }
    };

    runResolutionPump();

    // Repeat (CR 820): if `repeats_paid > 0`, re-run the item that many extra
    // times. Each re-run reads the same item from state_.chain.resuming
    // (still populated) — Cards see the same chain item and execute their
    // effect again — and goes through the SAME pump as the base resolution,
    // so a resumable card can yield and consume a choice on every tranche
    // (CR 820.2, Make Relevant Choices). Targets and the rest of the item are
    // re-used as-is (the simplification noted on ChainItem::repeats_paid);
    // only resume_point / resume_data are reset, so each tranche starts at
    // the card's `case 0` branch with a clean slate.
    if (state_.chain.resuming.has_value()) {
        for (int r = 0; r < state_.chain.resuming->repeats_paid; ++r) {
            state_.chain.resuming->resume_point = 0;
            state_.chain.resuming->resume_data.clear();
            runResolutionPump();
        }
    }

    // Read the (possibly-mutated) resolving item back out and dispose.
    resolved = std::move(state_.chain.resuming.value());
    state_.chain.resuming.reset();

    if (resolved.is_spell) {
        // Spell goes to controller's trash after resolving (CR 359.3) —
        // UNLESS it was played for its Flow cost, in which case leaving the
        // chain (and it wasn't instructed by its own execution) banishes it
        // instead (CR 829.1.b.1).
        if (state_.objectExists(resolved.source)) {
            auto& spell_obj = state_.getObject(resolved.source);
            spell_obj.location = std::nullopt;
            ZoneType destination = ZoneType::Trash;
            if (resolved.banish_on_leave) {
                destination = ZoneType::Banishment;
                spell_obj.zone = ZoneType::Banishment;
                spell_obj.is_empowered = false;  // CR 441.1.a
                state_.player(resolved.controller).banishment.push_back(resolved.source);
                events_.logTrace("FLOW: " + spell_obj.name + " banished");
            } else {
                spell_obj.zone = ZoneType::Trash;
                state_.player(resolved.controller).trash.push_back(resolved.source);
            }

            events_.emit(SpellResolvedEvent{resolved.source, resolved.controller});
            events_.emit(LeftBoardEvent{resolved.source, resolved.controller,
                CardType::Spell, BaseLocation{resolved.controller},
                destination, false});
        }
    }
    // Triggered/activated abilities leave their source on the board — no
    // zone changes here. The chain item itself is gone (popped above).

    events_.emit(ChainItemResolvedEvent{resolved.id, resolved.source,
                                         resolved.controller});
}

std::vector<Intent> ChainManager::generateClosedStateActions(
    PlayerId player) const {

    std::vector<Intent> actions;

    // Always can pass priority
    actions.push_back(Intent::passPriority(player));

    // Can play Reaction spells from hand (CR 309.1.a)
    auto& ps = state_.player(player);
    for (auto card_id : ps.hand) {
        auto& card = state_.getObject(card_id);
        if (!card.isSpell()) continue;
        if (!card.keywords.has(Keyword::Reaction)) continue;

        // Check affordability via injected callback from GameEngine
        if (can_afford_ && !can_afford_(player, card_id)) continue;

        Intent play;
        play.type = IntentType::PlayReaction;
        play.player = player;
        play.card = card_id;
        // Targets will be filled by the agent from legal targets
        actions.push_back(play);
    }

    return actions;
}

} // namespace riftbound
