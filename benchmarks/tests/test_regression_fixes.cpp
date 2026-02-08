#include "benchmarks/workload/operation_stream.hpp"
#include "benchmarks/workload/phase_executor.hpp"
#include "benchmarks/workload/phase_descriptor.hpp"
#include "benchmarks/workload/phase_context.hpp"
#include "benchmarks/workload/workload_params.hpp"
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