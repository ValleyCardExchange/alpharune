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

class LightningRush : public SpellCard {
public:
    const CardDef& def() const override { return def_; }

    // "Look at the top 3 cards of your Main Deck. You may choose A card
    // from among them and draw it. Put the rest into your trash." — "may
    // choose A card" is at most ONE, so this deliberately avoids
    // EffectExecutor::revealAndChoose (which asks draw-or-skip
    // independently per revealed card and would let the agent draw all
    // three). Modelled on Stacked Deck (0183_stacked_deck.cpp): peek the
    // top up-to-3 once (resume_point < 3, before Card::pickMode claims
    // resume_points 3-5), stash their ids in resume_data[2..], then
    // Card::pickMode offers EXACTLY ONE decision — one mode per revealed
    // card plus a trailing "None" mode (index == the reveal count) — so
    // "at most one" is structural, not agent discipline.
    //
    // "Look AT" is a private look (CR 128.4 / 424.1), NOT a public
    // Reveal: each peeked card gets its own CardRevealedEvent with
    // revealed_to_all=false / revealed_to=controller, matching Stacked
    // Deck and the Vision keyword (trigger_manager.cpp).
    void onResolve(CardContext& ctx, const std::vector<GameObjectId>& /*targets*/) override {
        auto& ri = ctx.state.chain.resuming.value();
        auto& ps = ctx.state.player(ctx.controller);

        if (ri.resume_point < 3) {
            int actual = std::min(3, static_cast<int>(ps.main_deck.size()));
            while (ri.resume_data.size() < 2) ri.resume_data.push_back(0);
            ri.resume_data.push_back(actual);  // index 2: how many peeked
            for (int i = 0; i < actual; ++i) {
                auto cid = ps.main_deck.back();
                ps.main_deck.pop_back();
                // index 3+i. resume_data is an int32 scratchpad and
                // GameObjectId is uint32_t: ids above INT32_MAX would wrap,
                // which the engine's sequential id allocator never reaches.
                ri.resume_data.push_back(static_cast<int32_t>(cid));
                if (ctx.state.objectExists(cid)) {
                    auto& obj = ctx.state.getObject(cid);
                    ctx.events.logTrace("  LOOKED AT: " + obj.name + " (id=" +
                                         std::to_string(cid) +
                                         ") — PRIVATE to " + toString(ctx.controller));
                    ctx.events.emit(CardRevealedEvent{
                        cid, obj.card_def_id, obj.owner,
                        /*revealed_to_all=*/false, /*revealed_to=*/ctx.controller,
                        ZoneType::MainDeck,
                    });
                }
            }
        }

        int actual = ri.resume_data.size() >= 3
            ? static_cast<int>(ri.resume_data[2]) : 0;
        if (actual == 0) return;  // empty deck — nothing to look at

        std::vector<GameObjectId> revealed;
        revealed.reserve(actual);
        for (int i = 0; i < actual; ++i)
            revealed.push_back(static_cast<GameObjectId>(ri.resume_data[3 + i]));

        std::vector<std::string> labels;
        labels.reserve(actual + 1);
        for (auto cid : revealed) {
            labels.push_back(ctx.state.objectExists(cid)
                ? ctx.state.getObject(cid).name : std::string("?"));
        }
        labels.push_back("None");

        int mode = pickMode(ctx, "Lightning Rush: choose a card to draw",
                             actual + 1, labels);
        if (mode == -1) return;  // suspended — awaiting agent choice
        if (mode == -2) return;  // no legal mode (unreachable: actual >= 1)

        GameObjectId drawn = (mode < actual) ? revealed[mode] : kInvalidId;

        int drawn_count = 0;
        for (auto cid : revealed) {
            if (!ctx.state.objectExists(cid)) continue;
            auto& obj = ctx.state.getObject(cid);
            if (cid == drawn) {
                obj.zone = ZoneType::Hand;
                obj.location = std::nullopt;
                ps.hand.push_back(cid);
                ++drawn_count;
                ctx.events.logTrace("LIGHTNING RUSH: drew " + obj.name);
            } else {
                obj.zone = ZoneType::Trash;
                obj.location = std::nullopt;
                ps.trash.push_back(cid);
                ctx.events.logTrace("LIGHTNING RUSH: trashed " + obj.name);
            }
        }
        if (drawn_count > 0) {
            // The chosen card lands in hand AS A DRAW, matching
            // EffectExecutor::revealAndChoose (effect_executor.cpp) exactly,
            // so WhenYouDrawACard consumers see it.
            ps.draws_this_turn += drawn_count;
            ctx.events.emit(CardsDrawnEvent{ctx.controller, drawn_count});
        }
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
