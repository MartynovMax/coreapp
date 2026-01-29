#include "test_helpers.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

TEST(AdvancedWorkloadTest, OneMillionOperationsExperiment) {
    MallocAllocator allocator;
    SeededRNG rng(1000);
    WorkloadParams params{};
    params.seed = 1000;
    params.operationCount = 1000000;
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Bounded;
    params.maxLiveObjects = 1024;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "Million";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Steady;
    desc.params = params;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_GT(stats.allocCount, 0u);
    EXPECT_GT(stats.freeCount, 0u);
}

TEST(AdvancedWorkloadTest, TickEventsEvery10KOperations) {
    MallocAllocator allocator;
    SeededRNG rng(1100);
    WorkloadParams params{};
    params.seed = 1100;
    params.operationCount = 100000;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.tickInterval = 10000;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "Ticks";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Steady;
    desc.params = params;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    MockEventSink sink;
    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();

    u32 tickCount = 0;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::Tick) {
            tickCount++;
        }
    }
    EXPECT_EQ(tickCount, 10u);
}

TEST(AdvancedWorkloadTest, LongRunningNoLeaks) {
    MallocAllocator allocator;
    SeededRNG rng(1200);
    WorkloadParams params{};
    params.seed = 1200;
    params.operationCount = 20000;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "LeakFree";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    for (u32 i = 0; i < 5; ++i) {
        SeededRNG runRng(params.seed + i);
        PhaseContext ctx{};
        ctx.allocator = &allocator;
        ctx.rng = &runRng;
        PhaseExecutor exec(desc, ctx);
        exec.Execute();
        const PhaseStats& stats = exec.GetStats();
        EXPECT_EQ(stats.freeCount, stats.allocCount);
    }
}

TEST(AdvancedWorkloadTest, LongRunningNoPerformanceRegression) {
    MallocAllocator allocator;
    WorkloadParams params{};
    params.seed = 1300;
    params.operationCount = 200000;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    MockEventSink sink1;
    SeededRNG rng1(1300);
    PhaseDescriptor desc{};
    desc.name = "Perf";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Steady;
    desc.params = params;

    PhaseContext ctx1{};
    ctx1.allocator = &allocator;
    ctx1.rng = &rng1;
    PhaseExecutor exec1(desc, ctx1, &sink1);
    exec1.Execute();

    MockEventSink sink2;
    SeededRNG rng2(1300);
    PhaseContext ctx2{};
    ctx2.allocator = &allocator;
    ctx2.rng = &rng2;
    PhaseExecutor exec2(desc, ctx2, &sink2);
    exec2.Execute();

    Event evt1{};
    Event evt2{};
    for (const auto& evt : sink1.events) {
        if (evt.type == EventType::PhaseComplete) {
            evt1 = evt;
        }
    }
    for (const auto& evt : sink2.events) {
        if (evt.type == EventType::PhaseComplete) {
            evt2 = evt;
        }
    }

    EXPECT_GT(evt1.data.phaseComplete.opsPerSec, 0.0);
    EXPECT_GT(evt2.data.phaseComplete.opsPerSec, 0.0);
    EXPECT_GE(evt2.data.phaseComplete.opsPerSec, evt1.data.phaseComplete.opsPerSec * 0.1);
}

TEST(AdvancedWorkloadTest, DeterminismTenRunsSameSeed) {
    MallocAllocator allocator;
    WorkloadParams params{};
    params.seed = 1400;
    params.operationCount = 20000;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "Determinism";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Steady;
    desc.params = params;

    MockEventSink baseSink;
    SeededRNG baseRng(1400);
    PhaseContext baseCtx{};
    baseCtx.allocator = &allocator;
    baseCtx.rng = &baseRng;
    PhaseExecutor baseExec(desc, baseCtx, &baseSink);
    baseExec.Execute();
    const PhaseStats baseStats = baseExec.GetStats();

    std::vector<EventType> baseSeq;
    baseSeq.reserve(baseSink.events.size());
    for (const auto& evt : baseSink.events) {
        baseSeq.push_back(evt.type);
    }

    for (u32 i = 0; i < 10; ++i) {
        MockEventSink sink;
        SeededRNG rng(1400);
        PhaseContext ctx{};
        ctx.allocator = &allocator;
        ctx.rng = &rng;
        PhaseExecutor exec(desc, ctx, &sink);
        exec.Execute();
        const PhaseStats stats = exec.GetStats();
        EXPECT_EQ(stats.allocCount, baseStats.allocCount);
        EXPECT_EQ(stats.freeCount, baseStats.freeCount);
        EXPECT_EQ(stats.bytesAllocated, baseStats.bytesAllocated);
        EXPECT_EQ(stats.bytesFreed, baseStats.bytesFreed);
        EXPECT_EQ(stats.peakLiveCount, baseStats.peakLiveCount);
        EXPECT_EQ(stats.peakLiveBytes, baseStats.peakLiveBytes);

        ASSERT_EQ(sink.events.size(), baseSeq.size());
        for (size_t j = 0; j < baseSeq.size(); ++j) {
            EXPECT_EQ(sink.events[j].type, baseSeq[j]);
        }
    }
}
