#pragma once
/// @file equip_base.h
/// Shared equip infrastructure for gear cards — the canonical standardEquip
/// helper plus the SimpleEquipGear / UniversalEquipGear base classes. Lives in
/// the gear/ type dir (relocated from the old _shared holding pen). This is the
/// ONE canonical standardEquip; the old per-audit-file copies redirect here.

#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/game_state.h"
#include "engine/effect_executor.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace riftbound {

// ─── Shared equip-cost predicate ────────────────────────────────────────────
// ONE scan of the controller's base, shared by `standardEquip` (which needs
// the rune ids to spend) and by `SimpleEquipGear::canEquip` (which needs only
// the yes/no). Keeping both on this scan is what makes `canEquip` and
// `onEquip` incapable of disagreeing.
//
// Recycling a rune for power carries NO readiness condition (CR 164.2.b —
// "Recycle this: [Reaction] — Add [C]"), which is why `domain_rune` ignores
// `is_exhausted`; a rune exhausted to pay the energy may then be recycled for
// the power.
struct StandardEquipScan {
    int ready_runes = 0;                 // in base, any domain
    GameObjectId domain_rune = kInvalidId;  // matching domain, exhausted or ready
    bool payable(int energy_cost) const {
        return ready_runes >= energy_cost && domain_rune != kInvalidId;
    }
};

inline StandardEquipScan scanStandardEquip(const GameState& state, PlayerId player,
                                           Domain domain) {
    StandardEquipScan scan;
    auto base_loc = BaseLocation{player};
    for (const auto& [id, obj] : state.objects) {
        if (!obj.isRune() || obj.controller != player) continue;
        if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
        if (!obj.is_exhausted) scan.ready_runes++;
        if (scan.domain_rune == kInvalidId) {
            for (auto d : obj.domains) {
                if (d == domain) { scan.domain_rune = id; break; }
            }
        }
    }
    return scan;
}

/// `Card::canEquip` for any gear whose equip cost is [energy] + one [domain]
/// power — the predicate half of `standardEquip`.
inline bool canStandardEquip(const GameState& state, PlayerId player,
                             int energy_cost, Domain domain) {
    return scanStandardEquip(state, player, domain).payable(energy_cost);
}

/// `Card::canEquip` for [A] gear: the energy must be coverable by ready runes
/// AND one rune must remain in base to recycle for the [A] power — an
/// exhausted one counts, including one just exhausted for the energy. That is
/// exactly `total runes in base >= max(1, energy_cost)`.
inline bool canUniversalEquip(const GameState& state, PlayerId player,
                              int energy_cost) {
    int ready = 0, total = 0;
    auto base_loc = BaseLocation{player};
    for (const auto& [id, obj] : state.objects) {
        if (!obj.isRune() || obj.controller != player) continue;
        if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
        total++;
        if (!obj.is_exhausted) ready++;
    }
    return ready >= energy_cost && total >= std::max(1, energy_cost);
}

// ─── Canonical standard equip: pay [energy] + one [domain] power, then attach ──
inline bool standardEquip(CardContext& ctx, GameObjectId gear_id, GameObjectId unit_id,
                          int energy_cost, Domain domain) {
    auto& state = ctx.state;
    auto player = ctx.controller;
    auto& ps = state.player(player);

    // PRE-CHECK: bail (no state change) unless BOTH the energy and the
    // domain-power can be paid — otherwise the loops below would partially
    // pay and then "equip for free". Same scan `canStandardEquip` answers on.
    auto scan = scanStandardEquip(state, player, domain);
    if (!scan.payable(energy_cost)) return false;
    const GameObjectId domain_rune = scan.domain_rune;
    auto base_loc = BaseLocation{player};

    // Pay energy: exhaust ready runes.
    if (energy_cost > 0) {
        int e_remaining = energy_cost;
        for (auto& [id, obj] : state.objects) {
            if (e_remaining <= 0) break;
            if (!obj.isRune() || obj.controller != player || obj.is_exhausted) continue;
            if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
            obj.is_exhausted = true;
            e_remaining--;
        }
    }

    // Recycle the pre-located matching-domain rune for power.
    {
        auto& dr = state.getObject(domain_rune);
        ctx.events.logTrace("  EQUIP_COST: recycled " + dr.name + " for power");
        dr.location = std::nullopt;
        dr.zone = ZoneType::RuneDeck;
        ps.rune_deck.insert(ps.rune_deck.begin(), domain_rune);
    }

    // Attach.
    auto& gear = state.getObject(gear_id);
    auto& unit = state.getObject(unit_id);
    ctx.events.logTrace("EQUIP: " + gear.name + " -> " + unit.name +
                         " (might_bonus=" + std::to_string(gear.might_bonus) + ")");
    gear.attached_to = unit_id;
    unit.attachments.push_back(gear_id);
    gear.location = unit.location;
    gear.zone = unit.zone;
    unit.attachment_might_bonus += gear.might_bonus;
    unit.recomputeMight();
    ctx.events.emit(ObjectStateChangedEvent{gear_id, "attached"});
    ctx.events.emit(ObjectStateChangedEvent{unit_id, "equipped"});
    return true;
}

// ─── Simple equip gear: pay [DOMAIN] power, then attach ─────────────────────
class SimpleEquipGear : public GearCard {
public:
    // Behavior config only (equip domain + energy). The concrete card supplies
    // its data via def().
    explicit SimpleEquipGear(Domain equip_domain, int energy_cost = 0)
        : equip_domain_(equip_domain), energy_cost_(energy_cost) {}

    bool hasEquipAbility() const override { return true; }
    bool canEquip(const GameState& state, PlayerId controller) const override {
        return canStandardEquip(state, controller, energy_cost_, equip_domain_);
    }
    bool onEquip(CardContext& ctx, GameObjectId unit) override {
        if (!canEquip(ctx.state, ctx.controller)) return false;
        return standardEquip(ctx, ctx.source, unit, energy_cost_, equip_domain_);
    }

protected:
    Domain equip_domain_;
    int energy_cost_;
};

// ─── Universal equip gear: pay [A] (recycle any rune), then attach ──────────
class UniversalEquipGear : public GearCard {
public:
    explicit UniversalEquipGear(int energy_cost = 0)
        : energy_cost_(energy_cost) {}

    bool hasEquipAbility() const override { return true; }
    bool canEquip(const GameState& state, PlayerId controller) const override {
        return canUniversalEquip(state, controller, energy_cost_);
    }
    bool onEquip(CardContext& ctx, GameObjectId unit) override {
        if (!canEquip(ctx.state, ctx.controller)) return false;
        auto& state = ctx.state;
        auto player = ctx.controller;
        auto& ps = state.player(player);
        auto base_loc = BaseLocation{player};

        for (int i = 0; i < energy_cost_; ++i) {
            for (auto& [id, obj] : state.objects) {
                if (!obj.isRune() || obj.controller != player || obj.is_exhausted) continue;
                if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
                obj.is_exhausted = true;
                break;
            }
        }
        for (auto& [id, obj] : state.objects) {
            if (!obj.isRune() || obj.controller != player) continue;
            if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
            ctx.events.logTrace("  EQUIP_COST: recycled " + obj.name + " for [A]");
            obj.location = std::nullopt;
            obj.zone = ZoneType::RuneDeck;
            ps.rune_deck.insert(ps.rune_deck.begin(), id);
            break;
        }
        auto& gear = state.getObject(ctx.source);
        auto& unit_obj = state.getObject(unit);
        ctx.events.logTrace("EQUIP: " + gear.name + " -> " + unit_obj.name);
        gear.attached_to = unit;
        unit_obj.attachments.push_back(ctx.source);
        gear.location = unit_obj.location;
        gear.zone = unit_obj.zone;
        unit_obj.attachment_might_bonus += gear.might_bonus;
        unit_obj.recomputeMight();
        ctx.events.emit(ObjectStateChangedEvent{ctx.source, "attached"});
        ctx.events.emit(ObjectStateChangedEvent{unit, "equipped"});
        return true;
    }

protected:
    int energy_cost_;
};

} // namespace riftbound
