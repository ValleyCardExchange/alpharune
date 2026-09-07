#pragma once
/// @file chain_manager.h
/// Chain resolution manager — implements the FEPR loop.
///
/// FEPR = Finalize → Execute → Pass → Resolve
/// This is the core mechanic enabling spells, reactions, triggered abilities,
/// and activated abilities. Items stack on the chain and resolve LIFO.
///
/// Rules references: CR 335-340, 309, 312, 337.1.c, 354, 359

#include "core/card_db.h"
#include "core/events.h"
#include "core/game_state.h"
#include "core/intent.h"

#include <functional>
#include <vector>

namespace riftbound {

class EffectExecutor;

/// Callback to query an agent for an action.
/// Given a player and their legal actions, returns their chosen action.
using AgentQuery = std::function<Intent(PlayerId, const std::vector<Intent>&)>;

class ChainManager {
public:
    ChainManager(GameState& state, EventBus& events, const CardDB& card_db);

    /// Add a spell to the chain as a Pending Item.
    /// Returns the chain item ID.
    ChainItemId addSpell(GameObjectId spell_obj, PlayerId controller,
                         const std::vector<GameObjectId>& targets);

    /// Add a permanent (unit/gear) to the chain as a Pending Item.
    /// Permanents resolve immediately on finalize (CR 337.1.c).
    ///
    /// `targets` are the objects the PLAY chose, carried onto the item so
    /// GameEngine::resolvePermanent can read them back: a [Quick-Draw] gear
    /// names the unit it attaches to as it enters (CR 819). Every other
    /// permanent play leaves it empty, which is the default.
    ChainItemId addPermanent(GameObjectId card_obj, PlayerId controller,
                             const std::vector<GameObjectId>& targets = {});

    /// Add a triggered/activated ability to the chain. Source stays on
    /// board (not moved to chain zone). `is_activated` distinguishes
    /// "player chose to activate (e.g. [E]:)" from "event fired" — set
    /// it true from the ActivateAbility intent handler in the engine,
    /// false (default) from TriggerManager. Drives whether stepResolve
    /// dispatches through Card::onActivate or Card::onTrigger.
    ChainItemId addAbility(GameObjectId source, PlayerId controller,
                           CardDefId def_id,
                           const std::vector<GameObjectId>& targets = {},
                           bool is_activated = false,
                           int ability_index = 0);

    /// Callback to generate legal closed-state actions for a player.
    /// Injected from GameEngine so targeting and affordability are handled.
    using ClosedActionsGen = std::function<std::vector<Intent>(PlayerId)>;

    /// Run the FEPR loop until the chain is empty.
    /// query_agent is called whenever a player with priority must act.
    /// resolve_permanent is called when a permanent finalizes (bypasses chain).
    /// resolve_spell is called when a spell resolves.
    /// gen_closed_actions generates legal actions during Closed State.
    void processFEPR(
        AgentQuery query_agent,
        std::function<void(const ChainItem&)> resolve_permanent,
        std::function<void(const ChainItem&)> resolve_spell,
        ClosedActionsGen gen_closed_actions = nullptr
    );

    /// Check if the chain currently exists.
    bool chainExists() const { return state_.chain.exists(); }

    /// Set affordability check callback (injected from GameEngine).
    using AffordCheck = std::function<bool(PlayerId, GameObjectId)>;
    void setAffordCheck(AffordCheck check) { can_afford_ = std::move(check); }

    /// Set cost payment callback (injected from GameEngine). Used only by the
    /// bare-ChainManager spell fallback in stepExecuteAndPass — the injected
    /// executors below pay their own costs.
    using PayCost = std::function<bool(PlayerId, GameObjectId)>;
    void setPayCost(PayCost pay) { pay_cost_ = std::move(pay); }

    /// Set the play executors (injected from GameEngine —
    /// GameEngine::executePlaySpell and GameEngine::executePlayCard).
    ///
    /// Closed-State [Reaction] offers are answered HERE, in
    /// stepExecuteAndPass: `GameEngine::executeIntent` also has a
    /// PlayReaction case, but that one serves the SHOWDOWN decision path
    /// (resolveShowdownDecision) and never sees a closed-state offer, because
    /// nothing in this class calls executeIntent. The two paths are disjoint,
    /// and both end in the SAME two executors — which is the point of these
    /// callbacks.
    ///
    /// Both halves used to be hand-rolled here instead, and both were wrong.
    /// The spell copy paid via payCardCost only (no [Flow], no Sandswept Tomb
    /// staging), looked for the card in `PlayerState::hand` alone (so a
    /// trash-replay or [Flow] play was never removed from the trash and the
    /// disposal pushed a DUPLICATE trash entry), never set `banish_on_leave`,
    /// never consumed a granted Flow and never stamped
    /// `target_battlefield_restriction`. The non-spell copy ended in
    /// `addSpell`, which sets `is_spell` — so a [Quick-Draw] gear, an
    /// [Ambush] / Rengar unit or a facedown PERMANENT was paid for, skipped
    /// CR 337.1.c's finalize-time resolution, ran through Card::onResolve (a
    /// no-op on a permanent) and was disposed into the TRASH instead of
    /// reaching the board.
    ///
    /// Each executor adds its own chain item and re-enters
    /// GameEngine::runChain, which returns immediately while this loop is live
    /// (see isProcessing).
    using PlaySpell = std::function<void(const Intent&)>;
    void setPlaySpell(PlaySpell play) { play_spell_ = std::move(play); }

    /// @see setPlaySpell — the non-spell half (units, gear, facedown
    /// permanents), routed to GameEngine::executePlayCard.
    using PlayCard = std::function<void(const Intent&)>;
    void setPlayCard(PlayCard play) { play_card_ = std::move(play); }

    /// True while processFEPR is running.
    ///
    /// GameEngine::executePlaySpell ends by calling GameEngine::runChain, and
    /// the routed closed-state play calls it from INSIDE this loop. The
    /// engine consults this so the nested call adds its chain item and
    /// returns instead of starting a second FEPR loop that would resolve the
    /// chain out from under the outer one (stepExecuteAndPass already
    /// restarts at Finalize once an item is added).
    bool isProcessing() const { return processing_; }

    /// Inject the EffectExecutor so stepResolve can detect mid-resolution
    /// pending choices published by Card::onResolve / onTrigger via
    /// `requestChoice`. Required before processFEPR runs.
    void setEffectExecutor(EffectExecutor* exec) { executor_ = exec; }

    /// Generate legal actions for a player with priority in Closed State.
    std::vector<Intent> generateClosedStateActions(PlayerId player) const;

private:
    GameState& state_;
    EventBus& events_;
    const CardDB& card_db_;
    AffordCheck can_afford_;
    PayCost pay_cost_;
    PlaySpell play_spell_;
    PlayCard play_card_;
    EffectExecutor* executor_ = nullptr;
    bool processing_ = false;

    /// Step 1: Finalize all pending items in order.
    /// Returns true if any items were finalized.
    bool stepFinalize(
        std::function<void(const ChainItem&)> resolve_permanent
    );

    /// Steps 2-3: Execute (player acts) and Pass (check all passed).
    /// Returns true if an item was added (restart from Finalize).
    bool stepExecuteAndPass(AgentQuery query_agent,
                            ClosedActionsGen gen_closed_actions);

    /// Step 4: Resolve the top item on the chain. Drives a resumable loop:
    /// while the resolving Card publishes a pending choice via
    /// `EffectExecutor::requestChoice`, query the agent + record the
    /// choice + re-invoke resolve_spell so the Card's next switch case
    /// runs. Single-shot resolution (the common case) loops once.
    void stepResolve(AgentQuery query_agent,
                     std::function<void(const ChainItem&)> resolve_spell);
};

} // namespace riftbound
