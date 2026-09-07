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

class UpFromTheDeep : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    void onResolve(CardContext& ctx, const std::vector<GameObjectId>& /*targets*/) override {
        // "Play two 1 [M] Tentacle unit tokens from Bilgewater." Tokens
        // are not cards (CR 185, 350.2) and createToken emits no
        // CardPlayedEvent, so this never fires WhenYouPlayFromNonHand
        // (addendum #7 / test #28).
        LocationId loc{BaseLocation{ctx.controller}};
        ctx.executor.createToken(ctx.controller, CardType::Unit, "Tentacle",
                                 1, {"Tentacle", "Bilgewater"}, KeywordSet{},
                                 loc, /*enter_ready=*/false);
        ctx.executor.createToken(ctx.controller, CardType::Unit, "Tentacle",
                                 1, {"Tentacle", "Bilgewater"}, KeywordSet{},
                                 loc, /*enter_ready=*/false);
        ctx.events.logTrace("UP FROM THE DEEP: played two 1[M] Tentacle tokens");
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 791;
        d.def_id = R"RB(ven-100-166)RB";
        d.name = R"RB(Up from the Deep)RB";
        d.set_code = R"RB(VEN)RB";
        d.set_name = R"RB(Vendetta)RB";
        d.public_code = R"RB(VEN-100/166)RB";
        d.collector_number = 100;
        d.artist = R"RB(Kudos Productions)RB";
        d.card_type = CardType::Spell;
        d.domains = {Domain::Chaos};
        d.energy_cost = 3;
        d.rarity = Rarity::Common;
        d.keywords.set(Keyword::Flow);
        d.flow_energy = 3;
        d.ability_text = R"RB(Play two 1 [M] Tentacle unit tokens from Bilgewater.
[Flow] [3] (You may play this from your trash for its Flow cost. Then banish it.))RB";
        d.image_url = R"RB(https://cmsassets.rgpub.io/sanity/images/dsfx7636/game_data_live/245942e8b2bf073d5d6d57074474ec273557f4df-744x1039.png?accountingTag=RB)RB";
        return d;
    }();
};

}  // anonymous namespace

void register_card_791(CardRegistry& r) {
    r.registerCard(791, std::make_unique<UpFromTheDeep>());
}

} // namespace riftbound
