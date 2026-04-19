// =============================================================================
// test_scenario_loader.cpp
// Tests for LoadScenariosFromJson and related scenario_id helpers.
// =============================================================================

#include "../config/scenario_loader.hpp"
#include "../runner/experiment_runner.hpp"   // BuildScenarioId (exposed via anonymous ns — tested indirectly)
#include "../experiments/article1_registry.hpp"
#include "../runner/experiment_registry.hpp"
#include <gtest/gtest.h>
#include <cstring>

using namespace core;
using namespace core::bench;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

#ifndef BENCHMARK_CONFIG_DIR
// Fallback for IDE / manual build: point relative to typical working directory.
#define BENCHMARK_CONFIG_DIR "config"
#endif

static const char* kArticle1JsonPath = BENCHMARK_CONFIG_DIR "/article1_matrix.json";

// ---------------------------------------------------------------------------
// LoadScenariosFromJson — success path
// ---------------------------------------------------------------------------

TEST(ScenarioLoaderTest, LoadsArticle1Json_Succeeds) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    EXPECT_TRUE(r.ok) << "errorMessage: " << r.errorMessage;
}

TEST(ScenarioLoaderTest, LoadsArticle1Json_ExactlyThirtyOneScenarios) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    EXPECT_EQ(r.count, 31u);
}

TEST(ScenarioLoaderTest, AllScenariosHaveNonNullName) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    for (u32 i = 0; i < r.count; ++i) {
        EXPECT_NE(r.scenarios[i].scenarioName, nullptr)
            << "scenario index " << i << " has null name";
    }
}

TEST(ScenarioLoaderTest, AllScenariosHaveNonEmptyName) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    for (u32 i = 0; i < r.count; ++i) {
        EXPECT_NE(r.scenarios[i].scenarioName[0], '\0')
            << "scenario index " << i << " has empty name";
    }
}

TEST(ScenarioLoaderTest, AllScenariosHavePositiveOperationCount) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    for (u32 i = 0; i < r.count; ++i) {
        EXPECT_GT(r.scenarios[i].operationCount, 0u)
            << "scenario '" << r.scenarios[i].scenarioName << "' has zero operationCount";
    }
}

TEST(ScenarioLoaderTest, AllScenariosHaveValidSizeRange) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    for (u32 i = 0; i < r.count; ++i) {
        const AllocBenchConfig& cfg = r.scenarios[i];
        EXPECT_GE(cfg.sizeMin, 1u)
            << "scenario '" << cfg.scenarioName << "' sizeMin == 0";
        EXPECT_GE(cfg.sizeMax, cfg.sizeMin)
            << "scenario '" << cfg.scenarioName << "' sizeMax < sizeMin";
    }
}

TEST(ScenarioLoaderTest, DefaultSeedAppliedToAllScenarios) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    for (u32 i = 0; i < r.count; ++i) {
        EXPECT_GT(r.scenarios[i].seed, 0u)
            << "scenario '" << r.scenarios[i].scenarioName
            << "' has seed=0 (default_seed not applied)";
    }
}

TEST(ScenarioLoaderTest, DefaultRepetitionsAppliedToAllScenarios) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    for (u32 i = 0; i < r.count; ++i) {
        EXPECT_GT(r.scenarios[i].repetitions, 0u)
            << "scenario '" << r.scenarios[i].scenarioName
            << "' has repetitions=0 (default_repetitions not applied)";
    }
}

// ---------------------------------------------------------------------------
// LoadScenariosFromJson — error path
// ---------------------------------------------------------------------------

TEST(ScenarioLoaderTest, NullPath_ReturnsFalse) {
    ScenarioLoadResult r = LoadScenariosFromJson(nullptr);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.errorMessage[0], '\0');
}

TEST(ScenarioLoaderTest, NonExistentFile_ReturnsFalse) {
    ScenarioLoadResult r = LoadScenariosFromJson("__nonexistent_path__/no_such_file.json");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.errorMessage[0], '\0');
}

// ---------------------------------------------------------------------------
// Drift guard: JSON names match the built-in C++ kArticle1Matrix table
// ---------------------------------------------------------------------------

TEST(ScenarioLoaderTest, JsonNamesMatchBuiltInTable) {
    // Load from JSON
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;

    // Build registry from the C++ table
    ExperimentRegistry registry;
    RegisterArticle1Matrix(registry);

    u32 tableCount = 0;
    registry.GetAll(tableCount);
    ASSERT_EQ(tableCount, r.count)
        << "JSON and C++ table have different scenario counts — they are out of sync";

    for (u32 i = 0; i < r.count; ++i) {
        const char* jsonName = r.scenarios[i].scenarioName;
        const ExperimentDescriptor* desc = registry.Find(jsonName);
        EXPECT_NE(desc, nullptr)
            << "JSON scenario '" << jsonName << "' not found in C++ table (index " << i << ")";
    }
}

// Drift guard: workload profile parameters in JSON match the C++ static table.
// This catches silent regressions where article1_matrix.json and article1_registry.cpp
// diverge (e.g. someone updates operation_count in JSON but forgets the C++ constants).
TEST(ScenarioLoaderTest, JsonProfileParamsMatchBuiltInTable) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;

    u32 staticCount = 0;
    const AllocBenchConfig* staticEntries = GetArticle1MatrixEntries(staticCount);
    ASSERT_NE(staticEntries, nullptr);
    ASSERT_EQ(r.count, staticCount)
        << "JSON and C++ static table have different scenario counts";

    for (u32 i = 0; i < r.count; ++i) {
        const AllocBenchConfig& json   = r.scenarios[i];
        const char* name = json.scenarioName;

        // Find matching static entry by name
        const AllocBenchConfig* found = nullptr;
        for (u32 j = 0; j < staticCount; ++j) {
            if (staticEntries[j].scenarioName &&
                strcmp(staticEntries[j].scenarioName, name) == 0) {
                found = &staticEntries[j];
                break;
            }
        }
        ASSERT_NE(found, nullptr) << "JSON scenario '" << name
            << "' not found in C++ static table — names are out of sync";

        EXPECT_EQ(json.sizeMin,        found->sizeMin)
            << "sizeMin mismatch for '" << name << "'";
        EXPECT_EQ(json.sizeMax,        found->sizeMax)
            << "sizeMax mismatch for '" << name << "'";
        EXPECT_EQ(json.operationCount, found->operationCount)
            << "operationCount mismatch for '" << name << "'";
        EXPECT_EQ(json.maxLiveObjects, found->maxLiveObjects)
            << "maxLiveObjects mismatch for '" << name << "'";
        EXPECT_NEAR(json.allocFreeRatio, found->allocFreeRatio, 1e-4f)
            << "allocFreeRatio mismatch for '" << name << "'";
        EXPECT_EQ(json.allocatorType,  found->allocatorType)
            << "allocatorType mismatch for '" << name << "'";
        EXPECT_EQ(json.lifetime,       found->lifetime)
            << "lifetime mismatch for '" << name << "'";
    }
}

// ---------------------------------------------------------------------------
// RegisterLoadedScenario — smoke test
// ---------------------------------------------------------------------------

TEST(ScenarioLoaderTest, RegisterLoadedScenario_PopulatesRegistry) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    ASSERT_GT(r.count, 0u);

    ExperimentRegistry registry;
    RegisterLoadedScenario(registry, r.scenarios[0]);

    EXPECT_EQ(registry.Count(), 1u);
    const ExperimentDescriptor* desc = registry.Find(r.scenarios[0].scenarioName);
    ASSERT_NE(desc, nullptr);
    EXPECT_STREQ(desc->name, r.scenarios[0].scenarioName);
}

TEST(ScenarioLoaderTest, RegisterLoadedScenario_FactoryCreatesExperiment) {
    ScenarioLoadResult r = LoadScenariosFromJson(kArticle1JsonPath);
    ASSERT_TRUE(r.ok) << r.errorMessage;
    ASSERT_GT(r.count, 0u);

    ExperimentRegistry registry;
    RegisterLoadedScenario(registry, r.scenarios[0]);

    const ExperimentDescriptor* desc = registry.Find(r.scenarios[0].scenarioName);
    ASSERT_NE(desc, nullptr);
    ASSERT_NE(desc->factory, nullptr);

    IExperiment* exp = desc->factory();
    ASSERT_NE(exp, nullptr);
    EXPECT_STREQ(exp->Name(), r.scenarios[0].scenarioName);
    delete exp;
}

// ---------------------------------------------------------------------------
// scenario_id determinism
// (BuildScenarioId is in anonymous namespace — test via ExperimentRunner output
//  by checking that two runs of the same config produce the same scenario_id log.
//  Here we test the property via a helper that duplicates the logic.)
// ---------------------------------------------------------------------------

static void BuildScenarioIdForTest(char* buf, size_t sz,
                                    const char* name, u64 seed, u32 reps) noexcept {
    snprintf(buf, sz, "%s|seed=%llu|reps=%u",
             name ? name : "unknown",
             static_cast<unsigned long long>(seed),
             static_cast<unsigned int>(reps));
}

TEST(ScenarioIdTest, SameInputsProduceSameId) {
    char id1[256], id2[256];
    BuildScenarioIdForTest(id1, sizeof(id1), "article1/malloc/fifo/fixed_small", 42, 5);
    BuildScenarioIdForTest(id2, sizeof(id2), "article1/malloc/fifo/fixed_small", 42, 5);
    EXPECT_STREQ(id1, id2);
}

TEST(ScenarioIdTest, DifferentSeedProducesDifferentId) {
    char id1[256], id2[256];
    BuildScenarioIdForTest(id1, sizeof(id1), "article1/malloc/fifo/fixed_small", 1, 5);
    BuildScenarioIdForTest(id2, sizeof(id2), "article1/malloc/fifo/fixed_small", 2, 5);
    EXPECT_STRNE(id1, id2);
}

TEST(ScenarioIdTest, DifferentRepsProducesDifferentId) {
    char id1[256], id2[256];
    BuildScenarioIdForTest(id1, sizeof(id1), "article1/malloc/fifo/fixed_small", 0, 5);
    BuildScenarioIdForTest(id2, sizeof(id2), "article1/malloc/fifo/fixed_small", 0, 10);
    EXPECT_STRNE(id1, id2);
}

TEST(ScenarioIdTest, FormatIsHumanReadable) {
    char id[256];
    BuildScenarioIdForTest(id, sizeof(id), "article1/malloc/fifo/fixed_small", 42, 5);
    // Must start with scenario name
    EXPECT_EQ(strncmp(id, "article1/malloc/fifo/fixed_small", 32), 0);
    // Must contain seed component
    EXPECT_NE(strstr(static_cast<const char*>(id), "seed=42"), nullptr);
    // Must contain reps component
    EXPECT_NE(strstr(static_cast<const char*>(id), "reps=5"), nullptr);
}

TEST(ScenarioIdTest, NullNameFallsBackToUnknown) {
    char id[256];
    BuildScenarioIdForTest(id, sizeof(id), nullptr, 0, 1);
    EXPECT_EQ(strncmp(id, "unknown", 7), 0);
}




