#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/game_state.h"
#include "core/events.h"
#include "engine/effect_executor.h"
#include <memory>
#include <string>
#include <vector>

namespace riftbound {
namespace {

class LightningRush : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    void onResolve(CardContext& ctx, const std::vector<GameObjectId>& /*targets*/) override {
        // "Look at the top 3 cards of your Main Deck. You may choose a
        // card from among them and draw it. Put the rest into your
        // trash." — EffectExecutor::revealAndChoose(rest=Trash) already
        // implements this exactly: public reveal (CR 424), choose-one-
        // or-none per revealed card, the chosen card lands in hand as a
        // real draw, the rest go to trash in revealed order (Kennen spec
        // §6 / addendum #4). It queries the agent directly and
        // synchronously (like EffectExecutor::predict), so no pickTarget
        // / resume machinery is needed here.
        ctx.executor.revealAndChoose(ctx.controller, 3,
                                      EffectExecutor::RestDestination::Trash);
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 790;
        d.def_id = R"RB(ven-156-166)RB";
        d.name = R"RB(Lightning Rush)RB";
        d.set_code = R"RB(VEN)RB";
        d.set_name = R"RB(Vendetta)RB";
        d.public_code = R"RB(VEN-156/166)RB";
        d.collector_number = 156;
        d.artist = R"RB(Kudos Productions)RB";
        d.card_type = CardType::Spell;
        d.super_type = SuperType::Signature;
        d.domains = {Domain::Order, Domain::Chaos};
        d.tags = {R"RB(Kennen)RB"};
        d.energy_cost = 1;
        d.rarity = Rarity::Epic;
        d.keywords.set(Keyword::Flow);
        d.flow_energy = 2;
        d.flow_power = 1;
        d.flow_any_domain = true;
        d.ability_text = R"RB(Look at the top 3 cards of your Main Deck. You may choose a card from among them and draw it. Put the rest into your trash.
[Flow] [2][A] (You may play this from your trash for its Flow cost. Then banish it.))RB";
        d.image_url = R"RB(https://cmsassets.rgpub.io/sanity/images/dsfx7636/game_data_live/22375e0d2d1a49cb0eb7e31aebf3e73a2e0bba55-744x1039.png?accountingTag=RB)RB";
        return d;
    }();
};

}  // anonymous namespace

void register_card_790(CardRegistry& r) {
    r.registerCard(790, std::make_unique<LightningRush>());
}

} // namespace riftbound
