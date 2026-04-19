#include "benchmarks/workload/operation_stream.hpp"
#include "benchmarks/workload/phase_executor.hpp"
#include "benchmarks/workload/phase_descriptor.hpp"
#include "benchmarks/workload/phase_context.hpp"
#include "benchmarks/workload/workload_params.hpp"
#include "benchmarks/events/event_payloads.hpp"
#include "benchmarks/events/event_sink.hpp"
#include "benchmarks/common/seeded_rng.hpp"
#include "core/memory/system_allocator.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;

// ===========================================================================
// Test: Reproducibility (Contract 1)
// ===========================================================================

TEST(Invariants, OperationStream_ReproducibleSameSeed) {
    WorkloadParams params;
    params.seed = 12345ULL;
    params.operationCount = 1000;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.6f;

    OperationStream s1(params);
    OperationStream s2(params);

    for (u64 i = 0; i < params.operationCount; ++i) {
        Operation op1 = s1.Next(0);
        Operation op2 = s2.Next(0);
        ASSERT_EQ(op1.type, op2.type) << "Operation type mismatch at index " << i;
        ASSERT_EQ(op1.size, op2.size) << "Operation size mismatch at index " << i;
        ASSERT_EQ(op1.reason, op2.reason) << "Operation reason mismatch at index " << i;
    }
}

// ===========================================================================
// Test: Strict Intensity (Contract 2)
// ===========================================================================

TEST(Invariants, PhaseExecutor_IssuedOpsEqualsOperationCount) {
    WorkloadParams params;
    params.seed = 42ULL;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.maxLiveObjects = 200;

    PhaseDescriptor desc{};
    desc.name = "TestPhase";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    SeededRNG rng(123ULL);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    PhaseExecutor executor(desc, ctx, nullptr);
    executor.Execute();

    const PhaseStats& stats = executor.GetStats();
    EXPECT_EQ(stats.issuedOpCount, params.operationCount) 
        << "issuedOpCount must equal operationCount regardless of forced/noop";
}

TEST(Invariants, PhaseExecutor_IssuedOpsWithEmptyLiveSet) {
    WorkloadParams params;
    params.seed = 42ULL;
    params.operationCount = 50;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.0f; // Free-only (but live-set is empty)
    params.lifetimeModel = LifetimeModel::Fifo;
    params.maxLiveObjects = 100;

    PhaseDescriptor desc{};
    desc.name = "TestPhase";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    SeededRNG rng(123ULL);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    PhaseExecutor executor(desc, ctx, nullptr);
    executor.Execute();

    const PhaseStats& stats = executor.GetStats();
    EXPECT_EQ(stats.issuedOpCount, params.operationCount) 
        << "issuedOpCount must equal operationCount even with ratio==0 and empty live-set";
    EXPECT_EQ(stats.noopFreeCount, params.operationCount) 
        << "All operations should be noop frees when ratio==0 and liveCount==0";
}

// ===========================================================================
// Test: Bulk Reclaim Metrics (Contract 3)
// ===========================================================================

TEST(Invariants, BulkReclaim_MetricsUseLiveSetTracker) {
    WorkloadParams params;
    params.seed = 42ULL;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 1.0f; // Alloc-only
    params.lifetimeModel = LifetimeModel::Fifo;
    params.maxLiveObjects = 200;

    PhaseDescriptor desc{};
    desc.name = "TestPhase";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    SeededRNG rng(123ULL);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    PhaseExecutor executor(desc, ctx, nullptr);
    executor.Execute();

    const PhaseStats& stats = executor.GetStats();
    EXPECT_GT(stats.preReclaimLiveCount, 0u) 
        << "preReclaimLiveCount should reflect actual live-set before reclaim";
    EXPECT_EQ(stats.finalLiveCount, 0u) 
        << "finalLiveCount should be 0 after FreeAll reclaim";
}

// ===========================================================================
// Test: opCount==0 Do-While Semantics (Contract 4)
// ===========================================================================

TEST(Invariants, OpCountZero_DoWhileRunsOnceEvenIfComplete) {
    WorkloadParams params;
    params.seed = 42ULL;
    params.operationCount = 0; // Special mode
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "TestPhase";
    desc.type = PhaseType::Custom;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::None;
    desc.maxIterations = 10;

    // Use static counter accessible by function pointer callback
    static u32 s_customOpCallCount = 0;
    s_customOpCallCount = 0;

    // completionCheck returns true immediately (stateless lambda)
    desc.completionCheck = [](const PhaseContext&) noexcept -> bool {
        return true; // Complete immediately
    };

    // customOperation just counts calls (stateless, uses static counter)
    desc.customOperation = [](PhaseContext&, const Operation&) noexcept {
        s_customOpCallCount++;
    };

    SeededRNG rng(123ULL);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    PhaseExecutor executor(desc, ctx, nullptr);
    executor.Execute();

    const PhaseStats& stats = executor.GetStats();
    EXPECT_EQ(stats.issuedOpCount, 1u) 
        << "opCount==0 with immediate completion should execute exactly 1 iteration (do-while)";
    EXPECT_EQ(s_customOpCallCount, 1u) 
        << "customOperation should be called exactly once";
}

// ===========================================================================
// Test: Runtime Sanity Checks
// ===========================================================================

TEST(Invariants, SanityChecks_ZeroFailuresForHealthyRun) {
    WorkloadParams params;
    params.seed = 42ULL;
    params.operationCount = 200;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.maxLiveObjects = 200;

    PhaseDescriptor desc{};
    desc.name = "SanityHealthy";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;
    desc.enableSanityChecks = true;

    SeededRNG rng(123ULL);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    PhaseExecutor executor(desc, ctx, nullptr);
    executor.Execute();

    const PhaseStats& stats = executor.GetStats();
    EXPECT_EQ(stats.sanityCheckFailures, 0u)
        << "Healthy run must have zero sanity check failures";
    EXPECT_LE(stats.finalLiveBytes, stats.peakLiveBytes)
        << "peak_live_bytes >= final_live_bytes";
    EXPECT_LE(stats.finalLiveCount, stats.peakLiveCount)
        << "peak_live_count >= final_live_count";
}

TEST(Invariants, SanityChecks_DisabledWhenFlagOff) {
    WorkloadParams params;
    params.seed = 42ULL;
    params.operationCount = 50;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.maxLiveObjects = 100;

    PhaseDescriptor desc{};
    desc.name = "SanityDisabled";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;
    desc.enableSanityChecks = false;

    SeededRNG rng(123ULL);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    PhaseExecutor executor(desc, ctx, nullptr);
    executor.Execute();

    const PhaseStats& stats = executor.GetStats();
    EXPECT_EQ(stats.sanityCheckFailures, 0u)
        << "sanityCheckFailures must be 0 when enableSanityChecks is false";
}

TEST(Invariants, SanityChecks_AllocOnlyPeakEqualsAllocated) {
    WorkloadParams params;
    params.seed = 99ULL;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 1.0f; // Alloc-only
    params.lifetimeModel = LifetimeModel::LongLived;
    params.maxLiveObjects = 200;

    PhaseDescriptor desc{};
    desc.name = "SanityAllocOnly";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;
    desc.enableSanityChecks = true;

    SeededRNG rng(123ULL);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    PhaseExecutor executor(desc, ctx, nullptr);
    executor.Execute();

    const PhaseStats& stats = executor.GetStats();
    EXPECT_EQ(stats.sanityCheckFailures, 0u);
    EXPECT_EQ(stats.peakLiveBytes, stats.bytesAllocated)
        << "With alloc-only workload, peak should equal total allocated";
    EXPECT_EQ(stats.finalLiveCount, 0u)
        << "FreeAll reclaim should leave zero live objects";
}
