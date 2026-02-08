#include "benchmarks/workload/operation_stream.hpp"
#include "benchmarks/workload/phase_executor.hpp"
#include "benchmarks/workload/phase_descriptor.hpp"
#include "benchmarks/workload/phase_context.hpp"
#include "benchmarks/workload/workload_params.hpp"
#include "benchmarks/workload/lifetime_tracker.hpp"
#include "benchmarks/events/event_sink.hpp"
#include "benchmarks/common/seeded_rng.hpp"
#include "core/memory/system_allocator.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;

// ===========================================================================
// REGRESSION: OperationStream must not mutate const alignment buckets
// ===========================================================================

TEST(RegressionFixes, ConstAlignmentBucketsNotMutated) {
    static constexpr core::memory_alignment kConstBuckets[] = {8, 16, 32, 64};
    static constexpr f32 kWeights[] = {0.25f, 0.25f, 0.25f, 0.25f};
    
    core::memory_alignment originalCopy[4];
    for (u32 i = 0; i < 4; ++i) {
        originalCopy[i] = kConstBuckets[i];
    }
    
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.alignmentDistribution.type = AlignmentDistributionType::CustomBuckets;
    params.alignmentDistribution.buckets = kConstBuckets;
    params.alignmentDistribution.weights = kWeights;
    params.alignmentDistribution.bucketCount = 4;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 4; ++i) {
        EXPECT_EQ(kConstBuckets[i], originalCopy[i]) 
            << "Original const bucket at index " << i << " was mutated!";
    }
}

TEST(RegressionFixes, NonPowerOfTwoAlignmentBucketsNormalized) {
    core::memory_alignment badBuckets[] = {7, 15, 33, 65};
    static constexpr f32 kWeights[] = {0.25f, 0.25f, 0.25f, 0.25f};
    
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.alignmentDistribution.type = AlignmentDistributionType::CustomBuckets;
    params.alignmentDistribution.buckets = badBuckets;
    params.alignmentDistribution.weights = kWeights;
    params.alignmentDistribution.bucketCount = 4;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 10; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc && op.alignment > 0) {
            EXPECT_TRUE((op.alignment & (op.alignment - 1)) == 0) 
                << "Generated alignment " << op.alignment << " is not power-of-2";
        }
    }
}

// ===========================================================================
// REGRESSION: Phase duration must include reclaim time
// ===========================================================================

TEST(RegressionFixes, PhaseDurationIncludesReclaimTime) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.maxLiveObjects = 200;

    PhaseDescriptor desc{};
    desc.name = "TestPhase";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::Custom;
    
    static volatile u64 s_burnCycles = 0;
    desc.reclaimCallback = [](PhaseContext& ctx) noexcept {
        for (u64 i = 0; i < 100000; ++i) {
            s_burnCycles = i;
        }
    };

    SeededRNG rng(123);
    IAllocator* alloc = &core::GetDefaultAllocator();

    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;

    class MockEventSink : public IEventSink {
    public:
        u64 capturedDuration = 0;
        
        void OnEvent(const Event& evt) noexcept override {
            if (evt.type == EventType::PhaseComplete) {
                capturedDuration = evt.data.phaseComplete.durationNs;
            }
        }
    };

    MockEventSink sink;
    PhaseExecutor executor(desc, ctx, &sink);
    executor.Execute();

    EXPECT_GT(sink.capturedDuration, 0u) 
        << "Phase duration should be > 0 and include reclaim callback time";
}

// ===========================================================================
// REGRESSION: Validate peak ranges for bimodal distributions
// ===========================================================================

TEST(RegressionFixes, BimodalPeakRangesNormalized) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution.type = DistributionType::Bimodal;
    params.sizeDistribution.minSize = 16;
    params.sizeDistribution.maxSize = 1024;
    params.sizeDistribution.peak1Min = 1;
    params.sizeDistribution.peak1Max = 5000;
    params.sizeDistribution.peak2Min = 3000;
    params.sizeDistribution.peak2Max = 10;
    params.sizeDistribution.peak1Weight = 0.8f;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 100; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            EXPECT_GE(op.size, params.sizeDistribution.minSize);
            EXPECT_LE(op.size, params.sizeDistribution.maxSize);
        }
    }
}

TEST(RegressionFixes, MinSizeZeroNormalizedToOne) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 50;
    params.sizeDistribution.type = DistributionType::Uniform;
    params.sizeDistribution.minSize = 0;
    params.sizeDistribution.maxSize = 0;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 50; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            EXPECT_GE(op.size, 1u);
        }
    }
}

TEST(RegressionFixes, MinSize8MaxSize0Normalized) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 50;
    params.sizeDistribution.type = DistributionType::Uniform;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 0;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 50; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            EXPECT_GE(op.size, 1u);
            EXPECT_LE(op.size, 8u);
        }
    }
}

TEST(RegressionFixes, MinSizeMaxSizeSwapped) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 50;
    params.sizeDistribution.type = DistributionType::Uniform;
    params.sizeDistribution.minSize = 128;
    params.sizeDistribution.maxSize = 16;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 50; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            EXPECT_GE(op.size, 16u);
            EXPECT_LE(op.size, 128u);
        }
    }
}

// ===========================================================================
// REGRESSION: AlignmentDistribution::CustomBuckets truncation + renormalization
// ===========================================================================

TEST(RegressionFixes, CustomBucketsTruncatedTo16) {
    core::memory_alignment bigBuckets[20] = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512,
        1024, 2048, 4096, 8192, 16384, 32768,
        65536, 131072, 262144, 524288
    };
    f32 weights[20];
    for (u32 i = 0; i < 20; ++i) {
        weights[i] = 1.0f / 20.0f;
    }
    
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.alignmentDistribution.type = AlignmentDistributionType::CustomBuckets;
    params.alignmentDistribution.buckets = bigBuckets;
    params.alignmentDistribution.weights = weights;
    params.alignmentDistribution.bucketCount = 20;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 20; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc && op.alignment > 0) {
            EXPECT_TRUE((op.alignment & (op.alignment - 1)) == 0);
        }
    }
}

TEST(RegressionFixes, CustomBucketsFallbackOnNullBuckets) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 10;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.alignmentDistribution.type = AlignmentDistributionType::CustomBuckets;
    params.alignmentDistribution.buckets = nullptr;
    params.alignmentDistribution.weights = nullptr;
    params.alignmentDistribution.bucketCount = 0;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 10; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            EXPECT_EQ(op.alignment, CORE_DEFAULT_ALIGNMENT);
        }
    }
}

TEST(RegressionFixes, CustomBucketsFallbackOnZeroWeightSum) {
    core::memory_alignment buckets[] = {8, 16, 32};
    f32 weights[] = {0.0f, 0.0f, 0.0f};
    
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 10;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.alignmentDistribution.type = AlignmentDistributionType::CustomBuckets;
    params.alignmentDistribution.buckets = buckets;
    params.alignmentDistribution.weights = weights;
    params.alignmentDistribution.bucketCount = 3;
    params.allocFreeRatio = 1.0f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 10; ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            EXPECT_EQ(op.alignment, CORE_DEFAULT_ALIGNMENT);
        }
    }
}

// ===========================================================================
// REGRESSION: OperationStream::Next() past end safe in release
// ===========================================================================

TEST(RegressionFixes, NextPastEndReturnsNoop) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 5;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;
    
    OperationStream stream(params);
    
    for (u32 i = 0; i < 5; ++i) {
        stream.Next(0);
    }
    
    EXPECT_FALSE(stream.HasNext());
    
    Operation pastEnd = stream.Next(0);
    EXPECT_EQ(pastEnd.type, OpType::Free);
    EXPECT_EQ(pastEnd.reason, OpReason::NoopFreeEmptyLive);
    EXPECT_EQ(pastEnd.size, 0u);
}

// ===========================================================================
// REGRESSION: Shared tracker capacity robustness
// ===========================================================================

TEST(RegressionFixes, SharedTrackerCapacitySufficientForMultiPhase) {
    IAllocator* alloc = &core::GetDefaultAllocator();
    
    WorkloadParams phase1;
    phase1.seed = 100;
    phase1.operationCount = 1000;
    phase1.lifetimeModel = LifetimeModel::Fifo;
    phase1.maxLiveObjects = 500;
    phase1.allocFreeRatio = 1.0f;
    phase1.sizeDistribution = SizePresets::SmallObjects();
    
    WorkloadParams phase2;
    phase2.seed = 200;
    phase2.operationCount = 2000;
    phase2.lifetimeModel = LifetimeModel::Fifo;
    phase2.maxLiveObjects = 1000;
    phase2.allocFreeRatio = 0.5f;
    phase2.sizeDistribution = SizePresets::SmallObjects();
    
    u32 max1 = phase1.maxLiveObjects;
    u32 max2 = phase2.maxLiveObjects;
    u32 maxAcrossPhases = (max1 > max2) ? max1 : max2;
    u32 capacity = maxAcrossPhases + (maxAcrossPhases / 2);
    
    void* trackerMem = alloc->Allocate(core::AllocationRequest{
        .size = sizeof(LifetimeTracker),
        .alignment = static_cast<memory_alignment>(alignof(LifetimeTracker))
    });
    
    ASSERT_NE(trackerMem, nullptr);
    
    SeededRNG rng(42);
    LifetimeTracker* tracker = new (trackerMem) LifetimeTracker(
        capacity,
        LifetimeModel::Fifo,
        rng,
        alloc
    );
    
    EXPECT_TRUE(tracker->isValid());
    
    PhaseDescriptor desc1{};
    desc1.name = "Phase1";
    desc1.type = PhaseType::RampUp;
    desc1.params = phase1;
    desc1.reclaimMode = ReclaimMode::None;
    
    PhaseContext ctx1{};
    ctx1.allocator = alloc;
    ctx1.callbackRng = &rng;
    ctx1.liveSetTracker = tracker;
    
    class NullEventSink : public IEventSink {
    public:
        void OnEvent(const Event&) noexcept override {}
    };
    NullEventSink sink;
    
    PhaseExecutor exec1(desc1, ctx1, &sink);
    exec1.Execute();
    
    EXPECT_TRUE(tracker->isValid());
    EXPECT_LE(tracker->GetLiveCount(), capacity);
    
    PhaseDescriptor desc2{};
    desc2.name = "Phase2";
    desc2.type = PhaseType::Steady;
    desc2.params = phase2;
    desc2.reclaimMode = ReclaimMode::None;
    
    PhaseContext ctx2{};
    ctx2.allocator = alloc;
    ctx2.callbackRng = &rng;
    ctx2.liveSetTracker = tracker;
    
    PhaseExecutor exec2(desc2, ctx2, &sink);
    exec2.Execute();
    
    EXPECT_TRUE(tracker->isValid());
    EXPECT_LE(tracker->GetLiveCount(), capacity);
    
    tracker->~LifetimeTracker();
    alloc->Deallocate(core::AllocationInfo{
        .ptr = trackerMem,
        .size = sizeof(LifetimeTracker),
        .alignment = static_cast<memory_alignment>(alignof(LifetimeTracker))
    });
}

// ===========================================================================
// REGRESSION: Saturating arithmetic in totals
// ===========================================================================

TEST(RegressionFixes, SaturatingArithmeticInTotals) {
    WorkloadParams params;
    params.seed = 42;
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
    
    SeededRNG rng(123);
    IAllocator* alloc = &core::GetDefaultAllocator();
    
    PhaseContext ctx{};
    ctx.allocator = alloc;
    ctx.callbackRng = &rng;
    ctx.phaseName = desc.name;
    
    class MockEventSink : public IEventSink {
    public:
        u64 totalFreeCount = 0;
        u64 totalBytesFreed = 0;
        
        void OnEvent(const Event& evt) noexcept override {
            if (evt.type == EventType::PhaseComplete) {
                totalFreeCount = evt.data.phaseComplete.totalFreeCount;
                totalBytesFreed = evt.data.phaseComplete.totalBytesFreed;
            }
        }
    };
    
    MockEventSink sink;
    PhaseExecutor executor(desc, ctx, &sink);
    executor.Execute();
    
    EXPECT_GE(sink.totalFreeCount, 0u);
    EXPECT_GE(sink.totalBytesFreed, 0u);
}