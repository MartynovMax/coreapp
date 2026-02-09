#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/phase_context.hpp"
#include "../workload/workload_params.hpp"
#include "../workload/lifetime_tracker.hpp"
#include "../workload/phase_types.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_types.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>

#include "core/memory/malloc_allocator.hpp"

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

// =============================================================================
// Acceptance Test Matrix for PhaseExecutor
// 
// This test suite establishes machine-checkable acceptance criteria through:
// - 18 tier-1 scenarios covering all execution mode combinations
// - 7 mandatory invariants that must hold for all executions
// - Determinism verification for reproducibility
//
// Purpose: Eliminate feedback loops by providing executable definition of done.
// =============================================================================

// -----------------------------------------------------------------------------
// Test Scenario Definition
// -----------------------------------------------------------------------------

struct AcceptanceScenario {
    const char* name;
    u64 operationCount;
    bool hasCustomOperation;
    ReclaimMode reclaimMode;
    LifetimeModel lifetimeModel;
    f32 allocFreeRatio;
    u32 maxLiveObjects;  // For Bounded model
    
    // Expected invariants to verify
    bool expectDeterminism;
    bool expectMetricsConsistency;
    bool expectLiveSetCorrectness;
};

// -----------------------------------------------------------------------------
// Invariant Verification Functions
// -----------------------------------------------------------------------------

// Verify metrics consistency (Invariants 1-5)
void VerifyMetricsConsistency(const PhaseStats& stats, ReclaimMode reclaimMode) {
    // Invariant 1: totalFreeCount == freeCount + internalFreeCount + reclaimFreeCount
    EXPECT_EQ(stats.totalFreeCount, stats.freeCount + stats.internalFreeCount + stats.reclaimFreeCount)
        << "Invariant 1 violated: totalFreeCount must equal sum of all free counts";
    
    // Invariant 2: totalBytesFreed == bytesFreed + internalBytesFreed + reclaimBytesFreed
    EXPECT_EQ(stats.totalBytesFreed, stats.bytesFreed + stats.internalBytesFreed + stats.reclaimBytesFreed)
        << "Invariant 2 violated: totalBytesFreed must equal sum of all freed bytes";
    
    // Invariant 3: finalLiveCount <= peakLiveCount
    EXPECT_LE(stats.finalLiveCount, stats.peakLiveCount)
        << "Invariant 3 violated: finalLiveCount cannot exceed peakLiveCount";
    
    // Invariant 4: finalLiveBytes <= peakLiveBytes
    EXPECT_LE(stats.finalLiveBytes, stats.peakLiveBytes)
        << "Invariant 4 violated: finalLiveBytes cannot exceed peakLiveBytes";
    
    // Invariant 5: For ReclaimMode::FreeAll, finalLiveCount and finalLiveBytes must be 0
    if (reclaimMode == ReclaimMode::FreeAll) {
        EXPECT_EQ(stats.finalLiveCount, 0u)
            << "Invariant 5 violated: ReclaimMode::FreeAll must result in finalLiveCount == 0";
        EXPECT_EQ(stats.finalLiveBytes, 0u)
            << "Invariant 5 violated: ReclaimMode::FreeAll must result in finalLiveBytes == 0";
    }
}

// Verify live-set correctness (Invariants 6-7)
void VerifyLiveSetCorrectness(const PhaseStats& stats) {
    // Invariant 6: allocCount >= totalFreeCount (cannot free more than allocated)
    EXPECT_GE(stats.allocCount, stats.totalFreeCount)
        << "Invariant 6 violated: Cannot free more objects than were allocated";
    
    // Invariant 7: finalLiveCount == allocCount - totalFreeCount
    EXPECT_EQ(stats.finalLiveCount, stats.allocCount - stats.totalFreeCount)
        << "Invariant 7 violated: finalLiveCount must equal allocCount - totalFreeCount";
}

// Verify determinism (same seed produces identical results)
void VerifyDeterminism(const AcceptanceScenario& scenario, u64 seed) {
    MallocAllocator allocator1, allocator2;
    SeededRNG rng1(seed), rng2(seed);
    
    WorkloadParams params;
    params.seed = seed;
    params.operationCount = scenario.operationCount;
    params.allocFreeRatio = scenario.allocFreeRatio;
    params.lifetimeModel = scenario.lifetimeModel;
    params.maxLiveObjects = scenario.maxLiveObjects;
    params.sizeDistribution = SizePresets::SmallObjects();
    
    PhaseDescriptor desc1;
    desc1.name = scenario.name;
    desc1.type = PhaseType::Steady;
    desc1.params = params;
    desc1.reclaimMode = scenario.reclaimMode;
    
    PhaseDescriptor desc2 = desc1;
    
    PhaseContext ctx1;
    ctx1.allocator = &allocator1;
    ctx1.callbackRng = &rng1;
    
    PhaseContext ctx2;
    ctx2.allocator = &allocator2;
    ctx2.callbackRng = &rng2;
    
    // Setup external trackers if needed
    LifetimeTracker* externalTracker1 = nullptr;
    LifetimeTracker* externalTracker2 = nullptr;
    
    if (scenario.reclaimMode == ReclaimMode::None && !scenario.hasCustomOperation) {
        u32 capacity = static_cast<u32>(scenario.operationCount > 0 ? scenario.operationCount : 100);
        externalTracker1 = new LifetimeTracker(capacity, scenario.lifetimeModel, rng1, &allocator1);
        externalTracker2 = new LifetimeTracker(capacity, scenario.lifetimeModel, rng2, &allocator2);
        ctx1.externalLifetimeTracker = externalTracker1;
        ctx2.externalLifetimeTracker = externalTracker2;
    }
    
    PhaseExecutor exec1(desc1, ctx1);
    PhaseExecutor exec2(desc2, ctx2);
    
    exec1.Execute();
    exec2.Execute();
    
    const PhaseStats& stats1 = exec1.GetStats();
    const PhaseStats& stats2 = exec2.GetStats();
    
    // Verify exact match of critical metrics
    EXPECT_EQ(stats1.allocCount, stats2.allocCount)
        << "Determinism violated: allocCount differs";
    EXPECT_EQ(stats1.totalFreeCount, stats2.totalFreeCount)
        << "Determinism violated: totalFreeCount differs";
    EXPECT_EQ(stats1.bytesAllocated, stats2.bytesAllocated)
        << "Determinism violated: bytesAllocated differs";
    EXPECT_EQ(stats1.bytesFreed, stats2.bytesFreed)
        << "Determinism violated: bytesFreed differs";
    EXPECT_EQ(stats1.peakLiveCount, stats2.peakLiveCount)
        << "Determinism violated: peakLiveCount differs";
    EXPECT_EQ(stats1.peakLiveBytes, stats2.peakLiveBytes)
        << "Determinism violated: peakLiveBytes differs";
    
    // Cleanup external trackers
    if (externalTracker1) {
        delete externalTracker1;
        delete externalTracker2;
    }
}

// -----------------------------------------------------------------------------
// Tier-1 Test Scenarios (Critical Acceptance Matrix)
// -----------------------------------------------------------------------------

static const AcceptanceScenario kTier1Scenarios[] = {
    // Standard path with FreeAll reclaim
    {
        "Standard_Alloc_FreeAll_Fifo",
        100,     // operationCount
        false,   // hasCustomOperation
        ReclaimMode::FreeAll,
        LifetimeModel::Fifo,
        1.0f,    // allocFreeRatio (alloc only)
        0,       // maxLiveObjects (unused)
        true,    // expectDeterminism
        true,    // expectMetricsConsistency
        true     // expectLiveSetCorrectness
    },
    {
        "Standard_Mixed_FreeAll_Fifo",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Fifo,
        0.5f,    // 50/50 alloc/free
        0,
        true,
        true,
        true
    },
    {
        "Standard_Mixed_FreeAll_Lifo",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Lifo,
        0.5f,
        0,
        true,
        true,
        true
    },
    {
        "Standard_Mixed_FreeAll_Random",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Random,
        0.5f,
        0,
        true,
        true,
        true
    },
    {
        "Standard_Mixed_FreeAll_Bounded",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Bounded,
        0.6f,
        10,      // maxLiveObjects for Bounded
        true,
        true,
        true
    },
    {
        "Standard_Alloc_FreeAll_LongLived",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::LongLived,
        1.0f,    // LongLived requires allocFreeRatio == 1.0
        0,
        true,
        true,
        true
    },
    
    // Standard path with None reclaim (external tracker)
    {
        "Standard_Alloc_None_Fifo",
        100,
        false,
        ReclaimMode::None,
        LifetimeModel::Fifo,
        1.0f,
        0,
        true,
        true,
        true
    },
    {
        "Standard_Mixed_None_Random",
        100,
        false,
        ReclaimMode::None,
        LifetimeModel::Random,
        0.5f,
        0,
        true,
        true,
        true
    },
    {
        "Standard_Alloc_None_LongLived",
        100,
        false,
        ReclaimMode::None,
        LifetimeModel::LongLived,
        1.0f,
        0,
        true,
        true,
        true
    },
    
    // Edge cases
    {
        "Standard_SingleOp_FreeAll_Fifo",
        1,       // Single operation
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Fifo,
        1.0f,
        0,
        true,
        true,
        true
    },
    {
        "Standard_Large_FreeAll_Random",
        10000,   // Large operation count
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Random,
        0.5f,
        0,
        true,
        true,
        true
    },
    {
        "Standard_AllocOnly_FreeAll_Fifo",
        50,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Fifo,
        1.0f,
        0,
        true,
        true,
        true
    },
    
    // Different operation counts
    {
        "Standard_Small_FreeAll_Lifo",
        10,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Lifo,
        0.7f,
        0,
        true,
        true,
        true
    },
    {
        "Standard_Medium_FreeAll_Random",
        500,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Random,
        0.6f,
        0,
        true,
        true,
        true
    },
    
    // Bounded with different limits
    {
        "Standard_Bounded_Small_FreeAll",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Bounded,
        0.8f,
        5,       // Small bound
        true,
        true,
        true
    },
    {
        "Standard_Bounded_Large_FreeAll",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Bounded,
        0.7f,
        50,      // Larger bound
        true,
        true,
        true
    },
    
    // Mixed ratios
    {
        "Standard_HighAlloc_FreeAll_Fifo",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Fifo,
        0.8f,    // 80% alloc
        0,
        true,
        true,
        true
    },
    {
        "Standard_LowAlloc_FreeAll_Random",
        100,
        false,
        ReclaimMode::FreeAll,
        LifetimeModel::Random,
        0.3f,    // 30% alloc
        0,
        true,
        true,
        true
    },
};

// -----------------------------------------------------------------------------
// Parameterized Test Class
// -----------------------------------------------------------------------------

class AcceptanceMatrixTest : public ::testing::TestWithParam<AcceptanceScenario> {
protected:
    void SetUp() override {
        // Reset state for each test
    }
    
    void TearDown() override {
        // Cleanup
    }
};

// -----------------------------------------------------------------------------
// Parameterized Test Implementation
// -----------------------------------------------------------------------------

TEST_P(AcceptanceMatrixTest, Tier1Scenarios) {
    const AcceptanceScenario& scenario = GetParam();
    SCOPED_TRACE(scenario.name);
    
    // Setup phase
    MallocAllocator allocator;
    SeededRNG rng(42);
    
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = scenario.operationCount;
    params.allocFreeRatio = scenario.allocFreeRatio;
    params.lifetimeModel = scenario.lifetimeModel;
    params.maxLiveObjects = scenario.maxLiveObjects;
    params.sizeDistribution = SizePresets::SmallObjects();
    
    PhaseDescriptor desc;
    desc.name = scenario.name;
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = scenario.reclaimMode;
    
    PhaseContext ctx;
    ctx.allocator = &allocator;
    ctx.callbackRng = &rng;
    
    // Setup external tracker if ReclaimMode::None and standard operations
    LifetimeTracker* externalTracker = nullptr;
    if (scenario.reclaimMode == ReclaimMode::None && !scenario.hasCustomOperation) {
        u32 capacity = static_cast<u32>(scenario.operationCount > 0 ? scenario.operationCount : 100);
        externalTracker = new LifetimeTracker(capacity, scenario.lifetimeModel, rng, &allocator);
        ctx.externalLifetimeTracker = externalTracker;
    }
    
    // Execute phase
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    
    const PhaseStats& stats = exec.GetStats();
    
    // Verify invariants based on scenario expectations
    if (scenario.expectMetricsConsistency) {
        VerifyMetricsConsistency(stats, scenario.reclaimMode);
    }
    
    if (scenario.expectLiveSetCorrectness) {
        VerifyLiveSetCorrectness(stats);
    }
    
    if (scenario.expectDeterminism) {
        VerifyDeterminism(scenario, 12345);
    }
    
    // Cleanup
    if (externalTracker) {
        delete externalTracker;
    }
}

INSTANTIATE_TEST_SUITE_P(
    Tier1,
    AcceptanceMatrixTest,
    ::testing::ValuesIn(kTier1Scenarios),
    [](const ::testing::TestParamInfo<AcceptanceScenario>& info) {
        return std::string(info.param.name);
    }
);

// -----------------------------------------------------------------------------
// Additional Acceptance Tests for Custom Operations
// -----------------------------------------------------------------------------

TEST(AcceptanceMatrix, CustomOperation_WithMetricsUpdates) {
    // Custom operation that properly updates metrics
    auto customOp = +[](PhaseContext& ctx, const Operation& op) noexcept {
        core::AllocationRequest req{};
        req.size = 64;
        req.alignment = 16;
        req.tag = 0;
        req.flags = core::AllocationFlags::None;
        
        void* ptr = ctx.allocator->Allocate(req);
        if (ptr) {
            ctx.allocCount++;
            ctx.bytesAllocated += 64;
            
            // Immediately free
            core::AllocationInfo info{};
            info.ptr = ptr;
            info.size = 64;
            info.alignment = 16;
            info.tag = 0;
            ctx.allocator->Deallocate(info);
            ctx.freeCount++;
            ctx.bytesFreed += 64;
        }
    };
    
    MallocAllocator allocator;
    SeededRNG rng(42);
    
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 50;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::LongLived;
    params.sizeDistribution = SizePresets::SmallObjects();
    
    PhaseDescriptor desc;
    desc.name = "CustomOp";
    desc.type = PhaseType::Custom;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::None;
    desc.customOperation = customOp;
    
    PhaseContext ctx;
    ctx.allocator = &allocator;
    ctx.callbackRng = &rng;
    
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    
    const PhaseStats& stats = exec.GetStats();
    
    // Verify metrics were updated
    EXPECT_EQ(stats.allocCount, 50u);
    EXPECT_EQ(stats.freeCount, 50u);
    EXPECT_EQ(stats.bytesAllocated, 50u * 64u);
    EXPECT_EQ(stats.bytesFreed, 50u * 64u);
    
    // Verify consistency invariants
    VerifyMetricsConsistency(stats, ReclaimMode::None);
}

TEST(AcceptanceMatrix, LoopUntilComplete_Determinism) {
    static int opCount;
    
    auto customOp = +[](PhaseContext& ctx, const Operation& op) noexcept {
        opCount++;
    };
    
    auto completionCheck = +[](const PhaseContext& ctx) noexcept -> bool {
        return opCount >= 7;
    };
    
    MallocAllocator allocator;
    SeededRNG rng(42);
    
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 0;  // Loop-until-complete mode
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    
    PhaseDescriptor desc;
    desc.name = "LoopUntilComplete";
    desc.type = PhaseType::Custom;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::None;
    desc.customOperation = customOp;
    desc.completionCheck = completionCheck;
    desc.maxIterations = 100;
    
    PhaseContext ctx;
    ctx.allocator = &allocator;
    ctx.callbackRng = &rng;
    
    // Run twice to verify determinism
    opCount = 0;
    PhaseExecutor exec1(desc, ctx);
    exec1.Execute();
    const PhaseStats& stats1 = exec1.GetStats();
    
    opCount = 0;
    PhaseExecutor exec2(desc, ctx);
    exec2.Execute();
    const PhaseStats& stats2 = exec2.GetStats();
    
    EXPECT_EQ(stats1.issuedOpCount, stats2.issuedOpCount);
    EXPECT_EQ(stats1.issuedOpCount, 7u);
}
