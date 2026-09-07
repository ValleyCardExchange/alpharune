#include "cards/card.h"
#include "core/game_state.h"
#include "core/intent.h"
#include "engine/effect_executor.h"
#include "core/events.h"

namespace riftbound {

int Card::confirmOptional(CardContext& ctx, const std::string& label,
                           const std::function<bool()>& still_legal) {
    auto& ri_opt = ctx.state.chain.resuming;
    if (!ri_opt.has_value()) {
        // Card invoked outside the chain (test/direct invocation). The
        // resumable yes/no path can't be driven here — default to YES if
        // the precondition holds. Tests that want to exercise the
        // "no"-branch must use the chain-driven path via
        // CardTestFixture::driveResumableTrigger (which sets resuming).
        return still_legal() ? 1 : 0;
    }
    auto& ri = *ri_opt;

    if (ri.resume_point == 0) {
        // Validate-before-prompt. CR principle: never offer a choice that
        // can't legally be acted on. If preconditions failed (e.g. the
        // spell to banish isn't in trash anymore), skip silently.
        // The trace tag distinguishes "skipped because illegal" from
        // "agent declined" — important for V&V.
        if (!still_legal()) {
            ctx.events.logTrace("MAY_NOT_OFFERED: " + label +
                                 " (precondition failed at prompt time)");
            ri.resume_point = 2;  // mark "done, do not re-enter"
            return 0;
        }
        // Publish a 2-option choice via Intent::chosen_value (no = 0, yes = 1).
        //
        // The action_vocab encoder reserves a slot per chosen_value, so no
        // and yes encode to DISTINCT OpenSpiel actions — the policy head can
        // express preference and the random agent picks ~50/50. The earlier
        // encoding stuffed a sentinel GameObjectId into chosen_objects which
        // collided with slot 0 (empty chosen_objects) in the encoder; result
        // was OpenSpiel deduping the two intents and never showing "yes" to
        // any agent. Caused 100% MAY_DECLINED on Virtuoso. See
        // action_vocab.h "kNumIntChoices" + comment block.
        Intent no_choice;
        no_choice.type = IntentType::MakeChoice;
        no_choice.player = ctx.controller;
        no_choice.chosen_value = 0;
        no_choice.choice_label = "No — " + label;
        Intent yes_choice;
        yes_choice.type = IntentType::MakeChoice;
        yes_choice.player = ctx.controller;
        yes_choice.chosen_value = 1;
        yes_choice.choice_label = "Yes — " + label;

        ctx.events.logTrace("MAY_PROMPT: " + label +
                             " (P=" + std::string(toString(ctx.controller)) +
                             ", deferred to agent)");
        ctx.executor.requestChoice(ctx.controller,
                                    {no_choice, yes_choice},
                                    "may: " + label);
        ri.resume_point = 1;
        return -1;  // caller returns, chain manager re-enters after agent
    }

    if (ri.resume_point == 1) {
        auto picked = ctx.executor.takeChoice();
        // Yes = chosen_value == 1 (set explicitly by yes_choice above).
        // Anything else (no_choice with chosen_value=0, no recorded choice,
        // legacy intents without chosen_value) reads as a decline.
        bool said_yes = picked.has_value() &&
                        picked->chosen_value.has_value() &&
                        *picked->chosen_value == 1;
        if (!said_yes) {
            ctx.events.logTrace("MAY_DECLINED: " + label);
            ri.resume_point = 2;
            return 0;
        }
        // Re-validate at use time. State may have shifted while the
        // agent decided — if the precondition no longer holds, the
        // "yes" can't be honored. CR-correct silent-fail behavior.
        if (!still_legal()) {
            ctx.events.logTrace("MAY_INVALIDATED: " + label +
                                 " (state shifted between agent yes "
                                 "and execution; silent fail)");
            ri.resume_point = 2;
            return 0;
        }
        ctx.events.logTrace("MAY_ACCEPTED: " + label);
        ri.resume_point = 2;  // commit; caller proceeds to do the work
        return 1;
    }

    // resume_point >= 2: already confirmed and the card is in its
    // post-confirmation work. Return 1 so the caller re-enters its
    // execute branch on subsequent loop iterations (in case the card
    // itself requests further choices and is re-entered).
    return 1;
}

int Card::pickXAmount(CardContext& ctx, const std::string& label,
                       int min, int max) {
    auto& ri_opt = ctx.state.chain.resuming;
    if (!ri_opt.has_value()) {
        // Direct invocation (no chain) — default to max for test/legacy
        // paths. Real-game flow goes through the chain so this branch
        // only hits in test scenarios that bypass driveResumable.
        return max;
    }
    auto& ri = *ri_opt;

    if (ri.resume_point == 0) {
        if (max < min) {
            ctx.events.logTrace("X_NOT_OFFERED: " + label +
                                 " (max=" + std::to_string(max) +
                                 " below min=" + std::to_string(min) +
                                 ")");
            ri.resume_point = 2;
            ri.resume_data.push_back(min);
            return min;
        }
        // Publish (max - min + 1) MakeChoice intents, one per legal X.
        // Encode X via Intent::chosen_value — each X gets its OWN
        // OpenSpiel action slot (action_vocab reserves a kNumIntChoices-
        // wide range for int-coded MakeChoice answers), so MCTS /
        // AlphaZero can express preference between X values.
        std::vector<Intent> options;
        for (int x = min; x <= max; ++x) {
            Intent intent;
            intent.type = IntentType::MakeChoice;
            intent.player = ctx.controller;
            intent.chosen_value = x;
            intent.choice_label = label + ": " + std::to_string(x);
            options.push_back(intent);
        }
        ctx.events.logTrace("X_PROMPT: " + label + " (range " +
                             std::to_string(min) + ".." +
                             std::to_string(max) + ", " +
                             std::to_string(options.size()) +
                             " options) [agent decides]");
        ctx.executor.requestChoice(ctx.controller, std::move(options),
                                    "X: " + label);
        ri.resume_point = 1;
        return -1;
    }

    if (ri.resume_point == 1) {
        auto picked = ctx.executor.takeChoice();
        int x = min;
        if (picked.has_value() && picked->chosen_value.has_value()) {
            x = *picked->chosen_value;
        }
        if (x < min) x = min;
        if (x > max) x = max;
        ctx.events.logTrace("X_PICKED: " + label + " = " +
                             std::to_string(x));
        ri.resume_data.push_back(x);
        ri.resume_point = 2;
        return x;
    }

    // resume_point >= 2: previously chosen X is stashed in resume_data[0].
    if (!ri.resume_data.empty()) return ri.resume_data[0];
    return min;
}

int Card::pickMode(CardContext& ctx, const std::string& label, int num_modes,
                    const std::vector<std::string>& mode_labels,
                    uint32_t legal_modes) {
    auto& ri_opt = ctx.state.chain.resuming;

    // Build the legal-mode index list once. The bitmask filters out
    // modes that don't apply right now (e.g. damage modes when no
    // legal target is present).
    std::vector<int> legal;
    legal.reserve(num_modes);
    for (int m = 0; m < num_modes && m < 32; ++m) {
        if (legal_modes & (1u << m)) legal.push_back(m);
    }

    if (!ri_opt.has_value()) {
        // Direct invocation (no chain). Default to legal[0] or -2.
        if (legal.empty()) return -2;
        return legal.front();
    }
    auto& ri = *ri_opt;

    // resume_point reservations: 3 (publish) → 4 (consume) → 5 (done).
    // We use a separate slot from pickXAmount (0/1/2) so they coexist.

    if (ri.resume_point < 3) ri.resume_point = 3;

    if (ri.resume_point == 3) {
        if (legal.empty()) {
            ctx.events.logTrace("MODE_NOT_OFFERED: " + label +
                                 " (no legal modes after filtering)");
            ri.resume_point = 5;
            while (ri.resume_data.size() < 2) ri.resume_data.push_back(0);
            ri.resume_data[1] = -2;
            return -2;
        }
        // Always publish a choice, even if only one mode is legal. The
        // agent still records the decision (matches the engine-wide
        // rule that every decision point should surface to the agent
        // for training fidelity — see cost-payment loop, pickTarget).
        std::vector<Intent> options;
        std::string label_summary;
        for (int m : legal) {
            Intent i;
            i.type = IntentType::MakeChoice;
            i.player = ctx.controller;
            // Encode the mode index via chosen_value — each mode gets its
            // own action_vocab slot. Previously this stuffed (m+1) into
            // chosen_objects as a fake GameObjectId, which collided with
            // real objects whose CardDefId happened to share that slot
            // (e.g. obj=1 = Bounty Hunter), polluting the policy head's
            // view of what each slot represents.
            i.chosen_value = m;
            std::string mode_name = (m < static_cast<int>(mode_labels.size()))
                ? mode_labels[m]
                : ("mode#" + std::to_string(m));
            i.choice_label = mode_name;
            options.push_back(i);
            if (!label_summary.empty()) label_summary += "/";
            label_summary += mode_name;
        }
        ctx.events.logTrace("MODE_PROMPT: " + label + " [" + label_summary +
                             "] (agent decides among " +
                             std::to_string(legal.size()) + " modes)");
        ctx.executor.requestChoice(ctx.controller, std::move(options),
                                    "mode: " + label);
        ri.resume_point = 4;
        return -1;
    }

    if (ri.resume_point == 4) {
        auto picked = ctx.executor.takeChoice();
        int m = legal.empty() ? 0 : legal.front();
        if (picked.has_value() && picked->chosen_value.has_value()) {
            m = *picked->chosen_value;
        }
        if (m < 0) m = 0;
        if (m >= num_modes) m = num_modes - 1;
        ctx.events.logTrace("MODE_PICKED: " + label + " = mode#" +
                             std::to_string(m) +
                             (m < static_cast<int>(mode_labels.size())
                                ? " (" + mode_labels[m] + ")" : ""));
        while (ri.resume_data.size() < 2) ri.resume_data.push_back(0);
        ri.resume_data[1] = m;
        ri.resume_point = 5;
        return m;
    }

    // resume_point >= 5: previously chosen mode in resume_data[1].
    if (ri.resume_data.size() >= 2) return ri.resume_data[1];
    return legal.empty() ? -2 : legal.front();
}

GameObjectId Card::pickTarget(CardContext& ctx, const std::string& label,
                                const std::vector<GameObjectId>& legal_targets) {
    auto& ri_opt = ctx.state.chain.resuming;

    if (!ri_opt.has_value()) {
        // Direct invocation (test / no chain). Default to first legal
        // target or kInvalidId. Real-game flow goes through the chain
        // so this branch only hits in tests that bypass the resume
        // machinery.
        return legal_targets.empty() ? kInvalidId : legal_targets.front();
    }
    auto& ri = *ri_opt;

    // ── Sandswept Tomb (792): the restricted play's commitment ──
    //
    // "Each spell that chooses one or more units here that are friendly to it
    // costs [A] less." A spell that picks its target at RESOLVE time was
    // offered a second, discounted intent that COMMITTED it to choosing at a
    // named battlefield; executePlaySpell paid the discount and stamped the
    // battlefield onto the chain item. Enforce that commitment here, in the
    // one place every resolve-time single-target pick goes through, so no
    // card needs to know about the Tomb: narrow the legal list to friendly
    // units at that battlefield before publishing it. If nothing survives,
    // the empty-list branch below handles it exactly like a target that
    // vanished before resolution.
    std::vector<GameObjectId> restricted;
    const std::vector<GameObjectId>* legal_p = &legal_targets;
    if (ri.target_battlefield_restriction.has_value()) {
        const auto bf = *ri.target_battlefield_restriction;
        for (auto t : legal_targets) {
            if (!ctx.state.objectExists(t)) continue;
            const auto& obj = ctx.state.getObject(t);
            if (!obj.isUnit() || obj.controller != ctx.controller) continue;
            auto at = obj.battlefieldId();
            if (at && *at == bf) restricted.push_back(t);
        }
        legal_p = &restricted;
    }
    const std::vector<GameObjectId>& legal = *legal_p;

    // resume_point reservations: 6 (publish) → 7 (consume) → 8 (done).
    // Distinct from pickXAmount (0/1/2 + data[0]) and pickMode
    // (3/4/5 + data[1]) so all three can coexist in one Card's
    // onResolve. Uses resume_data[2] to stash the picked id across
    // re-entry.
    if (ri.resume_point < 6) ri.resume_point = 6;

    if (ri.resume_point == 6) {
        if (legal.empty()) {
            ctx.events.logTrace("TGT_NOT_OFFERED: " + label +
                                 " (no legal targets at resolve time)");
            ri.resume_point = 8;
            while (ri.resume_data.size() < 3) ri.resume_data.push_back(0);
            ri.resume_data[2] = static_cast<int32_t>(kInvalidId);
            return kInvalidId;
        }
        // Always publish a choice, even if only one target is legal —
        // the agent still records the decision. (See pickMode + the
        // cost-payment loop for the same rule. Auto-picking single
        // targets was hiding meaningful decision points like Bounty
        // Hunter's Ganking grant when only one unit was on board.)
        // Publish one MakeChoice per legal target. action_vocab keys
        // MakeChoice slots by chosen_objects[0]'s card_def_id (Phase
        // 5g), so the policy head sees one slot per distinct
        // target-card-type. Two friendly Bounty Hunters alias to the
        // Bounty Hunter slot — known limitation, same as discard /
        // predict picks today.
        std::vector<Intent> options;
        options.reserve(legal.size());
        std::string label_summary;
        for (auto t : legal) {
            Intent i;
            i.type = IntentType::MakeChoice;
            i.player = ctx.controller;
            i.chosen_objects = { t };
            options.push_back(std::move(i));
            if (!label_summary.empty()) label_summary += "/";
            label_summary += ctx.state.objectExists(t)
                ? ctx.state.getObject(t).name
                : std::string("?");
        }
        ctx.events.logTrace("TGT_PROMPT: " + label + " [" + label_summary +
                             "] (agent decides among " +
                             std::to_string(legal.size()) +
                             " targets)");
        ctx.executor.requestChoice(ctx.controller, std::move(options),
                                    "target: " + label);
        ri.resume_point = 7;
        return kInvalidId;
    }

    if (ri.resume_point == 7) {
        auto picked = ctx.executor.takeChoice();
        GameObjectId t = legal.empty()
            ? kInvalidId : legal.front();
        // TODO: the answer is taken on trust — it is not re-checked against
        // `legal`. Sandswept Tomb's [A] discount is charged for a COMMITMENT
        // to choose at the restricted battlefield, and `legal` above is what
        // enforces it; an agent that answers off-list would keep the discount
        // and dodge the commitment. Every in-tree agent answers from the
        // published list, so this is a trust boundary, not a live bug.
        if (picked.has_value() && !picked->chosen_objects.empty()) {
            t = picked->chosen_objects.front();
        }
        std::string name = ctx.state.objectExists(t)
            ? ctx.state.getObject(t).name
            : std::string("?");
        ctx.events.logTrace("TGT_PICKED: " + label + " = " + name +
                             " (id=" + std::to_string(t) + ")");
        while (ri.resume_data.size() < 3) ri.resume_data.push_back(0);
        ri.resume_data[2] = static_cast<int32_t>(t);
        ri.resume_point = 8;
        return t;
    }

    // resume_point >= 8: previously chosen target in resume_data[2].
    if (ri.resume_data.size() >= 3) {
        return static_cast<GameObjectId>(ri.resume_data[2]);
    }
    return legal.empty() ? kInvalidId : legal.front();
}

std::pair<GameObjectId, GameObjectId> Card::pickTargetPair(
    CardContext& ctx, const std::string& label,
    const std::vector<GameObjectId>& legal_a,
    const std::function<std::vector<GameObjectId>(GameObjectId)>& legal_b_fn) {

    auto& ri_opt = ctx.state.chain.resuming;
    if (!ri_opt.has_value()) {
        // Direct invocation (test / no chain). Default to first
        // legal pair or kInvalidId-pair.
        if (legal_a.empty()) return {kInvalidId, kInvalidId};
        auto a = legal_a.front();
        auto lb = legal_b_fn(a);
        if (lb.empty()) return {a, kInvalidId};
        return {a, lb.front()};
    }
    auto& ri = *ri_opt;

    // ── Sandswept Tomb (792): the restricted play's commitment, for a PAIR ──
    //
    // The single-pick case (Card::pickTarget) just narrows its one list. A
    // pair is harder: the commitment is "at least ONE of the two chosen units
    // is a friendly unit at the named battlefield", and the twenty-odd callers
    // build their A and B lists in card-specific ways — for Star-Crossed A is
    // the friendly list, for Switcheroo A is every unit at any battlefield.
    // Narrowing both lists would be wrong (it would force an ENEMY pick to
    // stand at the Tomb too); narrowing only A would let a card whose friendly
    // units live in B keep the discount for free.
    //
    // So the rule is caller-agnostic and two-branched, and needs no per-card
    // knowledge:
    //   • A keeps a candidate that either IS a friendly unit at the
    //     battlefield, or can still REACH one through its own B list. Anything
    //     from which the commitment can never be met is dropped.
    //   • B is left completely alone when the chosen A already satisfies the
    //     commitment; otherwise B is narrowed to friendly units at the
    //     battlefield — the second pick is then the only chance to meet it.
    // Either way, a completed pair contains a friendly unit at that
    // battlefield. Empty lists fall through to the existing
    // TGT_PAIR_NOT_OFFERED branches, exactly like a target that vanished.
    const bool tomb_restricted = ri.target_battlefield_restriction.has_value();
    auto friendlyAtRestrictedBf = [&](GameObjectId id) {
        if (!tomb_restricted) return false;
        if (!ctx.state.objectExists(id)) return false;
        const auto& o = ctx.state.getObject(id);
        if (!o.isUnit() || o.controller != ctx.controller) return false;
        auto at = o.battlefieldId();
        return at.has_value() && *at == *ri.target_battlefield_restriction;
    };
    std::vector<GameObjectId> restricted_a;
    const std::vector<GameObjectId>* a_source = &legal_a;
    if (tomb_restricted) {
        for (auto a : legal_a) {
            if (friendlyAtRestrictedBf(a)) { restricted_a.push_back(a); continue; }
            for (auto b : legal_b_fn(a)) {
                if (!friendlyAtRestrictedBf(b)) continue;
                restricted_a.push_back(a);
                break;
            }
        }
        a_source = &restricted_a;
    }
    const std::vector<GameObjectId>& legal_a_use = *a_source;
    auto legalB = [&](GameObjectId a) {
        auto lb = legal_b_fn(a);
        if (!tomb_restricted || friendlyAtRestrictedBf(a)) return lb;
        std::vector<GameObjectId> out;
        for (auto b : lb)
            if (friendlyAtRestrictedBf(b)) out.push_back(b);
        return out;
    };

    // resume_point reservations:
    //   9  = publish first target
    //   10 = consume first target
    //   11 = publish second target (uses picked_a from resume_data[3])
    //   12 = consume second target
    //   13 = done
    // resume_data[3] = picked first target id (int32 cast)
    // resume_data[4] = picked second target id (int32 cast)
    if (ri.resume_point < 9) ri.resume_point = 9;

    while (ri.resume_data.size() < 5) ri.resume_data.push_back(0);

    // ── Step 1: publish first target ──
    if (ri.resume_point == 9) {
        if (legal_a_use.empty()) {
            ctx.events.logTrace("TGT_PAIR_NOT_OFFERED: " + label +
                                 " (no legal first targets at resolve time)");
            ri.resume_data[3] = static_cast<int32_t>(kInvalidId);
            ri.resume_data[4] = static_cast<int32_t>(kInvalidId);
            ri.resume_point = 13;
            return {kInvalidId, kInvalidId};
        }
        // Always publish a choice, even with a single legal first
        // target — the agent records the decision (engine-wide rule).
        {
            std::vector<Intent> options;
            options.reserve(legal_a_use.size());
            std::string summary;
            for (auto t : legal_a_use) {
                Intent i;
                i.type = IntentType::MakeChoice;
                i.player = ctx.controller;
                i.chosen_objects = { t };
                options.push_back(std::move(i));
                if (!summary.empty()) summary += "/";
                summary += ctx.state.objectExists(t)
                    ? ctx.state.getObject(t).name : std::string("?");
            }
            ctx.events.logTrace("TGT_PAIR_PROMPT_A: " + label + " [" +
                                 summary + "] (agent picks 1st of 2)");
            ctx.executor.requestChoice(ctx.controller, std::move(options),
                                        "target A: " + label);
            ri.resume_point = 10;
            return {kInvalidId, kInvalidId};
        }
    }

    if (ri.resume_point == 10) {
        auto picked = ctx.executor.takeChoice();
        GameObjectId a = legal_a_use.empty() ? kInvalidId : legal_a_use.front();
        // TODO: the answer is taken on trust — it is not re-checked against
        // `legal_a_use`, which is where Sandswept Tomb's restricted-offer
        // commitment lives for the A pick (the discount was already paid for
        // it). An agent answering off-list would keep the discount and dodge
        // the commitment. Trust boundary, not a live bug: every in-tree agent
        // answers from the published list.
        if (picked.has_value() && !picked->chosen_objects.empty()) {
            a = picked->chosen_objects.front();
        }
        std::string name = ctx.state.objectExists(a)
            ? ctx.state.getObject(a).name : std::string("?");
        ctx.events.logTrace("TGT_PAIR_PICKED_A: " + label + " = " +
                             name + " (id=" + std::to_string(a) + ")");
        ri.resume_data[3] = static_cast<int32_t>(a);
        ri.resume_point = 11;
        // Fall through to second-target publish.
    }

    // ── Step 2: publish second target, filtered by picked_a ──
    if (ri.resume_point == 11) {
        GameObjectId a = static_cast<GameObjectId>(ri.resume_data[3]);
        auto legal_b = legalB(a);
        if (legal_b.empty()) {
            ctx.events.logTrace("TGT_PAIR_NOT_OFFERED: " + label +
                                 " (no legal second targets after A picked)");
            ri.resume_data[4] = static_cast<int32_t>(kInvalidId);
            ri.resume_point = 13;
            return {a, kInvalidId};
        }
        // Always publish a choice, even with a single legal second
        // target — the agent records the decision (engine-wide rule).
        std::vector<Intent> options;
        options.reserve(legal_b.size());
        std::string summary;
        for (auto t : legal_b) {
            Intent i;
            i.type = IntentType::MakeChoice;
            i.player = ctx.controller;
            i.chosen_objects = { t };
            options.push_back(std::move(i));
            if (!summary.empty()) summary += "/";
            summary += ctx.state.objectExists(t)
                ? ctx.state.getObject(t).name : std::string("?");
        }
        ctx.events.logTrace("TGT_PAIR_PROMPT_B: " + label + " [" +
                             summary + "] (agent picks 2nd of 2)");
        ctx.executor.requestChoice(ctx.controller, std::move(options),
                                    "target B: " + label);
        ri.resume_point = 12;
        return {a, kInvalidId};
    }

    if (ri.resume_point == 12) {
        GameObjectId a = static_cast<GameObjectId>(ri.resume_data[3]);
        auto picked = ctx.executor.takeChoice();
        auto legal_b = legalB(a);
        GameObjectId b = legal_b.empty() ? kInvalidId : legal_b.front();
        // TODO: the answer is taken on trust — it is not re-checked against
        // `legal_b`, which is where Sandswept Tomb's commitment lands when
        // the A pick did not itself satisfy it (the narrowing that made the
        // discount honest). An agent answering off-list would keep the
        // discount and dodge the commitment. Trust boundary, not a live bug:
        // every in-tree agent answers from the published list.
        if (picked.has_value() && !picked->chosen_objects.empty()) {
            b = picked->chosen_objects.front();
        }
        std::string name = ctx.state.objectExists(b)
            ? ctx.state.getObject(b).name : std::string("?");
        ctx.events.logTrace("TGT_PAIR_PICKED_B: " + label + " = " +
                             name + " (id=" + std::to_string(b) + ")");
        ri.resume_data[4] = static_cast<int32_t>(b);
        ri.resume_point = 13;
        return {a, b};
    }

    // resume_point >= 13: return the previously-stashed pair.
    return {static_cast<GameObjectId>(ri.resume_data[3]),
            static_cast<GameObjectId>(ri.resume_data[4])};
}

std::vector<GameObjectId> Card::enumerateLegalTargets(
    const GameState& state, PlayerId controller) const {

    auto reqs = getTargetRequirements();
    if (reqs.count == 0) return {};

    std::vector<GameObjectId> targets;

    for (auto& [id, obj] : state.objects) {
        // Must be on the board (has a location)
        if (!obj.location.has_value()) continue;

        // Type filter
        if (reqs.must_be_unit && !obj.isUnit()) continue;
        if (reqs.must_be_gear && !obj.isGear()) continue;

        // Ownership filter
        if (reqs.must_be_enemy && obj.controller == controller) continue;
        if (reqs.must_be_friendly && obj.controller != controller) continue;

        // Targeting protection (CR 700.x — "can't be chosen by enemy
        // spells and abilities"). Filtered when the candidate is an
        // enemy of `controller`. Friendly targeting still works. Flag
        // is populated by the engine during cleanup/aura-recalc from
        // each card's Card::canBeChosenByEnemy() override.
        if (obj.controller != controller && obj.untargetable_by_enemy) continue;

        // Location filter
        if (reqs.must_be_at_battlefield && !obj.isAtBattlefield()) continue;

        // Stat filter
        if (reqs.max_might > 0 && obj.current_might > reqs.max_might) continue;

        targets.push_back(id);
    }

    return targets;
}

bool Card::hasLegalTargets(const GameState& state,
                            PlayerId controller) const {
    auto reqs = getTargetRequirements();
    if (reqs.count == 0) return true;
    if (reqs.optional) return true;
    return !enumerateLegalTargets(state, controller).empty();
}

// Multi-ability default: wrap the legacy single-ability surface into a
// one-element vector when hasActivatedAbility() is true. Cards with N
// activated abilities override this directly. (Phase 6r)
std::vector<ActivatedAbility> Card::activatedAbilities() const {
    if (!hasActivatedAbility()) return {};
    return { ActivatedAbility{
        .cost = getActivationCost(),
        .targets = getTargetRequirements(),
        .is_action = isActionAbility(),
        .is_reaction = isReactionAbility(),
        // Single-ability cards don't use the multi-ability target-defer
        // hook today; opt in by overriding activatedAbilities() directly.
        .needs_activation_time_target = false,
    } };
}

} // namespace riftbound
