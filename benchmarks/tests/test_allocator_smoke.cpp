// =============================================================================
// test_allocator_smoke.cpp
// Smoke tests: verify that each allocator completes Setup → RunPhases → Teardown
// without assertion failures or crashes.
//
// Task 3 DoD: "For each selected allocator, a basic smoke-run works."
// =============================================================================

#include "test_helpers.hpp"
#include "../experiments/alloc_bench_experiment.hpp"
#include "../common/allocator_capabilities.hpp"
#include "../runner/experiment_params.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;

namespace {

// Helper: run a single scenario through full lifecycle
void SmokeRunScenario(const AllocBenchConfig& cfg) {
    IExperiment* exp = AllocBenchExperiment::Create(cfg);
    ASSERT_NE(exp, nullptr) << "Failed to create experiment: " << cfg.scenarioName;

    ExperimentParams params{};
    params.seed = 42;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    // Should not throw or assert
    exp->Setup(params);
    exp->RunPhases(0);
    exp->Teardown();

    delete exp;
}

AllocBenchConfig MakeSmoke(const char* name, AllocatorType alloc,
                           LifetimeModel life, WorkloadProfile work,
                           u32 sizeMin, u32 sizeMax) {
    AllocBenchConfig cfg{};
    cfg.scenarioName   = name;
    cfg.allocatorType  = alloc;
    cfg.lifetime       = life;
    cfg.workload       = work;
    cfg.operationCount = 500;
    cfg.maxLiveObjects = 100;
    cfg.allocFreeRatio = 0.5f;
    cfg.sizeMin        = sizeMin;
    cfg.sizeMax        = sizeMax;
    return cfg;
}

} // namespace

// ---------------------------------------------------------------------------
// Smoke: malloc (baseline) — full interface, all workloads OK
// ---------------------------------------------------------------------------

TEST(AllocatorSmoke, Malloc_Fifo_FixedSmall) {
    SmokeRunScenario(MakeSmoke("smoke/malloc/fifo/fixed_small",
        AllocatorType::Malloc, LifetimeModel::Fifo,
        WorkloadProfile::FixedSmall, 32, 32));
}

TEST(AllocatorSmoke, Malloc_Random_VariableSize) {
    SmokeRunScenario(MakeSmoke("smoke/malloc/random/variable_size",
        AllocatorType::Malloc, LifetimeModel::Random,
        WorkloadProfile::VariableSize, 8, 512));
}

TEST(AllocatorSmoke, Malloc_LongLived_Churn) {
    auto cfg = MakeSmoke("smoke/malloc/long_lived/churn",
        AllocatorType::Malloc, LifetimeModel::LongLived,
        WorkloadProfile::Churn, 16, 64);
    cfg.allocFreeRatio = 0.6f;
    SmokeRunScenario(cfg);
}

// ---------------------------------------------------------------------------
// Smoke: monotonic_arena — reset-only, LongLived only
// ---------------------------------------------------------------------------

TEST(AllocatorSmoke, MonotonicArena_LongLived_FixedSmall) {
    SmokeRunScenario(MakeSmoke("smoke/monotonic_arena/long_lived/fixed_small",
        AllocatorType::MonotonicArena, LifetimeModel::LongLived,
        WorkloadProfile::FixedSmall, 32, 32));
}

TEST(AllocatorSmoke, MonotonicArena_LongLived_VariableSize) {
    SmokeRunScenario(MakeSmoke("smoke/monotonic_arena/long_lived/variable_size",
        AllocatorType::MonotonicArena, LifetimeModel::LongLived,
        WorkloadProfile::VariableSize, 8, 512));
}

// ---------------------------------------------------------------------------
// Smoke: pool_allocator — fixed-size only
// ---------------------------------------------------------------------------

TEST(AllocatorSmoke, Pool_Fifo_FixedSmall) {
    SmokeRunScenario(MakeSmoke("smoke/pool/fifo/fixed_small",
        AllocatorType::Pool, LifetimeModel::Fifo,
        WorkloadProfile::FixedSmall, 32, 32));
}

TEST(AllocatorSmoke, Pool_LongLived_FixedSmall) {
    SmokeRunScenario(MakeSmoke("smoke/pool/long_lived/fixed_small",
        AllocatorType::Pool, LifetimeModel::LongLived,
        WorkloadProfile::FixedSmall, 32, 32));
}

// ---------------------------------------------------------------------------
// Smoke: segregated_list — full interface, variable sizes OK
// ---------------------------------------------------------------------------

TEST(AllocatorSmoke, SegregatedList_Fifo_FixedSmall) {
    SmokeRunScenario(MakeSmoke("smoke/segregated_list/fifo/fixed_small",
        AllocatorType::SegregatedList, LifetimeModel::Fifo,
        WorkloadProfile::FixedSmall, 32, 32));
}

TEST(AllocatorSmoke, SegregatedList_Random_VariableSize) {
    SmokeRunScenario(MakeSmoke("smoke/segregated_list/random/variable_size",
        AllocatorType::SegregatedList, LifetimeModel::Random,
        WorkloadProfile::VariableSize, 8, 512));
}

TEST(AllocatorSmoke, SegregatedList_Lifo_Churn) {
    auto cfg = MakeSmoke("smoke/segregated_list/lifo/churn",
        AllocatorType::SegregatedList, LifetimeModel::Lifo,
        WorkloadProfile::Churn, 16, 64);
    cfg.allocFreeRatio = 0.6f;
    SmokeRunScenario(cfg);
}

// ---------------------------------------------------------------------------
// Capability validation tests
// ---------------------------------------------------------------------------

TEST(AllocatorCapabilities, ResetOnlyFlags) {
    EXPECT_FALSE(IsResetOnly(AllocatorType::Malloc));
    EXPECT_TRUE (IsResetOnly(AllocatorType::MonotonicArena));
    EXPECT_FALSE(IsResetOnly(AllocatorType::Pool));
    EXPECT_FALSE(IsResetOnly(AllocatorType::SegregatedList));
}

TEST(AllocatorCapabilities, VariableSizeFlags) {
    EXPECT_TRUE (SupportsVariableSize(AllocatorType::Malloc));
    EXPECT_TRUE (SupportsVariableSize(AllocatorType::MonotonicArena));
    EXPECT_FALSE(SupportsVariableSize(AllocatorType::Pool));
    EXPECT_TRUE (SupportsVariableSize(AllocatorType::SegregatedList));
}

TEST(AllocatorCapabilities, BulkReclaimFlags) {
    EXPECT_FALSE(SupportsBulkReclaim(AllocatorType::Malloc));
    EXPECT_TRUE (SupportsBulkReclaim(AllocatorType::MonotonicArena));
    EXPECT_TRUE (SupportsBulkReclaim(AllocatorType::Pool));
    EXPECT_FALSE(SupportsBulkReclaim(AllocatorType::SegregatedList));
}

TEST(AllocatorCapabilities, ValidateCompatibility_ResetOnlyRejectsNonLongLived) {
    EXPECT_NE(nullptr, ValidateScenarioCompatibility(
        AllocatorType::MonotonicArena, WorkloadProfile::FixedSmall, LifetimeModel::Fifo));
    EXPECT_NE(nullptr, ValidateScenarioCompatibility(
        AllocatorType::MonotonicArena, WorkloadProfile::FixedSmall, LifetimeModel::Random));
    EXPECT_EQ(nullptr, ValidateScenarioCompatibility(
        AllocatorType::MonotonicArena, WorkloadProfile::FixedSmall, LifetimeModel::LongLived));
}

TEST(AllocatorCapabilities, ValidateCompatibility_PoolRejectsVariableSize) {
    EXPECT_NE(nullptr, ValidateScenarioCompatibility(
        AllocatorType::Pool, WorkloadProfile::VariableSize, LifetimeModel::Fifo));
    EXPECT_NE(nullptr, ValidateScenarioCompatibility(
        AllocatorType::Pool, WorkloadProfile::Churn, LifetimeModel::Fifo));
    EXPECT_EQ(nullptr, ValidateScenarioCompatibility(
        AllocatorType::Pool, WorkloadProfile::FixedSmall, LifetimeModel::Fifo));
}

TEST(AllocatorCapabilities, ValidateCompatibility_MallocAcceptsAll) {
    EXPECT_EQ(nullptr, ValidateScenarioCompatibility(
        AllocatorType::Malloc, WorkloadProfile::FixedSmall, LifetimeModel::Fifo));
    EXPECT_EQ(nullptr, ValidateScenarioCompatibility(
        AllocatorType::Malloc, WorkloadProfile::VariableSize, LifetimeModel::Random));
    EXPECT_EQ(nullptr, ValidateScenarioCompatibility(
        AllocatorType::Malloc, WorkloadProfile::Churn, LifetimeModel::LongLived));
}


