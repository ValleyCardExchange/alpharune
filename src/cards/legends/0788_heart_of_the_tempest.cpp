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

class HeartOfTheTempest : public LegendCard {
public:
    const CardDef& def() const override { return def_; }

    TriggerType triggerType() const override {
        return TriggerType::WhenYouPlayFromNonHand;
    }
    void onTrigger(CardContext& ctx, const std::vector<GameObjectId>& /*targets*/) override {
        ctx.executor.empowerObject(ctx.source);
    }

    // "[Action] Disempower me, [E]: Give a unit [Assault 2] this turn."
    // Any unit (the text says "a unit", not "a friendly unit").
    std::vector<ActivatedAbility> activatedAbilities() const override {
        return {{
            .cost = {.exhaust = true, .disempower_self = true},
            .targets = TargetRequirements{.count = 1, .must_be_unit = true},
            .is_action = true,
            .is_reaction = false,
            .needs_activation_time_target = true,
        }};
    }
    TargetRequirements getTargetRequirements() const override {
        return TargetRequirements{.count = 1, .must_be_unit = true};
    }
    void onActivate(CardContext& ctx, int /*ability_index*/,
                    const std::vector<GameObjectId>& targets) override {
        GameObjectId picked = kInvalidId;
        if (!targets.empty()) {
            picked = targets[0];
        } else {
            auto legal = enumerateLegalTargets(ctx.state, ctx.controller);
            picked = pickTarget(ctx, "Heart of the Tempest (give a unit Assault 2)", legal);
        }
        if (picked == kInvalidId || !ctx.state.objectExists(picked)) return;
        ctx.executor.giveTemporaryKeyword(picked, Keyword::Assault, 2);
        ctx.events.logTrace("HEART OF THE TEMPEST: gave " +
                             ctx.state.getObject(picked).name +
                             " [Assault 2] this turn");
    }
private:
    const CardDef def_ = [] {
        CardDef d;
        d.id = 788;
        d.def_id = R"RB(ven-155-166)RB";
        d.name = R"RB(Heart of the Tempest)RB";
        d.set_code = R"RB(VEN)RB";
        d.set_name = R"RB(Vendetta)RB";
        d.public_code = R"RB(VEN-155/166)RB";
        d.collector_number = 155;
        d.artist = R"RB(Envar Studio)RB";
        d.card_type = CardType::Legend;
        d.domains = {Domain::Order, Domain::Chaos};
        d.tags = {R"RB(Yordle)RB", R"RB(Kennen)RB"};
        d.rarity = Rarity::Rare;
        d.ability_text = R"RB(When you play a card from anywhere other than your hand, empower me.
[Action] Disempower me, [E]: Give a unit [Assault 2] this turn. (+2 [M] while it's an attacker.))RB";
        d.image_url = R"RB(https://cmsassets.rgpub.io/sanity/images/dsfx7636/game_data_live/0eab83392b310417d2630d50a3bfee3dd02b31c4-744x1039.png?accountingTag=RB)RB";
        return d;
    }();
};

}  // anonymous namespace

void register_card_788(CardRegistry& r) {
    r.registerCard(788, std::make_unique<HeartOfTheTempest>());
}

} // namespace riftbound
