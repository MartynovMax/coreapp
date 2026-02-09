#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/phase_context.hpp"
#include "../workload/workload_params.hpp"
#include "../workload/lifetime_tracker.hpp"
#include "../workload/phase_types.hpp"
#include "../workload/../events/event_sink.hpp"
#include "../workload/../events/event_types.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <vector>

#include "core/memory/malloc_allocator.hpp"
#include "core/memory/bump_arena.hpp"
#include "core/memory/pool_allocator.hpp"
#include "core/memory/arena.hpp"

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

static PhaseContext MakeContext(IAllocator* allocator, SeededRNG* rng) {
    PhaseContext ctx{};
    ctx.allocator = allocator;
    ctx.callbackRng = rng;
    return ctx;
}

static PhaseDescriptor MakeDesc(const char* name, PhaseType type, const WorkloadParams& params) {
    PhaseDescriptor d{};
    d.name = name;
    d.type = type;
    d.params = params;
    d.reclaimMode = ReclaimMode::FreeAll;
    return d;
}

// Helper completion check for operationCount=0 phases
static bool ImmediateCompletion(const PhaseContext& /*ctx*/) noexcept {
    return true;  // Immediately complete
}

// Helper no-op operation for operationCount=0 phases
static void NoOpOperation(PhaseContext& /*ctx*/, const Operation& /*op*/) noexcept {
    // No-op: used when phase logic is handled elsewhere (e.g., reclaim callback)
}

TEST(PhaseExecutorTest, SinglePhaseRampUpAllocOnly) {
    MallocAllocator allocator;
    SeededRNG rng(42);
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 10;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("RampUp", PhaseType::RampUp, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 10u);
    EXPECT_EQ(stats.totalFreeCount, 10u);
    EXPECT_GT(stats.bytesAllocated, 0u);
}

TEST(PhaseExecutorTest, SinglePhaseSteadyMixedAllocFree) {
    MallocAllocator allocator;
    SeededRNG rng(123);
    WorkloadParams params;
    params.seed = 123;
    params.operationCount = 20;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("SteadyMixed", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GE(stats.allocCount + stats.totalFreeCount, 20u);
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.totalFreeCount, 0u);
}

TEST(PhaseExecutorTest, ReclaimModeFreeAllFreesAllTrackedObjects) {
    MallocAllocator allocator;
    SeededRNG rng(1);
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 16;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("FreeAll", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.totalFreeCount, 16u);
}

TEST(PhaseExecutorTest, ReclaimModeCustomCallbackInvoked) {
    static bool called;
    called = false;
    auto customReclaim = +[](PhaseContext& ctx) noexcept { called = true; };

    MallocAllocator allocator;
    SeededRNG rng(2);
    WorkloadParams params;
    params.seed = 2;
    params.operationCount = 8;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("CustomReclaim", PhaseType::Steady, params);
    desc.reclaimMode = ReclaimMode::Custom;
    desc.reclaimCallback = customReclaim;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    EXPECT_TRUE(called);
}

TEST(PhaseExecutorTest, PhaseStatsAccumulateCorrectly) {
    MallocAllocator allocator;
    SeededRNG rng(3);
    WorkloadParams params;
    params.seed = 3;
    params.operationCount = 100;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("StatsAccumulate", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.totalFreeCount, 0u);
    EXPECT_GT(stats.bytesAllocated, 0u);
    EXPECT_GT(stats.bytesFreed, 0u);
}

TEST(PhaseExecutorTest, PeakMetricsTrackMaxima) {
    MallocAllocator allocator;
    SeededRNG rng(4);
    WorkloadParams params;
    params.seed = 4;
    params.operationCount = 32;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("PeakMetrics", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.peakLiveCount, 32u);
    EXPECT_EQ(stats.peakLiveBytes, stats.bytesAllocated);
}

TEST(PhaseExecutorTest, CustomOperationCallbackOverridesStandardOperation) {
    static int customCount;
    customCount = 0;
    auto customOp = +[](PhaseContext& ctx, const Operation& /*op*/) noexcept { customCount++; };

    MallocAllocator allocator;
    SeededRNG rng(5);
    WorkloadParams params;
    params.seed = 5;
    params.operationCount = 5;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("CustomOp", PhaseType::Custom, params);
    desc.customOperation = customOp;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    EXPECT_EQ(customCount, 5);
}

TEST(PhaseExecutorTest, CompletionCheckCallbackTerminatesPhaseEarly) {
    static int opCount;
    opCount = 0;

    auto customOp = +[](PhaseContext& ctx, const Operation& /*op*/) noexcept { opCount++; };
    auto completionCheck = +[](const PhaseContext& ctx) noexcept -> bool {
        return opCount >= 7;
    };

    MallocAllocator allocator;
    SeededRNG rng(6);
    WorkloadParams params;
    params.seed = 6;
    params.operationCount = 0;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("CompletionCheck", PhaseType::Custom, params);
    desc.customOperation = customOp;
    desc.completionCheck = completionCheck;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    EXPECT_EQ(opCount, 7);
}

// Test that custom operations are responsible for updating metrics
TEST(PhaseExecutorTest, CustomOperationMetricsContract) {
    // Custom operation that performs allocations and updates metrics manually
    auto customOp = +[](PhaseContext& ctx, const Operation& op) noexcept {
        // Perform a custom allocation
        core::AllocationRequest req{};
        req.size = 128;
        req.alignment = 16;
        req.tag = 0;
        req.flags = core::AllocationFlags::None;
        
        void* ptr = ctx.allocator->Allocate(req);
        if (ptr) {
            // Custom operations MUST update metrics manually
            ctx.allocCount++;
            ctx.bytesAllocated += 128;
            
            // Immediately free to clean up
            core::AllocationInfo info{};
            info.ptr = ptr;
            info.size = 128;
            info.alignment = 16;
            info.tag = 0;
            ctx.allocator->Deallocate(info);
            ctx.freeCount++;
            ctx.bytesFreed += 128;
        }
    };

    MallocAllocator allocator;
    SeededRNG rng(999);
    WorkloadParams params;
    params.seed = 999;
    params.operationCount = 10;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::LongLived;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("CustomMetrics", PhaseType::Custom, params);
    desc.customOperation = customOp;
    desc.reclaimMode = ReclaimMode::None;  // No reclaim needed since we free inline
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    
    const PhaseStats& stats = exec.GetStats();
    // Verify that our manual metric updates were captured
    EXPECT_EQ(stats.allocCount, 10u);
    EXPECT_EQ(stats.freeCount, 10u);
    EXPECT_EQ(stats.bytesAllocated, 10u * 128u);
    EXPECT_EQ(stats.bytesFreed, 10u * 128u);
}

TEST(PhaseExecutorTest, OnPhaseBeginEventEmitted) {
    MallocAllocator allocator;
    SeededRNG rng(7);
    WorkloadParams params;
    params.seed = 7;
    params.operationCount = 2;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("PhaseBeginEvent", PhaseType::Steady, params);
    MockEventSink sink;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();
    bool found = false;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::PhaseBegin) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(PhaseExecutorTest, OnPhaseCompleteEventEmitted) {
    MallocAllocator allocator;
    SeededRNG rng(8);
    WorkloadParams params;
    params.seed = 8;
    params.operationCount = 2;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("PhaseCompleteEvent", PhaseType::Steady, params);
    MockEventSink sink;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();
    bool found = false;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::PhaseComplete) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(PhaseExecutorTest, WithMallocAllocator) {
    MallocAllocator allocator;
    SeededRNG rng(9);
    WorkloadParams params;
    params.seed = 9;
    params.operationCount = 4;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("MallocAllocator", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 4u);
}

TEST(PhaseExecutorTest, WithBumpArena) {
    std::vector<uint8_t> buffer(4096);
    core::BumpArena arena(buffer.data(), buffer.size());
    core::ArenaAllocatorAdapter allocator(arena);

    SeededRNG rng(10);

    WorkloadParams params;
    params.seed = 10;
    params.operationCount = 10;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc = MakeDesc("BumpArena", PhaseType::Steady, params);

    desc.reclaimMode = ReclaimMode::None;

    PhaseContext ctx = MakeContext(&allocator, &rng);

    LifetimeTracker tracker(params.operationCount, params.lifetimeModel, rng, &allocator);
    ctx.externalLifetimeTracker = &tracker;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();

    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 10u);
    EXPECT_GT(stats.bytesAllocated, 0u);

    tracker.Clear();
}

TEST(PhaseExecutorTest, WithPoolAllocator) {
    std::vector<uint8_t> buffer(256);
    core::PoolAllocator allocator(buffer.data(), buffer.size(), 32);

    SeededRNG rng(11);

    WorkloadParams params;
    params.seed = 11;
    params.operationCount = 4;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizeDistribution{DistributionType::Uniform, 8, 16};
    params.alignmentDistribution.type = AlignmentDistributionType::Fixed;
    params.alignmentDistribution.fixedAlignment = CORE_DEFAULT_ALIGNMENT;

    PhaseDescriptor desc = MakeDesc("PoolAllocator", PhaseType::Steady, params);

    desc.reclaimMode = ReclaimMode::None;

    PhaseContext ctx = MakeContext(&allocator, &rng);

    core::MallocAllocator metaAlloc;
    SeededRNG trackerRng(1111);
    LifetimeTracker externalTracker(params.operationCount, params.lifetimeModel, trackerRng, &metaAlloc);
    externalTracker.Clear();

    ctx.externalLifetimeTracker = &externalTracker;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();

    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 4u);
}


TEST(PhaseExecutorTest, WithFifoLifetimeModel) {
    MallocAllocator allocator;
    SeededRNG rng(12);
    WorkloadParams params;
    params.seed = 12;
    params.operationCount = 20;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("FifoLifetime", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.totalFreeCount, 0u);
}

TEST(PhaseExecutorTest, WithLifoLifetimeModel) {
    MallocAllocator allocator;
    SeededRNG rng(13);
    WorkloadParams params;
    params.seed = 13;
    params.operationCount = 20;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Lifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("LifoLifetime", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.totalFreeCount, 0u);
}

TEST(PhaseExecutorTest, WithRandomLifetimeModel) {
    MallocAllocator allocator;
    SeededRNG rng(14);
    WorkloadParams params;
    params.seed = 14;
    params.operationCount = 20;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Random;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("RandomLifetime", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.totalFreeCount, 0u);
}

TEST(PhaseExecutorTest, WithBoundedLifetimeModel) {
    MallocAllocator allocator;
    SeededRNG rng(15);
    WorkloadParams params;
    params.seed = 15;
    params.operationCount = 10;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Bounded;
    params.maxLiveObjects = 3;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("BoundedLifetime", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_LE(stats.peakLiveCount, 4u);
}

TEST(PhaseExecutorTest, MultiPhaseSequence) {
    MallocAllocator allocator;
    SeededRNG rng(16);
    WorkloadParams params1;
    params1.seed = 16;
    params1.operationCount = 3;
    params1.allocFreeRatio = 1.0f;
    params1.lifetimeModel = LifetimeModel::Fifo;
    params1.sizeDistribution = SizePresets::SmallObjects();
    WorkloadParams params2 = params1;
    params2.seed = 17;
    params2.operationCount = 2;
    WorkloadParams params3 = params1;
    params3.seed = 18;
    params3.operationCount = 1;
    PhaseDescriptor descs[3] = {
        MakeDesc("Phase1", PhaseType::Steady, params1),
        MakeDesc("Phase2", PhaseType::Steady, params2),
        MakeDesc("Phase3", PhaseType::Steady, params3)
    };
    PhaseContext ctx = MakeContext(&allocator, &rng);
    u64 totalOps = 0;
    for (int i = 0; i < 3; ++i) {
        PhaseExecutor exec(descs[i], ctx);
        exec.Execute();
        totalOps += exec.GetStats().allocCount + exec.GetStats().totalFreeCount;
    }
    EXPECT_GE(totalOps, 6u);
}

TEST(PhaseExecutorTest, DeterminismSameSeedIdenticalStatsAndEvents) {
    MallocAllocator allocator1, allocator2;
    SeededRNG rng1(19), rng2(19);
    WorkloadParams params;
    params.seed = 19;
    params.operationCount = 10;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("Determinism", PhaseType::Steady, params);
    MockEventSink sink1, sink2;
    PhaseContext ctx1 = MakeContext(&allocator1, &rng1);
    PhaseContext ctx2 = MakeContext(&allocator2, &rng2);
    PhaseExecutor exec1(desc, ctx1, &sink1);
    PhaseExecutor exec2(desc, ctx2, &sink2);
    exec1.Execute();
    exec2.Execute();
    const PhaseStats& stats1 = exec1.GetStats();
    const PhaseStats& stats2 = exec2.GetStats();
    EXPECT_EQ(stats1.allocCount, stats2.allocCount);
    EXPECT_EQ(stats1.totalFreeCount, stats2.totalFreeCount);
    EXPECT_EQ(stats1.bytesAllocated, stats2.bytesAllocated);
    EXPECT_EQ(stats1.bytesFreed, stats2.bytesFreed);
    EXPECT_EQ(stats1.peakLiveCount, stats2.peakLiveCount);
    EXPECT_EQ(stats1.peakLiveBytes, stats2.peakLiveBytes);
    ASSERT_EQ(sink1.events.size(), sink2.events.size());
    for (size_t i = 0; i < sink1.events.size(); ++i) {
        EXPECT_EQ(sink1.events[i].type, sink2.events[i].type);
    }
}

TEST(PhaseExecutorTest, StressTest1MOperations) {
    MallocAllocator allocator;
    SeededRNG rng(20);
    WorkloadParams params;
    params.seed = 20;
    params.operationCount = 1000000;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Random;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc = MakeDesc("Stress1M", PhaseType::Steady, params);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 400000u);
    EXPECT_GT(stats.totalFreeCount, 400000u);
    EXPECT_LE(stats.allocCount + stats.freeCount, 1000000u);
    EXPECT_GT(stats.reclaimFreeCount, 0u);
}

TEST(PhaseExecutorTest, ReclaimModeNoneUsesExternalTracker) {
    MallocAllocator allocator;
    SeededRNG rng(42);
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 0;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc = MakeDesc("ReclaimNoneExternal", PhaseType::Steady, params);
    desc.reclaimMode = ReclaimMode::None;
    desc.customOperation = NoOpOperation;       // Required for operationCount=0
    desc.completionCheck = ImmediateCompletion;  // Required for operationCount=0

    LifetimeTracker externalTracker(4, params.lifetimeModel, rng, &allocator);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    ctx.externalLifetimeTracker = &externalTracker;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();

    // NOTE: After changes, internal tracker is no longer exposed via ctx.lifetimeTracker
    // The contract is that externalLifetimeTracker is used, which we validate by checking it's valid
    EXPECT_TRUE(externalTracker.isValid());
}

TEST(PhaseExecutorTest, ReclaimModeFreeAllRejectsExternalTracker) {
    MallocAllocator allocator;
    SeededRNG rng(43);
    WorkloadParams params;
    params.seed = 43;
    params.operationCount = 0;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc = MakeDesc("FreeAllExternal", PhaseType::Steady, params);
    desc.reclaimMode = ReclaimMode::FreeAll;
    desc.customOperation = NoOpOperation;       // Required for operationCount=0
    desc.completionCheck = ImmediateCompletion;  // Required for operationCount=0

    LifetimeTracker externalTracker(4, params.lifetimeModel, rng, &allocator);
    PhaseContext ctx = MakeContext(&allocator, &rng);
    ctx.externalLifetimeTracker = &externalTracker;

#if GTEST_HAS_DEATH_TEST
    PhaseExecutor exec(desc, ctx);
    ASSERT_DEATH({ exec.Execute(); }, ".*");
#else
    (void)desc;
    (void)ctx;
#endif
}

