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

class SandsweptTomb : public BattlefieldCard {
public:
    const CardDef& def() const override { return def_; }
    // "Each spell that chooses one or more units here that are friendly to it
    //  costs [A] less." A POWER (rune) discount of 1 in ANY domain. Wired via
    //  BattlefieldState::friendly_spell_power_discount (set here in
    //  applyPassiveAura, reset with the other BF aura flags in
    //  GameEngine::recalculateAuras):
    //    • the spell action generators offer a second, battlefield-RESTRICTED
    //      intent for a resolve-time-target spell that could choose a friendly
    //      unit here, priced with the discount;
    //    • executePlaySpell stages the discount (PlayerState::
    //      transient_power_discount, or a reduced flow power) while the cost
    //      is paid, and stamps the restriction on the chain item;
    //    • Card::pickTarget filters the resolve-time legal list to units here,
    //      so the choice can never dodge the discount's condition.
    //  "Friendly to it" is relative to the SPELL'S controller and the flag
    //  lives on the battlefield, so both players benefit from the Tomb.
    void applyPassiveAura(GameState& state, PlayerId /*controller*/) const override {
        for (auto& b : state.battlefields) {
            if (!state.objectExists(b.card_object_id)) continue;
            if (state.getObject(b.card_object_id).card_def_id == cardDefId())
                b.friendly_spell_power_discount = 1;
        }
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 792;
        d.def_id = R"RB(ven-164-166)RB";
        d.name = R"RB(Sandswept Tomb)RB";
        d.set_code = R"RB(VEN)RB";
        d.set_name = R"RB(Vendetta)RB";
        d.public_code = R"RB(VEN-164/166)RB";
        d.collector_number = 164;
        d.artist = R"RB(Kudos Productions)RB";
        d.card_type = CardType::Battlefield;
        d.rarity = Rarity::Uncommon;
        d.ability_text = R"RB(Each spell that chooses one or more units here that are friendly to it costs [A] less.)RB";
        d.image_url = R"RB(https://cmsassets.rgpub.io/sanity/images/dsfx7636/game_data_live/d77c607878d6815e02796f8bfa8a5696ad5f1a7d-1039x744.png?accountingTag=RB)RB";
        return d;
    }();
};

}  // anonymous namespace

void register_card_792(CardRegistry& r) {
    r.registerCard(792, std::make_unique<SandsweptTomb>());
}

} // namespace riftbound
