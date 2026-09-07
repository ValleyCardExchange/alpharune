#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/game_state.h"
#include "core/events.h"
#include "engine/effect_executor.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace riftbound {
namespace {

class KennenStormOfShuriken : public UnitCard {
public:
    const CardDef& def() const override { return def_; }
    std::vector<TriggerType> triggerTypes() const override {
        return {TriggerType::WhenYouPlayMe, TriggerType::WhenIConquer};
    }
    void onTrigger(CardContext& ctx, const std::vector<GameObjectId>& /*targets*/) override {
        if (ctx.firing_trigger == TriggerType::WhenYouPlayMe) {
            ctx.executor.burnCards(ctx.controller, 2);
            ctx.events.logTrace("KENNEN: played -> Burn 2");
            return;
        }

        // WhenIConquer — mandatory: give a spell in the trash Flow equal
        // to its printed cost this turn (CR 829; spec §6 / addendum).
        auto& ps = ctx.state.player(ctx.controller);
        std::vector<GameObjectId> spells;
        for (auto cid : ps.trash) {
            if (!ctx.state.objectExists(cid)) continue;
            if (ctx.state.getObject(cid).card_type == CardType::Spell)
                spells.push_back(cid);
        }
        if (spells.empty()) return;

        GameObjectId picked = pickTarget(ctx, "Kennen (spell in trash gains Flow)", spells);
        if (picked == kInvalidId && ctx.state.chain.resuming.has_value() &&
            ctx.state.chain.resuming->resume_point == 7) {
            return;  // suspended — awaiting agent choice
        }
        if (picked == kInvalidId || !ctx.state.objectExists(picked)) return;

        auto& spell = ctx.state.getObject(picked);
        const CardDef* def = spell.card_def_id != kInvalidId
            ? &ctx.executor.cardDB().get(spell.card_def_id) : nullptr;

        GameObject::GrantedFlow gf;
        gf.energy = def ? def->energy_cost : 0;
        gf.power = def ? def->power_cost : 0;
        gf.power_domain = (def && !def->domains.empty()) ? def->domains.front()
                                                            : Domain::Fury;
        gf.any_domain = false;
        gf.valid_on_turn = ctx.state.turn.turn_number;
        spell.granted_flow = gf;

        ctx.events.logTrace("KENNEN: " + spell.name + " gains Flow [E" +
                             std::to_string(gf.energy) + "][P" +
                             std::to_string(gf.power) + "] this turn");
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 789;
        d.def_id = R"RB(ven-113-166)RB";
        d.name = R"RB(Kennen, Storm of Shuriken)RB";
        d.set_code = R"RB(VEN)RB";
        d.set_name = R"RB(Vendetta)RB";
        d.public_code = R"RB(VEN-113/166)RB";
        d.collector_number = 113;
        d.artist = R"RB(Six More Vodka)RB";
        d.card_type = CardType::Unit;
        d.super_type = SuperType::Champion;
        d.domains = {Domain::Chaos};
        d.tags = {R"RB(Yordle)RB", R"RB(Kennen)RB"};
        d.energy_cost = 3;
        d.power_cost = 1;
        d.might = 4;
        d.rarity = Rarity::Epic;
        d.ability_text = R"RB(When you play me, [Burn 2]. (Put the top 2 cards of your Main Deck into your trash.)
When I conquer, give a spell in your trash [Flow] equal to its cost this turn. (You may play it from your trash for its Flow cost. Then banish it.))RB";
        d.image_url = R"RB(https://cmsassets.rgpub.io/sanity/images/dsfx7636/game_data_live/2f27c99f3df2940940dddd65105e5e42bbeeab3d-744x1039.png?accountingTag=RB)RB";
        return d;
    }();
};

}  // anonymous namespace

void register_card_789(CardRegistry& r) {
    r.registerCard(789, std::make_unique<KennenStormOfShuriken>());
}

} // namespace riftbound
