#include "test_helpers.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>

namespace core::bench::test::advanced {

namespace {

void BulkReclaimSharedTracker(PhaseContext& ctx) noexcept {
    auto* t = static_cast<LifetimeTracker*>(ctx.userData);
    ASSERT(t != nullptr);
    ASSERT(t->isValid());

    u64 freedCount = 0;
    u64 freedBytes = 0;
    t->FreeAll(&freedCount, &freedBytes);

    ctx.reclaimFreeCount += freedCount;
    ctx.reclaimBytesFreed += freedBytes;

    ASSERT(t->GetLiveCount() == 0);
    ASSERT(t->GetLiveBytes() == 0);
}

} // namespace

TEST(AdvancedWorkloadTest, FivePhaseExperimentSequence) {
    MallocAllocator allocator;
    SeededRNG trackerRng(111);
    LifetimeTracker tracker(50000, LifetimeModel::Fifo, trackerRng, &allocator);
    MockEventSink sink;

    WorkloadParams ramp1{};
    ramp1.seed = 1;
    ramp1.operationCount = 5000;
    ramp1.allocFreeRatio = 1.0f;
    ramp1.lifetimeModel = LifetimeModel::Fifo;
    ramp1.sizeDistribution = SizePresets::SmallObjects();

    WorkloadParams steady{};
    steady.seed = 2;
    steady.operationCount = 8000;
    steady.allocFreeRatio = 0.5f;
    steady.lifetimeModel = LifetimeModel::Fifo;
    steady.sizeDistribution = SizePresets::SmallObjects();

    WorkloadParams drain{};
    drain.seed = 3;
    drain.operationCount = 10000;
    drain.allocFreeRatio = 0.0f;
    drain.lifetimeModel = LifetimeModel::Fifo;
    drain.sizeDistribution = SizePresets::SmallObjects();

    WorkloadParams ramp2 = ramp1;
    ramp2.seed = 4;
    ramp2.operationCount = 3000;

    WorkloadParams reclaim{};
    reclaim.seed = 5;
    reclaim.operationCount = 0;
    reclaim.allocFreeRatio = 0.0f;
    reclaim.lifetimeModel = LifetimeModel::Fifo;
    reclaim.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor p1{};
    p1.name = "RampUp1";
    p1.experimentName = "Advanced";
    p1.type = PhaseType::RampUp;
    p1.params = ramp1;
    p1.reclaimMode = ReclaimMode::None;

    PhaseDescriptor p2{};
    p2.name = "Steady";
    p2.experimentName = "Advanced";
    p2.type = PhaseType::Steady;
    p2.params = steady;
    p2.reclaimMode = ReclaimMode::None;

    PhaseDescriptor p3{};
    p3.name = "Drain";
    p3.experimentName = "Advanced";
    p3.type = PhaseType::Drain;
    p3.params = drain;
    p3.reclaimMode = ReclaimMode::None;

    PhaseDescriptor p4{};
    p4.name = "RampUp2";
    p4.experimentName = "Advanced";
    p4.type = PhaseType::RampUp;
    p4.params = ramp2;
    p4.reclaimMode = ReclaimMode::None;

    PhaseDescriptor p5{};
    p5.name = "BulkReclaim";
    p5.experimentName = "Advanced";
    p5.type = PhaseType::BulkReclaim;
    p5.params = reclaim;
    p5.reclaimMode = ReclaimMode::Custom;
    p5.reclaimCallback = BulkReclaimSharedTracker;
    p5.userData = &tracker;

    RunPhaseWithTracker(p1, &allocator, &tracker, &sink);
    const u64 afterRamp1 = tracker.GetLiveCount();
    RunPhaseWithTracker(p2, &allocator, &tracker, &sink);
    const u64 afterSteady = tracker.GetLiveCount();
    RunPhaseWithTracker(p3, &allocator, &tracker, &sink);
    const u64 afterDrain = tracker.GetLiveCount();
    RunPhaseWithTracker(p4, &allocator, &tracker, &sink);
    const u64 afterRamp2 = tracker.GetLiveCount();
    RunPhaseWithTracker(p5, &allocator, &tracker, &sink);
    const u64 afterReclaim = tracker.GetLiveCount();

    EXPECT_GT(afterRamp1, 0u);
    EXPECT_GT(afterSteady, 0u);
    EXPECT_LT(afterDrain, afterSteady);
    EXPECT_GT(afterRamp2, afterDrain);
    EXPECT_EQ(afterReclaim, 0u);
}

TEST(AdvancedWorkloadTest, PhaseTransitionMetricsContinuity) {
    MallocAllocator allocator;
    SeededRNG trackerRng(222);
    LifetimeTracker tracker(20000, LifetimeModel::Fifo, trackerRng, &allocator);
    MockEventSink sink;

    WorkloadParams ramp{};
    ramp.seed = 10;
    ramp.operationCount = 2000;
    ramp.allocFreeRatio = 1.0f;
    ramp.lifetimeModel = LifetimeModel::Fifo;
    ramp.sizeDistribution = SizePresets::SmallObjects();

    WorkloadParams steady{};
    steady.seed = 11;
    steady.operationCount = 2000;
    steady.allocFreeRatio = 0.5f;
    steady.lifetimeModel = LifetimeModel::Fifo;
    steady.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor p1{};
    p1.name = "RampUp";
    p1.experimentName = "Continuity";
    p1.type = PhaseType::RampUp;
    p1.params = ramp;
    p1.reclaimMode = ReclaimMode::None;

    PhaseDescriptor p2{};
    p2.name = "Steady";
    p2.experimentName = "Continuity";
    p2.type = PhaseType::Steady;
    p2.params = steady;
    p2.reclaimMode = ReclaimMode::None;

    RunPhaseWithTracker(p1, &allocator, &tracker, &sink);
    const u64 countAfterRamp = tracker.GetLiveCount();
    RunPhaseWithTracker(p2, &allocator, &tracker, &sink);
    const u64 countAfterSteady = tracker.GetLiveCount();

    EXPECT_GT(countAfterRamp, 0u);
    EXPECT_GT(countAfterSteady, 0u);
}

} // namespace core::bench::test::advanced
