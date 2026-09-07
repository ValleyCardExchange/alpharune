/// @file test_repeat_reactions.cpp
/// [Repeat] (CR 820) tranches must run through the SAME resume pump the
/// first resolution uses — the final re-review's finding A.
///
/// `ChainManager::stepResolve` resolves the popped item inside a
/// `requestChoice`/`takeChoice` pump: call `resolve_spell`, and while the
/// resolving Card has published a pending choice, ask the agent, record the
/// answer, and re-enter so the Card advances past its `resume_point`. The
/// Repeat loop that follows re-invoked `resolve_spell` directly, once per
/// `repeats_paid`, with NO pump. A resumable card therefore:
///
///   * completed only its `case 0` branch on every paid tranche (the effect
///     never happened), and
///   * left `EffectExecutor::pending_` ACTIVE when it yielded — so the next
///     `stepResolve` consumed the orphaned choice and handed it to a
///     DIFFERENT card's resolution.
///
/// Hard Bargain (457) is the live example: `[Reaction]` + `[Repeat] [2]`,
/// only ever playable in the Closed State (`hasLegalTargets` needs a spell on
/// the chain), and its `case 0` yields whenever the counter target's
/// controller holds >= 2 ready runes. Since closed-state spell reactions
/// started routing through `GameEngine::executePlaySpell`, its Repeat is live
/// for the first time. Called Shot (443) has the same shape.
///
/// Tests here:
///   (1) Hard Bargain played CLOSED with one paid tranche, against a 3-item
///       chain whose two counter targets are both rescuable — driven through
///       the real FEPR loop with a scripted agent.
///   (2) The pump itself, at ChainManager level, where the test owns the
///       EffectExecutor and can assert no pending choice leaks.
///   (3) Regression: a NON-resumable [Repeat] spell still resolves exactly
///       `1 + repeats_paid` times.

#include "tests/cards/card_test_fixture.h"

#include "cards/card.h"
#include "cards/card_registry.h"
#include "core/events.h"
#include "core/game_state.h"
#include "engine/chain_manager.h"
#include "engine/effect_executor.h"
#include "engine/game_engine.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace riftbound;
using namespace riftbound::test;

namespace {

// Test-local ids, above the shipped registry and clear of the 90x/91x blocks
// test_flow.cpp and test_closed_state_plays.cpp already use.
constexpr CardDefId kTallyAction   = 930;  // [Action] 0E — opens the chain
constexpr CardDefId kVictimSpell   = 931;  // [Reaction] 0E — the first counter target
constexpr CardDefId kResumableRpt  = 932;  // resumable, [Repeat] [1]
constexpr CardDefId kPlainRepeat   = 933;  // non-resumable, [Repeat] [1]

// Resolution tallies. Reset by each fixture's SetUp.
int g_tally_resolves    = 0;
int g_victim_resolves   = 0;
int g_resumable_starts  = 0;  // case 0 entries
int g_resumable_done    = 0;  // case 1 entries (the effect actually happened)
int g_plain_resolves    = 0;

CardDef makeSpellDef(CardDefId id, const char* name, int energy) {
    CardDef d;
    d.id = id;
    d.name = name;
    d.card_type = CardType::Spell;
    d.domains = {Domain::Chaos};
    d.energy_cost = energy;
    return d;
}

/// Free [Action] spell whose only observable is "did I resolve?". It sits at
/// the BOTTOM of the chain and is the target of Hard Bargain's REPEAT
/// tranche: if the tranche works it is countered and never resolves.
class TallyActionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    void onResolve(CardContext&, const std::vector<GameObjectId>&) override {
        ++g_tally_resolves;
    }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kTallyAction, "Tally Action Spell", 0);
        d.keywords.set(Keyword::Action);
        return d;
    }();
};

/// Free [Reaction] spell — the target of Hard Bargain's FIRST resolution.
class VictimReactionSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    void onResolve(CardContext&, const std::vector<GameObjectId>&) override {
        ++g_victim_resolves;
    }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kVictimSpell, "Victim Reaction Spell", 0);
        d.keywords.set(Keyword::Reaction);
        return d;
    }();
};

/// Minimal resumable spell in Hard Bargain's shape: case 0 publishes a
/// yes/no choice and yields; case 1 consumes it and records the effect.
/// Carries a printed [Repeat] [1] so the parse path is realistic, though
/// these tests stamp `repeats_paid` on the chain item directly.
class ResumableRepeatSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    void onResolve(CardContext& ctx, const std::vector<GameObjectId>&) override {
        auto& ri = ctx.state.chain.resuming.value();
        switch (ri.resume_point) {
        case 0: {
            ++g_resumable_starts;
            Intent yes;
            yes.type = IntentType::MakeChoice;
            yes.player = ctx.controller;
            yes.chosen_value = 1;
            Intent no;
            no.type = IntentType::MakeChoice;
            no.player = ctx.controller;
            no.chosen_value = 0;
            ctx.executor.requestChoice(ctx.controller, {yes, no},
                                        "Resumable Repeat: proceed?");
            ri.resume_point = 1;
            return;
        }
        case 1: {
            auto choice = ctx.executor.takeChoice();
            if (choice && choice->chosen_value.value_or(0) == 1) ++g_resumable_done;
            return;
        }
        default:
            return;
        }
    }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kResumableRpt, "Resumable Repeat Spell", 1);
        d.keywords.set(Keyword::Reaction);
        d.keywords.set(Keyword::Repeat);
        d.ability_text = "[Reaction]\n[Repeat] [1]\nAsk a question, then do a thing.";
        return d;
    }();
};

/// Non-resumable [Repeat] spell — one straight-line effect per resolution.
class PlainRepeatSpell : public SpellCard {
public:
    const CardDef& def() const override { return def_; }
    void onResolve(CardContext&, const std::vector<GameObjectId>&) override {
        ++g_plain_resolves;
    }
private:
    const CardDef def_ = [] {
        CardDef d = makeSpellDef(kPlainRepeat, "Plain Repeat Spell", 1);
        d.keywords.set(Keyword::Reaction);
        d.keywords.set(Keyword::Repeat);
        d.ability_text = "[Reaction]\n[Repeat] [1]\nDo a thing.";
        return d;
    }();
};

/// One scripted agent for both seats.
///
/// It classifies every published choice set before answering:
///   • RESCUE  — exactly two MakeChoice options, one naming a single SPELL
///     object and the other naming nothing. That is Hard Bargain's
///     "pay 2 to save X? [pay | decline]". Always DECLINES, and records who
///     was asked, about which spell, and whether Hard Bargain had already
///     left the chain when the question arrived.
///   • YES/NO  — every option a MakeChoice carrying `chosen_value` and no
///     objects: the Repeat-tranche poll in executePlaySpell. Accepts
///     `accept_yes_no` times, then declines.
///   • otherwise — take the wanted play once (`want`), else pass priority,
///     else legal.front() (this is what answers the cost-payment cursor,
///     which publishes single-object MakeChoice sets of RUNES).
class ScriptAgent : public AgentInterface {
public:
    struct RescueAsk {
        PlayerId asked = PlayerId::None;
        GameObjectId about = kInvalidId;
        bool counterspell_already_gone = false;
    };

    std::function<bool(const Intent&)> want;
    /// Optional state gate on `want`: priority in the Closed State is offered
    /// to the newest item's controller FIRST, so without a gate P1 would take
    /// its reaction before P2 ever gets to add the item P1 is waiting for.
    std::function<bool(const GameState&)> gate;
    bool taken = false;
    int accept_yes_no = 0;
    int yes_no_seen = 0;
    std::vector<RescueAsk>* rescues = nullptr;
    GameObjectId counterspell = kInvalidId;  // watched for "already disposed"

    Intent selectAction(const GameState& s,
                        const std::vector<Intent>& legal) override {
        if (legal.empty()) return Intent{};

        // ── RESCUE prompt? ──
        if (legal.size() == 2) {
            const Intent* pay = nullptr;
            const Intent* decline = nullptr;
            bool shape_ok = true;
            for (const auto& i : legal) {
                if (i.type != IntentType::MakeChoice) { shape_ok = false; break; }
                if (i.chosen_objects.empty()) { decline = &i; continue; }
                if (i.chosen_objects.size() != 1) { shape_ok = false; break; }
                auto oid = i.chosen_objects.front();
                if (!s.objectExists(oid) || !s.getObject(oid).isSpell()) {
                    shape_ok = false;
                    break;
                }
                pay = &i;
            }
            if (shape_ok && pay && decline) {
                if (rescues) {
                    RescueAsk ask;
                    ask.asked = decline->player;
                    ask.about = pay->chosen_objects.front();
                    ask.counterspell_already_gone =
                        counterspell != kInvalidId &&
                        s.objectExists(counterspell) &&
                        s.getObject(counterspell).zone != ZoneType::Chain;
                    rescues->push_back(ask);
                }
                return *decline;   // never pay — let it be countered
            }
        }

        // ── yes/no poll (Repeat tranche)? ──
        bool all_yes_no = true;
        for (const auto& i : legal) {
            if (i.type != IntentType::MakeChoice || !i.chosen_objects.empty() ||
                !i.chosen_value.has_value()) {
                all_yes_no = false;
                break;
            }
        }
        if (all_yes_no) {
            ++yes_no_seen;
            const bool accept = yes_no_seen <= accept_yes_no;
            for (const auto& i : legal)
                if ((i.chosen_value.value_or(0) == 1) == accept) return i;
            return legal.front();
        }

        // ── the wanted play, once ──
        if (!taken && want && (!gate || gate(s))) {
            for (const auto& i : legal) {
                if (!want(i)) continue;
                taken = true;
                return i;
            }
        }
        for (const auto& i : legal)
            if (i.type == IntentType::PassPriority) return i;
        return legal.front();
    }
};

}  // namespace

// ─── Fixture ───────────────────────────────────────────────────────────────

class RepeatReactionsTest : public CardTestFixture {
protected:
    void SetUp() override {
        CardTestFixture::SetUp();
        g_tally_resolves = g_victim_resolves = 0;
        g_resumable_starts = g_resumable_done = g_plain_resolves = 0;
        card_registry.registerCard(kTallyAction,
                                    std::make_unique<TallyActionSpell>());
        card_registry.registerCard(kVictimSpell,
                                    std::make_unique<VictimReactionSpell>());
        card_registry.registerCard(kResumableRpt,
                                    std::make_unique<ResumableRepeatSpell>());
        card_registry.registerCard(kPlainRepeat,
                                    std::make_unique<PlainRepeatSpell>());
        card_db.buildFromClasses(card_registry);
    }

    /// Main Phase / Neutral Open, P1 to act, two battlefields in the ENGINE's
    /// state (the base fixture's `state` is a different object).
    void primeMainPhase(GameEngine& engine) {
        auto& s = engine.mutableState();
        s.mode             = ModeOfPlay{};
        s.players[0].id    = P1;
        s.players[1].id    = P2;
        s.turn.turn_player = P1;
        s.turn.turn_number = 3;
        s.turn.phase       = TurnPhase::MainPhase;
        s.turn.ns_state    = NeutralShowdownState::Neutral;
        s.turn.oc_state    = OpenClosedState::Open;
        BattlefieldState b0; b0.id = 0; s.battlefields.push_back(b0);
        BattlefieldState b1; b1.id = 1; s.battlefields.push_back(b1);
    }

    GameObjectId addReadyRuneIn(GameState& s, PlayerId owner, Domain d) {
        auto id = s.createObject();
        auto& r = s.getObject(id);
        r.owner = owner;
        r.controller = owner;
        r.card_type = CardType::Rune;
        r.name = "Test Rune";
        r.domains = {d};
        r.zone = ZoneType::Base;
        r.location = BaseLocation{owner};
        r.is_exhausted = false;
        return id;
    }

    GameObjectId addToHandIn(GameState& s, PlayerId owner, CardDefId def_id) {
        auto id = s.createObject();
        auto& obj = s.getObject(id);
        obj.owner = owner;
        obj.controller = owner;
        obj.card_def_id = def_id;
        const auto& def = card_db.get(def_id);
        obj.name = def.name;
        obj.card_type = def.card_type;
        obj.super_type = def.super_type;
        obj.keywords = def.keywords;
        obj.domains = def.domains;
        obj.tags = def.tags;
        obj.zone = ZoneType::Hand;
        s.player(owner).hand.push_back(id);
        return id;
    }

    static bool inTrashIn(const GameState& s, PlayerId p, GameObjectId id) {
        const auto& t = s.player(p).trash;
        return std::find(t.begin(), t.end(), id) != t.end();
    }
};

// ─── (1) Hard Bargain, closed state, one paid Repeat tranche ───────────────

TEST_F(RepeatReactionsTest, HardBargainRepeatTrancheCountersTheSecondSpell) {
    constexpr CardDefId kHardBargain = 457;

    GameEngine engine(card_db, events, card_registry);
    std::vector<ScriptAgent::RescueAsk> rescues;

    ScriptAgent agent1;   // P1: plays Hard Bargain, buys ONE Repeat tranche
    ScriptAgent agent2;   // P2: plays the Victim reaction
    agent1.rescues = &rescues;
    agent2.rescues = &rescues;
    agent1.accept_yes_no = 1;
    agent2.accept_yes_no = 0;

    engine.testHook_setAgents(&agent1, &agent2);
    engine.testHook_initSubsystems();
    primeMainPhase(engine);
    auto& s = engine.mutableState();

    auto tally  = addToHandIn(s, P1, kTallyAction);
    auto hb     = addToHandIn(s, P1, kHardBargain);
    auto victim = addToHandIn(s, P2, kVictimSpell);

    // P1 needs 2E (Hard Bargain) + 2E (one Repeat tranche) and must still
    // hold >= 2 READY runes afterwards, or the tranche's own case 0 would
    // auto-counter without ever asking the rescue question.
    for (int i = 0; i < 8; ++i) addReadyRuneIn(s, P1, Domain::Chaos);
    // P2 pays nothing (the Victim is free) and keeps 4 ready runes, so the
    // FIRST resolution asks its rescue question too.
    for (int i = 0; i < 4; ++i) addReadyRuneIn(s, P2, Domain::Chaos);

    agent1.counterspell = hb;
    agent2.counterspell = hb;
    agent1.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == hb;
    };
    // Hard Bargain only goes on top once the Victim is under it — otherwise
    // P1's first priority window (it controls the opener) would counter the
    // opener itself and the paid tranche would find an empty chain.
    agent1.gate = [](const GameState& gs) { return gs.chain.items.size() >= 2; };
    agent2.want = [&](const Intent& i) {
        return i.type == IntentType::PlayReaction && i.card == victim;
    };

    // P1 opens the chain with the free [Action] Tally spell. Everything after
    // this happens inside the real FEPR loop, in the Closed State.
    Intent open;
    open.type = IntentType::PlayActionCard;
    open.player = P1;
    open.card = tally;
    open.play_source = Intent::PlaySource::Hand;
    engine.testHook_executeIntent(open);

    // Chain drained.
    EXPECT_TRUE(s.chain.items.empty()) << "FEPR should have emptied the chain";
    ASSERT_TRUE(agent1.taken) << "P1 never got to play Hard Bargain";
    ASSERT_TRUE(agent2.taken) << "P2 never got to play the Victim spell";
    EXPECT_EQ(agent1.yes_no_seen, 2)
        << "Repeat poll should offer tranches until declined";

    // Both counter targets were asked their rescue question, and BOTH were
    // asked while Hard Bargain was still the resolving chain item.
    ASSERT_EQ(rescues.size(), 2u)
        << "Each Hard Bargain resolution (base + 1 paid tranche) must ask "
           "its own rescue question";
    EXPECT_EQ(rescues[0].asked, P2);
    EXPECT_EQ(rescues[0].about, victim);
    EXPECT_FALSE(rescues[0].counterspell_already_gone);
    EXPECT_EQ(rescues[1].asked, P1);
    EXPECT_EQ(rescues[1].about, tally);
    EXPECT_FALSE(rescues[1].counterspell_already_gone)
        << "The tranche's question must be asked DURING Hard Bargain's "
           "resolution, not leaked to whatever resolves next";

    // Two counters: neither spell ever resolved, both sit in trash.
    EXPECT_EQ(g_victim_resolves, 0) << "Victim was countered by the base resolution";
    EXPECT_EQ(g_tally_resolves, 0) << "Tally was countered by the Repeat tranche";
    EXPECT_TRUE(inTrashIn(s, P2, victim));
    EXPECT_TRUE(inTrashIn(s, P1, tally));
    EXPECT_TRUE(inTrashIn(s, P1, hb)) << "Hard Bargain itself trashes (CR 359.3)";
}

// ─── (2) The pump, at ChainManager level ───────────────────────────────────

TEST_F(RepeatReactionsTest, ResumableRepeatTranchesRunThroughTheResumePump) {
    EffectExecutor exec(state, events, card_db);
    ChainManager cm(state, events, card_db);
    cm.setEffectExecutor(&exec);

    auto src = state.createObject();
    {
        auto& obj = state.getObject(src);
        obj.owner = P1;
        obj.controller = P1;
        obj.card_def_id = kResumableRpt;
        obj.card_type = CardType::Spell;
        obj.name = card_db.get(kResumableRpt).name;
        obj.zone = ZoneType::Hand;
    }
    cm.addSpell(src, P1, {});
    ASSERT_FALSE(state.chain.items.empty());
    state.chain.items.back().repeats_paid = 2;   // base + 2 tranches = 3 runs

    int agent_answers = 0;
    PickByPredicateAgent agent([&](int, const std::vector<Intent>& legal) {
        for (const auto& i : legal)
            if (i.type == IntentType::PassPriority) return i;
        ++agent_answers;
        for (const auto& i : legal)
            if (i.chosen_value.value_or(0) == 1) return i;   // always "yes"
        return legal.empty() ? Intent{} : legal.front();
    });

    cm.processFEPR(
        [&](PlayerId, const std::vector<Intent>& legal) {
            return agent.selectAction(state, legal);
        },
        [](const ChainItem&) {},
        [&](const ChainItem& item) {
            Card* c = card_registry.get(item.card_def_id);
            ASSERT_NE(c, nullptr);
            CardContext ctx{state, events, exec, item.controller, item.source};
            c->onResolve(ctx, item.targets);
        });

    EXPECT_EQ(g_resumable_starts, 3)
        << "base resolution + 2 paid tranches each enter case 0";
    EXPECT_EQ(agent_answers, 3)
        << "every tranche's published choice must reach the agent";
    EXPECT_EQ(g_resumable_done, 3)
        << "every tranche must consume its answer and finish its effect";
    EXPECT_FALSE(exec.hasPendingChoice())
        << "a tranche must not leave a pending choice for the next card";
}

// ─── (3) Regression: non-resumable [Repeat] still repeats exactly N ────────

TEST_F(RepeatReactionsTest, NonResumableRepeatResolvesExactlyRepeatsPaidPlusOne) {
    EffectExecutor exec(state, events, card_db);
    ChainManager cm(state, events, card_db);
    cm.setEffectExecutor(&exec);

    auto src = state.createObject();
    {
        auto& obj = state.getObject(src);
        obj.owner = P1;
        obj.controller = P1;
        obj.card_def_id = kPlainRepeat;
        obj.card_type = CardType::Spell;
        obj.name = card_db.get(kPlainRepeat).name;
        obj.zone = ZoneType::Hand;
    }
    cm.addSpell(src, P1, {});
    ASSERT_FALSE(state.chain.items.empty());
    state.chain.items.back().repeats_paid = 2;

    FirstChoiceAgent agent;
    cm.processFEPR(
        [&](PlayerId, const std::vector<Intent>& legal) {
            return agent.selectAction(state, legal);
        },
        [](const ChainItem&) {},
        [&](const ChainItem& item) {
            Card* c = card_registry.get(item.card_def_id);
            ASSERT_NE(c, nullptr);
            CardContext ctx{state, events, exec, item.controller, item.source};
            c->onResolve(ctx, item.targets);
        });

    EXPECT_EQ(g_plain_resolves, 3) << "1 base + repeats_paid(2) extra runs";
    EXPECT_FALSE(exec.hasPendingChoice());
}
