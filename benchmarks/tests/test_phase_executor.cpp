#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/phase_context.hpp"
#include "../workload/workload_params.hpp"
#include "../workload/operation_stream.hpp"
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
    ctx.rng = rng;
    return ctx;
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
    PhaseDescriptor desc{};
    desc.name = "RampUpAlloc";
    desc.type = PhaseType::RampUp;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 10u);
    EXPECT_EQ(stats.freeCount, 0u);
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
    PhaseDescriptor desc{};
    desc.name = "SteadyMixed";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount + stats.freeCount, 20u);
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.freeCount, 0u);
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
    PhaseDescriptor desc{};
    desc.name = "FreeAll";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.freeCount, 16u);
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
    PhaseDescriptor desc{};
    desc.name = "CustomReclaim";
    desc.type = PhaseType::Steady;
    desc.params = params;
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
    PhaseDescriptor desc{};
    desc.name = "StatsAccumulate";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.freeCount, 0u);
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
    PhaseDescriptor desc{};
    desc.name = "PeakMetrics";
    desc.type = PhaseType::Steady;
    desc.params = params;
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
    auto customOp = +[](PhaseContext& ctx) noexcept { customCount++; };

    MallocAllocator allocator;
    SeededRNG rng(5);
    WorkloadParams params;
    params.seed = 5;
    params.operationCount = 5;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc{};
    desc.name = "CustomOp";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.customOperation = customOp;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    EXPECT_EQ(customCount, 5);
}

TEST(PhaseExecutorTest, CompletionCheckCallbackTerminatesPhaseEarly) {
    static int opCount;
    opCount = 0;

    auto customOp = +[](PhaseContext& ctx) noexcept { opCount++; };
    auto completionCheck = +[](const PhaseContext& ctx) noexcept -> bool {
        return opCount >= 7;
    };

    MallocAllocator allocator;
    SeededRNG rng(6);
    WorkloadParams params;
    params.seed = 6;
    params.operationCount = 100;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();
    PhaseDescriptor desc{};
    desc.name = "CompletionCheck";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.customOperation = customOp;
    desc.completionCheck = completionCheck;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    EXPECT_EQ(opCount, 7);
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
    PhaseDescriptor desc{};
    desc.name = "PhaseBeginEvent";
    desc.type = PhaseType::Steady;
    desc.params = params;
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
    PhaseDescriptor desc{};
    desc.name = "PhaseCompleteEvent";
    desc.type = PhaseType::Steady;
    desc.params = params;
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
    PhaseDescriptor desc{};
    desc.name = "MallocAllocator";
    desc.type = PhaseType::Steady;
    desc.params = params;
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
    PhaseDescriptor desc{};
    desc.name = "BumpArena";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 10u);
    EXPECT_GT(stats.bytesAllocated, 0u);
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
    PhaseDescriptor desc{};
    desc.name = "PoolAllocator";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
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
    PhaseDescriptor desc{};
    desc.name = "FifoLifetime";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.freeCount, 0u);
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
    PhaseDescriptor desc{};
    desc.name = "LifoLifetime";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.freeCount, 0u);
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
    PhaseDescriptor desc{};
    desc.name = "RandomLifetime";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.freeCount, 0u);
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
    PhaseDescriptor desc{};
    desc.name = "BoundedLifetime";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_LE(stats.peakLiveCount, 3u);
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
        {"Phase1", "", PhaseType::Steady, 0, params1},
        {"Phase2", "", PhaseType::Steady, 0, params2},
        {"Phase3", "", PhaseType::Steady, 0, params3}
    };
    PhaseContext ctx = MakeContext(&allocator, &rng);
    u64 totalOps = 0;
    for (int i = 0; i < 3; ++i) {
        PhaseExecutor exec(descs[i], ctx);
        exec.Execute();
        totalOps += exec.GetStats().allocCount + exec.GetStats().freeCount;
    }
    EXPECT_EQ(totalOps, 6u);
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
    PhaseDescriptor desc{};
    desc.name = "Determinism";
    desc.type = PhaseType::Steady;
    desc.params = params;
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
    EXPECT_EQ(stats1.freeCount, stats2.freeCount);
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
    PhaseDescriptor desc{};
    desc.name = "Stress1M";
    desc.type = PhaseType::Steady;
    desc.params = params;
    PhaseContext ctx = MakeContext(&allocator, &rng);
    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 400000u);
    EXPECT_GT(stats.freeCount, 400000u);
    EXPECT_LT(stats.allocCount + stats.freeCount, 1000000u);
}
