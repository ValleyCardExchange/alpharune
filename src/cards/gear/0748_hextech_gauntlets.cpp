#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/game_state.h"
#include "core/events.h"
#include "cards/gear/equip_base.h"
#include "engine/effect_executor.h"
#include <algorithm>
#include <optional>
#include <memory>
#include <string>
#include <vector>

namespace riftbound {
namespace {

class HextechGauntlets : public GearCard {
public:
    const CardDef& def() const override { return def_; }

    bool hasEquipAbility() const override { return true; }

    // "[Equip] [3][A], reduced by the chosen unit's Might" — the energy is
    // TARGET-dependent, so legality is answered PER TARGET and the
    // target-agnostic predicate is "does SOME friendly unit satisfy it".
    static int energyFor(int target_might) {
        return std::max(0, 3 - target_might);
    }

    bool canEquipTarget(const GameState& state, PlayerId controller,
                        GameObjectId unit) const override {
        if (!state.objectExists(unit)) return false;
        const auto& u = state.getObject(unit);
        if (!u.isUnit() || u.controller != controller) return false;
        if (!u.location.has_value()) return false;
        // One rune is recycled for the [A]; the energy comes off the OTHER
        // ready runes (scanEquipCost keeps the two halves off one rune).
        return scanEquipCost(state, controller, std::nullopt)
                   .payable(energyFor(u.current_might));
    }

    bool canEquip(const GameState& state, PlayerId controller) const override {
        for (const auto& [id, obj] : state.objects) {
            if (!obj.isUnit() || obj.controller != controller) continue;
            if (!obj.location.has_value()) continue;
            if (canEquipTarget(state, controller, id)) return true;
        }
        return false;
    }

    bool onEquip(CardContext& ctx, GameObjectId unit) override {
        auto& state = ctx.state;
        if (!canEquipTarget(state, ctx.controller, unit)) return false;

        const int reduction = state.getObject(unit).current_might;
        const int energy_cost = energyFor(reduction);
        auto scan = scanEquipCost(state, ctx.controller, std::nullopt);
        if (!scan.payable(energy_cost)) return false;  // canEquipTarget agrees
        payEquipCost(ctx, scan, energy_cost, "[A]");

        // Attach.
        auto& gear = state.getObject(ctx.source);
        auto& unit_obj = state.getObject(unit);
        ctx.events.logTrace("EQUIP: Hextech Gauntlets -> " + unit_obj.name +
                            " (energy paid=" + std::to_string(energy_cost) +
                            ", reduced by Might " + std::to_string(reduction) + ")");
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

    TriggerType equippedTriggerType() const override { return TriggerType::WhenIConquer; }
    void onEquippedTrigger(CardContext& ctx, GameObjectId /*unit*/,
                            const std::vector<GameObjectId>& /*targets*/) override {
        // "if you assigned 3 or more excess damage" — excess-damage assignment
        // isn't surfaced to the equipped trigger, so draw unconditionally on
        // conquer (best-effort; documented in report).
        ctx.executor.drawCards(ctx.controller, 1);
        ctx.events.logTrace("HEXTECH GAUNTLETS: conquer -> draw 1");
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 748;
        d.def_id = R"RB(unl-188-219)RB";
        d.name = R"RB(Hextech Gauntlets)RB";
        d.set_code = R"RB(UNL)RB";
        d.set_name = R"RB(Unleashed)RB";
        d.public_code = R"RB(UNL-188/219)RB";
        d.collector_number = 188;
        d.artist = R"RB(Grafit Studio)RB";
        d.card_type = CardType::Gear;
        d.super_type = SuperType::Signature;
        d.domains = {Domain::Fury, Domain::Order};
        d.tags = {R"RB(Vi)RB", R"RB(Equipment)RB"};
        d.energy_cost = 3;
        d.might_bonus = 3;
        d.rarity = Rarity::Epic;
        d.keywords.set(Keyword::Equip);
        d.ability_text = R"RB([Equip] [3][A]. This ability's Energy cost is reduced by the Might of the unit you choose. (Pay the cost: Attach this to a unit you control.))RB";
        d.effect_text = R"RB(When I conquer, if you assigned 3 or more excess damage, draw 1.)RB";
        d.image_url = R"RB(https://cmsassets.rgpub.io/sanity/images/dsfx7636/game_data_live/22755fc744a79fbdcce0ceadfe20d6d0a2b8624a-744x1039.png)RB";
        return d;
    }();
};

}  // anonymous namespace

void register_card_748(CardRegistry& r) {
    r.registerCard(748, std::make_unique<HextechGauntlets>());
}

} // namespace riftbound
