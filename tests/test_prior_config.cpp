/// @file test_prior_config.cpp
/// Unit tests for `PriorConfig` / `FamilyWeights` / `CorpusWeights` /
/// `loadPriorConfig` and the `intentFamilyWeight` lookup factored out of
/// `MctsAgent`'s `Prior()` (L4 — prior injection).
///
/// Spec: docs/superpowers/specs/2026-09-07-replay-loop-iter0-design.md
/// ("L4 — prior injection").
///
/// `Prior()` itself lives in `src/agents/mcts_agent.cpp`, which is
/// compiled into the `riftbound` executable (it needs the OpenSpiel
/// registrar TUs) and so is not linkable from `riftbound_tests` — same
/// situation as `corpus_evaluator.h` documents for the six-term
/// evaluator. `intentFamilyWeight` is the pure, OpenSpiel-free lookup
/// `Prior()` delegates to for "which family does this legal action
/// belong to, and what unnormalized weight does that family get" — it
/// lives in `agent_spec.{h,cpp}` (riftbound_core) precisely so this file
/// can pin it, the way `corpus_evaluator.h` pins the six evaluator terms.
///
/// The two guards this file exists for:
///   1. `FamilyWeights` / `CorpusWeights` defaults equal today's
///      hard-coded numbers — so a future edit to those literals must go
///      through the struct, not silently drift the defaults.
///   2. `loadPriorConfig` parses schema v1 exactly: every key optional,
///      unknown keys are errors, a missing/unparsable file is an error,
///      and a partial override merges with (does not replace) the
///      defaults.

#include "agents/agent_spec.h"

#include <gtest/gtest.h>

#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace riftbound {
namespace {

// Same resilient path resolution as tests/test_deck_validator.cpp — the
// test binary's cwd varies with how it's invoked (ctest vs. directly
// from the repo root vs. from build/).
std::string samplePriorPath() {
    for (auto& prefix : {"../docs/", "../../docs/", "docs/"}) {
        auto p = std::string(prefix) + "superpowers/priors/example.prior.json";
        if (std::filesystem::exists(p)) return p;
    }
    const char* root = std::getenv("RIFTBOUND_ROOT");
    if (root) {
        auto path = std::filesystem::path(root) / "docs" / "superpowers" /
                    "priors" / "example.prior.json";
        if (std::filesystem::exists(path)) return path.string();
    }
    return "docs/superpowers/priors/example.prior.json";
}

// ── FamilyWeights defaults == Prior()'s hard-coded literals ─────────────────

TEST(FamilyWeightsDefaults, MatchTheHardCodedPriorWeights) {
    FamilyWeights f;
    EXPECT_DOUBLE_EQ(f.play, 4.0);
    EXPECT_DOUBLE_EQ(f.combat_damage, 4.0);
    EXPECT_DOUBLE_EQ(f.move_to_battlefield, 3.0);
    EXPECT_DOUBLE_EQ(f.move_to_base, 0.6);
    EXPECT_DOUBLE_EQ(f.activate, 2.5);
    EXPECT_DOUBLE_EQ(f.choice, 1.5);
    EXPECT_DOUBLE_EQ(f.setup, 1.0);
    EXPECT_DOUBLE_EQ(f.pass, 0.3);
    EXPECT_DOUBLE_EQ(f.concede, 0.01);
}

// ── CorpusWeights defaults == the kCorpus* constants ─────────────────────────

TEST(CorpusWeightsDefaults, MatchTheCorpusEvaluatorConstants) {
    CorpusWeights w;
    EXPECT_DOUBLE_EQ(w.score, kCorpusWeightScore);
    EXPECT_DOUBLE_EQ(w.battlefield, kCorpusWeightBattlefield);
    EXPECT_DOUBLE_EQ(w.unit, kCorpusWeightUnit);
    EXPECT_DOUBLE_EQ(w.held_interaction, kCorpusWeightHeldInteraction);
    EXPECT_DOUBLE_EQ(w.trash_resource, kCorpusWeightTrashResource);
    EXPECT_DOUBLE_EQ(w.empowered_legend, kCorpusWeightEmpoweredLegend);
}

// ── PriorConfig default-constructs to those same defaults ───────────────────

TEST(PriorConfigDefaults, DefaultConstructsToTheHardCodedWeights) {
    PriorConfig p;
    EXPECT_DOUBLE_EQ(p.family.play, 4.0);
    EXPECT_DOUBLE_EQ(p.evaluator.score, kCorpusWeightScore);
}

// ── intentFamilyWeight: the pure lookup Prior() delegates to ────────────────

TEST(IntentFamilyWeight, PlayFamilyCoversAllThreePlayIntents) {
    FamilyWeights f;
    for (auto t : {IntentType::PlayCard, IntentType::PlayReaction,
                   IntentType::PlayActionCard}) {
        EXPECT_DOUBLE_EQ(intentFamilyWeight(t, /*moves_to_battlefield=*/false, f),
                         4.0);
    }
}

TEST(IntentFamilyWeight, CombatDamageFamily) {
    FamilyWeights f;
    EXPECT_DOUBLE_EQ(
        intentFamilyWeight(IntentType::AssignCombatDamage, false, f), 4.0);
}

TEST(IntentFamilyWeight, StandardMoveSplitsOnDestination) {
    FamilyWeights f;
    EXPECT_DOUBLE_EQ(
        intentFamilyWeight(IntentType::StandardMove, /*moves_to_battlefield=*/true, f),
        3.0);
    EXPECT_DOUBLE_EQ(
        intentFamilyWeight(IntentType::StandardMove, /*moves_to_battlefield=*/false, f),
        0.6);
}

TEST(IntentFamilyWeight, ActivateFamilyCoversAllThreeActivateIntents) {
    FamilyWeights f;
    for (auto t : {IntentType::ActivateAbility, IntentType::ActivateReactionAbility,
                   IntentType::ActivateActionAbility}) {
        EXPECT_DOUBLE_EQ(intentFamilyWeight(t, false, f), 2.5);
    }
}

TEST(IntentFamilyWeight, ChoiceFamily) {
    FamilyWeights f;
    EXPECT_DOUBLE_EQ(intentFamilyWeight(IntentType::MakeChoice, false, f), 1.5);
}

TEST(IntentFamilyWeight, SetupFamilyCoversMulliganChooseBattlefieldAndPlayFirst) {
    FamilyWeights f;
    for (auto t : {IntentType::MulliganDecision, IntentType::ChooseBattlefield,
                   IntentType::PlayFirstDecision}) {
        EXPECT_DOUBLE_EQ(intentFamilyWeight(t, false, f), 1.0);
    }
}

TEST(IntentFamilyWeight, PassFamilyCoversEndTurnPassPriorityAndPassFocus) {
    FamilyWeights f;
    for (auto t : {IntentType::EndTurn, IntentType::PassPriority,
                   IntentType::PassFocus}) {
        EXPECT_DOUBLE_EQ(intentFamilyWeight(t, false, f), 0.3);
    }
}

TEST(IntentFamilyWeight, ConcedeFamily) {
    FamilyWeights f;
    EXPECT_DOUBLE_EQ(intentFamilyWeight(IntentType::Concede, false, f), 0.01);
}

TEST(IntentFamilyWeight, UnclassifiedIntentTypesDefaultToOne) {
    FamilyWeights f;
    EXPECT_DOUBLE_EQ(intentFamilyWeight(IntentType::HideCard, false, f), 1.0);
    EXPECT_DOUBLE_EQ(intentFamilyWeight(IntentType::SideboardSwap, false, f), 1.0);
}

TEST(IntentFamilyWeight, AnOverriddenFamilyWeightIsReflected) {
    FamilyWeights f;
    f.play = 9.0;
    EXPECT_DOUBLE_EQ(intentFamilyWeight(IntentType::PlayCard, false, f), 9.0);
    // Other families are untouched by the override.
    EXPECT_DOUBLE_EQ(intentFamilyWeight(IntentType::Concede, false, f), 0.01);
}

// ── loadPriorConfig: schema v1 parse/validate ────────────────────────────────

class TempJsonFile {
public:
    explicit TempJsonFile(const std::string& contents) {
        static std::atomic<int> counter{0};
        path_ = (std::filesystem::temp_directory_path() /
                 ("rb_test_prior_" + std::to_string(::getpid()) + "_" +
                  std::to_string(counter++) + ".json"))
                    .string();
        std::ofstream f(path_);
        f << contents;
    }
    ~TempJsonFile() { std::remove(path_.c_str()); }
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

TEST(LoadPriorConfig, MissingFileThrows) {
    EXPECT_THROW(loadPriorConfig("/nonexistent/path/to/a.prior.json"),
                 std::runtime_error);
}

TEST(LoadPriorConfig, BadJsonThrows) {
    TempJsonFile f("{ not valid json ");
    EXPECT_THROW(loadPriorConfig(f.path()), std::runtime_error);
}

TEST(LoadPriorConfig, UnknownTopLevelKeyThrowsAndNamesIt) {
    TempJsonFile f(R"({"schema": 1, "bogus_key": true})");
    try {
        loadPriorConfig(f.path());
        FAIL() << "expected loadPriorConfig to throw";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("bogus_key"), std::string::npos) << msg;
    }
}

TEST(LoadPriorConfig, UnknownFamilyKeyThrowsAndNamesIt) {
    TempJsonFile f(R"({"action_family_weights": {"play": 5.0, "typo": 1.0}})");
    try {
        loadPriorConfig(f.path());
        FAIL() << "expected loadPriorConfig to throw";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("typo"), std::string::npos) << msg;
    }
}

TEST(LoadPriorConfig, UnknownEvaluatorKeyThrowsAndNamesIt) {
    TempJsonFile f(R"({"evaluator_weights": {"score": 2.0, "typo": 1.0}})");
    try {
        loadPriorConfig(f.path());
        FAIL() << "expected loadPriorConfig to throw";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("typo"), std::string::npos) << msg;
    }
}

TEST(LoadPriorConfig, EmptyObjectYieldsAllDefaults) {
    TempJsonFile f("{}");
    auto p = loadPriorConfig(f.path());
    EXPECT_DOUBLE_EQ(p.family.play, 4.0);
    EXPECT_DOUBLE_EQ(p.evaluator.score, kCorpusWeightScore);
}

TEST(LoadPriorConfig, PartialFamilyOverrideMergesWithDefaults) {
    TempJsonFile f(R"({"action_family_weights": {"play": 9.0}})");
    auto p = loadPriorConfig(f.path());
    EXPECT_DOUBLE_EQ(p.family.play, 9.0);
    // Everything else in the family stays default.
    EXPECT_DOUBLE_EQ(p.family.combat_damage, 4.0);
    EXPECT_DOUBLE_EQ(p.family.concede, 0.01);
}

TEST(LoadPriorConfig, PartialEvaluatorOverrideMergesWithDefaults) {
    TempJsonFile f(R"({"evaluator_weights": {"battlefield": 2.0}})");
    auto p = loadPriorConfig(f.path());
    EXPECT_DOUBLE_EQ(p.evaluator.battlefield, 2.0);
    EXPECT_DOUBLE_EQ(p.evaluator.score, kCorpusWeightScore);
    EXPECT_DOUBLE_EQ(p.evaluator.unit, kCorpusWeightUnit);
}

TEST(LoadPriorConfig, FullSchemaAllKeysParsesAndOverridesEverything) {
    TempJsonFile f(R"({
        "schema": 1,
        "matchup": "kennen-vs-rengar",
        "written": "2026-09-07",
        "action_family_weights": {
            "play": 5.0, "combat_damage": 5.5, "move_to_battlefield": 3.5,
            "move_to_base": 0.7, "activate": 2.6, "choice": 1.6,
            "setup": 1.1, "pass": 0.35, "concede": 0.02
        },
        "evaluator_weights": {
            "score": 1.1, "battlefield": 0.55, "unit": 0.3,
            "held_interaction": 0.2, "trash_resource": 0.12,
            "empowered_legend": 0.11
        }
    })");
    auto p = loadPriorConfig(f.path());
    EXPECT_DOUBLE_EQ(p.family.play, 5.0);
    EXPECT_DOUBLE_EQ(p.family.combat_damage, 5.5);
    EXPECT_DOUBLE_EQ(p.family.move_to_battlefield, 3.5);
    EXPECT_DOUBLE_EQ(p.family.move_to_base, 0.7);
    EXPECT_DOUBLE_EQ(p.family.activate, 2.6);
    EXPECT_DOUBLE_EQ(p.family.choice, 1.6);
    EXPECT_DOUBLE_EQ(p.family.setup, 1.1);
    EXPECT_DOUBLE_EQ(p.family.pass, 0.35);
    EXPECT_DOUBLE_EQ(p.family.concede, 0.02);
    EXPECT_DOUBLE_EQ(p.evaluator.score, 1.1);
    EXPECT_DOUBLE_EQ(p.evaluator.battlefield, 0.55);
    EXPECT_DOUBLE_EQ(p.evaluator.unit, 0.3);
    EXPECT_DOUBLE_EQ(p.evaluator.held_interaction, 0.2);
    EXPECT_DOUBLE_EQ(p.evaluator.trash_resource, 0.12);
    EXPECT_DOUBLE_EQ(p.evaluator.empowered_legend, 0.11);
}

// A non-numeric value where a number belongs: nlohmann throws
// json::type_error, which is NOT a std::runtime_error, so the header's
// documented contract was broken and the message named nothing the user
// wrote. Both sections are covered — they read their fields through the same
// guard.

TEST(LoadPriorConfig, NonNumericFamilyValueThrowsRuntimeErrorNamingTheKey) {
    TempJsonFile f(R"({"action_family_weights": {"play": "high"}})");
    try {
        loadPriorConfig(f.path());
        FAIL() << "expected loadPriorConfig to throw";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("play"), std::string::npos) << msg;
    }
}

TEST(LoadPriorConfig, NonNumericEvaluatorValueThrowsRuntimeErrorNamingTheKey) {
    TempJsonFile f(R"({"evaluator_weights": {"battlefield": true}})");
    try {
        loadPriorConfig(f.path());
        FAIL() << "expected loadPriorConfig to throw";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("battlefield"), std::string::npos) << msg;
    }
}

TEST(LoadPriorConfig, TheCommittedExampleFileParsesToDefaults) {
    auto p = loadPriorConfig(samplePriorPath());
    EXPECT_DOUBLE_EQ(p.family.play, 4.0);
    EXPECT_DOUBLE_EQ(p.family.concede, 0.01);
    EXPECT_DOUBLE_EQ(p.evaluator.score, kCorpusWeightScore);
    EXPECT_DOUBLE_EQ(p.evaluator.empowered_legend, kCorpusWeightEmpoweredLegend);
}

}  // namespace
}  // namespace riftbound
