#include "corpus_evaluator.h"

#include "core/game_object.h"
#include "core/game_state.h"
#include "core/types.h"

#include <algorithm>

namespace riftbound {

std::pair<double, double> corpusEvaluate(const GameState& state) {
    return corpusEvaluate(state, CorpusWeights{});
}

std::pair<double, double> corpusEvaluate(const GameState& state,
                                         const CorpusWeights& weights) {
    const auto& p1 = state.player(PlayerId::Player1);
    const auto& p2 = state.player(PlayerId::Player2);
    const double victory = static_cast<double>(std::max(1, state.mode.victory_score));

    // ── Term 1: score difference (weight 1.0, dominant) ──
    double value = weights.score *
                   static_cast<double>(p1.score - p2.score) / victory;

    // Accumulate the remaining terms per side, then take the difference,
    // which keeps the result exactly antisymmetric by construction.
    double p1_terms = 0.0;
    double p2_terms = 0.0;
    auto side = [&](PlayerId p) -> double& {
        return (p == PlayerId::Player1) ? p1_terms : p2_terms;
    };

    // ── Term 2: point tempo (battlefields controlled) ──
    // A battlefield held at turn start is the next point. Contested
    // splits it: half to the holder, half to the contester — so a
    // battlefield the opponent is contesting is worth nothing net.
    const double w_bf = weights.battlefield / victory;
    for (const auto& bf : state.battlefields) {
        const bool contested =
            bf.is_contested && bf.contested_by != PlayerId::None;
        if (bf.controller.has_value() && *bf.controller != PlayerId::None) {
            side(*bf.controller) += contested ? w_bf * 0.5 : w_bf;
        }
        if (contested) side(bf.contested_by) += w_bf * 0.5;
    }

    // ── Term 3: board width / retention ──
    // "Never empty" — a wide, rebuildable board is the antidote to board
    // evaporation. Diminishing so the fourth body is worth a quarter of
    // the first, and capped so a board dump can't outweigh a point.
    const double w_unit = weights.unit / victory;
    for (auto p : {PlayerId::Player1, PlayerId::Player2}) {
        const int units =
            static_cast<int>(state.allUnitsControlledBy(p).size());
        const int counted = std::min(units, kCorpusUnitCap);
        for (int i = 0; i < counted; ++i) {
            side(p) += w_unit * (1.0 - 0.25 * static_cast<double>(i));
        }
    }

    // ── Term 4: held interaction ──
    // "Play to your outs / don't tap out": a Reaction spell still in hand
    // is insurance. Capped at two — a third copy is not a third out.
    const double w_hold = weights.held_interaction / victory;
    for (auto p : {PlayerId::Player1, PlayerId::Player2}) {
        int held = 0;
        for (auto oid : state.player(p).hand) {
            if (held >= kCorpusHeldInteractionCap) break;
            if (!state.objectExists(oid)) continue;
            const auto& o = state.getObject(oid);
            if (o.isSpell() && o.keywords.has(Keyword::Reaction)) ++held;
        }
        side(p) += w_hold * static_cast<double>(held);
    }

    // ── Term 5: live trash resources ──
    // "The trash is a second hand" — but only for spells whose Flow cost
    // is LIVE right now: a printed [Flow] keyword, or a granted flow
    // stamped THIS turn (CR 829; grants expire by evaluation, not by a
    // scheduled cleanup, so a stale stamp must read as dead). Cap 3.
    const double w_trash = weights.trash_resource / victory;
    for (auto p : {PlayerId::Player1, PlayerId::Player2}) {
        int live = 0;
        for (auto oid : state.player(p).trash) {
            if (live >= kCorpusTrashResourceCap) break;
            if (!state.objectExists(oid)) continue;
            const auto& o = state.getObject(oid);
            if (!o.isSpell()) continue;
            const bool printed_flow = o.keywords.has(Keyword::Flow);
            const bool granted_live =
                o.granted_flow.has_value() &&
                o.granted_flow->valid_on_turn == state.turn.turn_number;
            if (printed_flow || granted_live) ++live;
        }
        side(p) += w_trash * static_cast<double>(live);
    }

    // ── Term 6: empowered legend ──
    // Empowered is a stored resource, not decoration — Heart of the
    // Tempest's action spends it for Assault 2.
    const double w_emp = weights.empowered_legend / victory;
    for (auto p : {PlayerId::Player1, PlayerId::Player2}) {
        const auto legend = state.player(p).legend_zone;
        if (legend == kInvalidId || !state.objectExists(legend)) continue;
        if (state.getObject(legend).is_empowered) side(p) += w_emp;
    }

    value += p1_terms - p2_terms;

    if (value >  1.0) value =  1.0;
    if (value < -1.0) value = -1.0;
    return {value, -value};
}

}  // namespace riftbound
