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
#include <optional>
#include <string>
#include <vector>

namespace riftbound {

// ─── The power rune: chosen FIRST, exhausted preferred ──────────────────────
//
// Recycling a rune for power carries NO readiness condition (CR 164.2.b —
// "Recycle this: [Reaction] — Add [C]"), so an EXHAUSTED rune pays power just
// as well as a ready one. Preferring the exhausted one keeps ready runes free
// for the energy half, exactly like the engine's canonical additional-cost
// payer (GameEngine::payAdditionalCost sorts its matching runes exhausted
// first). `domain == nullopt` means any rune — the [A] power symbol.
//
// Lives here rather than in card_helpers.h (which includes this header and
// builds canPayOnePower / payOnePower on top of it) so the equip scan and the
// one-power payer pick their rune by the same rule.
inline GameObjectId findPowerRune(const GameState& state, PlayerId player,
                                  std::optional<Domain> domain) {
    auto base_loc = BaseLocation{player};
    GameObjectId exhausted = kInvalidId, ready = kInvalidId;
    for (const auto& [id, obj] : state.objects) {
        if (!obj.isRune() || obj.controller != player) continue;
        if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
        if (domain.has_value()) {
            bool match = false;
            for (auto d : obj.domains) if (d == *domain) { match = true; break; }
            if (!match) continue;
        }
        if (obj.is_exhausted) {
            if (exhausted == kInvalidId) exhausted = id;
        } else if (ready == kInvalidId) {
            ready = id;
        }
    }
    return exhausted != kInvalidId ? exhausted : ready;
}

// ─── Shared equip-cost scan ─────────────────────────────────────────────────
// ONE scan of the controller's base, shared by every equip predicate and by
// the payer that spends the runes it names. Keeping both on this scan is what
// makes `canEquip` and `onEquip` incapable of disagreeing.
//
// The power rune is chosen FIRST and then EXCLUDED from the energy count, so
// one physical rune can never pay both halves of one cost. That is the rule
// the engine's own canonical additional-cost payer already enforces
// (GameEngine::canPayAdditionalCost keeps its recycled runes out of
// `energy_available`; payAdditionalCost skips them when exhausting) — an
// equip payer that disagreed with it said "payable" for a Boneshiver
// ([1][D]) held up by a single ready Body rune and then paid the whole cost
// off that one rune.
struct StandardEquipScan {
    GameObjectId power_rune = kInvalidId;  // recycled for the [D] / [A] half
    int energy_runes = 0;    // ready runes in base OTHER than `power_rune`
    bool payable(int energy_cost) const {
        return power_rune != kInvalidId && energy_runes >= energy_cost;
    }
};

/// `domain == nullopt` scans for the [A] (any-rune) power half.
inline StandardEquipScan scanEquipCost(const GameState& state, PlayerId player,
                                       std::optional<Domain> domain) {
    StandardEquipScan scan;
    scan.power_rune = findPowerRune(state, player, domain);
    if (scan.power_rune == kInvalidId) return scan;
    auto base_loc = BaseLocation{player};
    for (const auto& [id, obj] : state.objects) {
        if (id == scan.power_rune) continue;  // reserved for the power half
        if (!obj.isRune() || obj.controller != player || obj.is_exhausted) continue;
        if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
        scan.energy_runes++;
    }
    return scan;
}

inline StandardEquipScan scanStandardEquip(const GameState& state, PlayerId player,
                                           Domain domain) {
    return scanEquipCost(state, player, domain);
}

/// `Card::canEquip` for any gear whose equip cost is [energy] + one [domain]
/// power — the predicate half of `standardEquip`.
inline bool canStandardEquip(const GameState& state, PlayerId player,
                             int energy_cost, Domain domain) {
    return scanEquipCost(state, player, domain).payable(energy_cost);
}

/// `Card::canEquip` for [A] gear: one rune is recycled for the [A] and the
/// energy comes off the OTHER ready runes.
inline bool canUniversalEquip(const GameState& state, PlayerId player,
                              int energy_cost) {
    return scanEquipCost(state, player, std::nullopt).payable(energy_cost);
}

/// Spend exactly what `scan` authorised: exhaust `energy_cost` ready runes,
/// skipping the reserved power rune, then recycle that rune. Callers MUST
/// check `scan.payable(energy_cost)` first — the same scan object, so the
/// check and the payment can never name different runes.
inline void payEquipCost(CardContext& ctx, const StandardEquipScan& scan,
                         int energy_cost, const std::string& power_label) {
    auto& state = ctx.state;
    auto player = ctx.controller;
    auto& ps = state.player(player);
    auto base_loc = BaseLocation{player};

    int e_remaining = energy_cost;
    for (auto& [id, obj] : state.objects) {
        if (e_remaining <= 0) break;
        if (id == scan.power_rune) continue;  // reserved for the power half
        if (!obj.isRune() || obj.controller != player || obj.is_exhausted) continue;
        if (!obj.location.has_value() || *obj.location != LocationId{base_loc}) continue;
        obj.is_exhausted = true;
        e_remaining--;
    }

    auto& pr = state.getObject(scan.power_rune);
    ctx.events.logTrace("  EQUIP_COST: recycled " + pr.name + " for " + power_label);
    pr.location = std::nullopt;
    pr.zone = ZoneType::RuneDeck;
    ps.rune_deck.insert(ps.rune_deck.begin(), scan.power_rune);
}

// ─── Canonical standard equip: pay [energy] + one [domain] power, then attach ──
inline bool standardEquip(CardContext& ctx, GameObjectId gear_id, GameObjectId unit_id,
                          int energy_cost, Domain domain) {
    auto& state = ctx.state;
    auto player = ctx.controller;

    // PRE-CHECK: bail (no state change) unless BOTH the energy and the
    // domain-power can be paid — otherwise the loops below would partially
    // pay and then "equip for free". Same scan `canStandardEquip` answers on,
    // and the same scan `payEquipCost` spends.
    auto scan = scanEquipCost(state, player, domain);
    if (!scan.payable(energy_cost)) return false;

    payEquipCost(ctx, scan, energy_cost, "power");

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
        auto& state = ctx.state;
        auto scan = scanEquipCost(state, ctx.controller, std::nullopt);
        if (!scan.payable(energy_cost_)) return false;
        payEquipCost(ctx, scan, energy_cost_, "[A]");

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
