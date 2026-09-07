#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/game_state.h"
#include "core/events.h"
#include "engine/effect_executor.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "cards/card_helpers.h"

namespace riftbound {
namespace {

class BladeOfTheRuinedKing : public GearCard {
public:
    const CardDef& def() const override { return def_; }
    bool hasEquipAbility() const override { return true; }

    // "[Equip] — [Y], Kill a friendly unit": both halves of the additional
    // cost, checked target-agnostically. The victim must be a friendly unit
    // OTHER than the equip target, so two friendly units on board means every
    // possible target leaves one to kill.
    bool canEquip(const GameState& state, PlayerId controller) const override {
        if (!canPayOnePower(state, controller, Domain::Order)) return false;
        int friendly_units = 0;
        for (const auto& [id, obj] : state.objects) {
            if (!obj.isUnit() || obj.controller != controller) continue;
            if (!obj.location.has_value()) continue;
            if (++friendly_units >= 2) return true;
        }
        return false;
    }

    bool onEquip(CardContext& ctx, GameObjectId unit) override {
        if (!canEquip(ctx.state, ctx.controller)) return false;
        auto& state = ctx.state;
        auto player = ctx.controller;

        std::vector<GameObjectId> killable;
        for (auto& [id, obj] : state.objects) {
            if (!obj.isUnit() || obj.controller != player) continue;
            if (!obj.location.has_value()) continue;
            if (id == unit) continue;  // can't kill the unit we're equipping
            killable.push_back(id);
        }

        // Commit cost 1: kill a friendly unit (agent choice).
        GameObjectId victim = pickTarget(ctx, "Blade of the Ruined King: "
                                              "kill a friendly unit", killable);
        if (victim == kInvalidId) {
            // Suspended for the agent choice — re-entry will resume.
            return false;
        }
        if (!state.objectExists(victim)) return false;
        ctx.executor.killObject(victim);

        // Commit cost 2: recycle the Order power rune for [Y] (CR 164.2.b —
        // an exhausted rune pays power, and is preferred over a ready one).
        if (!payOnePower(ctx, player, Domain::Order)) return false;

        // Attach.
        if (!state.objectExists(unit)) return false;
        auto& gear = state.getObject(ctx.source);
        auto& unit_obj = state.getObject(unit);
        ctx.events.logTrace("EQUIP: " + gear.name + " -> " + unit_obj.name +
                             " (might_bonus=" + std::to_string(gear.might_bonus) + ")");
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
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 498;
        d.def_id = R"RB(sfd-178-221)RB";
        d.name = R"RB(Blade of the Ruined King)RB";
        d.set_code = R"RB(SFD)RB";
        d.set_name = R"RB(Spiritforged)RB";
        d.public_code = R"RB(SFD-178/221)RB";
        d.collector_number = 178;
        d.artist = R"RB(黯荧岛Dark Glow)RB";
        d.card_type = CardType::Gear;
        d.domains = {Domain::Order};
        d.tags = {R"RB(Equipment)RB"};
        d.energy_cost = 3;
        d.power_cost = 1;
        d.might_bonus = 4;
        d.rarity = Rarity::Epic;
        d.keywords.set(Keyword::Equip);
        d.ability_text = R"RB([Equip] — [Y], Kill a friendly unit (Pay the cost: Attach this to a unit you control.))RB";
        d.image_url = R"RB(https://cmsassets.rgpub.io/sanity/images/dsfx7636/game_data_live/26ab126258a15afd380c313e973f7469808ce55f-744x1039.png?accountingTag=RB)RB";
        return d;
    }();
};

}  // anonymous namespace

void register_card_498(CardRegistry& r) {
    r.registerCard(498, std::make_unique<BladeOfTheRuinedKing>());
}

} // namespace riftbound
