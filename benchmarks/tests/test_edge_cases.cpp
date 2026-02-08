// =============================================================================
// test_edge_cases.cpp
// Edge case tests for distributions, overflow, and alignment validation
// =============================================================================

#include "benchmarks/workload/operation_stream.hpp"
#include "benchmarks/workload/lifetime_tracker.hpp"
#include "benchmarks/workload/workload_params.hpp"
#include "../common/seeded_rng.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <limits>

using namespace core;
using namespace core::bench;

// =============================================================================
// AllocFreeRatio Edge Cases
// =============================================================================

TEST(EdgeCases, AllocFreeRatio_Zero) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.0f;  // All frees
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    u32 freeCount = 0;
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Free) ++freeCount;
    }
    
    ASSERT_EQ(freeCount, params.operationCount);
}

TEST(EdgeCases, AllocFreeRatio_One) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 1.0f;  // All allocs
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    u32 allocCount = 0;
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) ++allocCount;
    }
    
    ASSERT_EQ(allocCount, params.operationCount);
}

TEST(EdgeCases, AllocFreeRatio_NaN) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = std::numeric_limits<f32>::quiet_NaN();
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    // Should normalize to 0.0 and generate all frees
    u32 freeCount = 0;
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Free) ++freeCount;
    }
    
    ASSERT_EQ(freeCount, params.operationCount);
}

TEST(EdgeCases, AllocFreeRatio_Negative) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = -1.0f;
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    // Should clamp to 0.0 and generate all frees
    u32 freeCount = 0;
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Free) ++freeCount;
    }
    
    ASSERT_EQ(freeCount, params.operationCount);
}

TEST(EdgeCases, AllocFreeRatio_OverOne) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 2.0f;
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    // Should clamp to 1.0 and generate all allocs
    u32 allocCount = 0;
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) ++allocCount;
    }
    
    ASSERT_EQ(allocCount, params.operationCount);
}

// =============================================================================
// Alignment Edge Cases
// =============================================================================

TEST(EdgeCases, Alignment_PowerOfTwoRange_Valid) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.alignmentDistribution.type = AlignmentDistributionType::PowerOfTwoRange;
    params.alignmentDistribution.minAlignment = 8;
    params.alignmentDistribution.maxAlignment = 64;
    
    SeededRNG rng(params.seed);
    
    // Should not assert - valid power-of-2 values
    ASSERT_NO_FATAL_FAILURE({
        OperationStream stream(params, rng);
    });
}

TEST(EdgeCases, Alignment_Zero) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 10;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.alignmentDistribution.type = AlignmentDistributionType::Fixed;
    params.alignmentDistribution.fixedAlignment = 0;  // 0 means allocator default
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        ASSERT_EQ(op.alignment, 0);  // Should preserve 0
    }
}

// =============================================================================
// Distribution Edge Cases
// =============================================================================

TEST(EdgeCases, Distribution_MinEqualsMax) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution.type = DistributionType::Uniform;
    params.sizeDistribution.minSize = 64;
    params.sizeDistribution.maxSize = 64;  // Same as min
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            ASSERT_EQ(op.size, 64);  // All sizes should be exactly 64
        }
    }
}

TEST(EdgeCases, Distribution_LogNormal_ZeroMean) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution.type = DistributionType::LogNormal;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 256;
    params.sizeDistribution.mean = 0.0f;  // Edge case
    params.sizeDistribution.stddev = 1.0f;
    params.sizeDistribution.meanInLogSpace = false;
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    // Should handle gracefully without division by zero
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            ASSERT_GE(op.size, params.sizeDistribution.minSize);
            ASSERT_LE(op.size, params.sizeDistribution.maxSize);
        }
    }
}

TEST(EdgeCases, Distribution_LogNormal_VerySmallMean) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution.type = DistributionType::LogNormal;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 256;
    params.sizeDistribution.mean = 1e-10f;  // Very small
    params.sizeDistribution.stddev = 1.0f;
    params.sizeDistribution.meanInLogSpace = false;
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    // Should handle without division issues
    for (u32 i = 0; i < params.operationCount && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            ASSERT_GE(op.size, params.sizeDistribution.minSize);
            ASSERT_LE(op.size, params.sizeDistribution.maxSize);
        }
    }
}

// =============================================================================
// LifetimeTracker Overflow Edge Cases
// =============================================================================

TEST(EdgeCases, LifetimeTracker_CapacityRoundedToPow2) {
    SeededRNG rng(42);
    IAllocator* alloc = &core::GetDefaultAllocator();
    
    // Non-power-of-2 capacity should be rounded up
    LifetimeTracker tracker(100, LifetimeModel::Fifo, rng, alloc);
    
    ASSERT_TRUE(tracker.isValid());
    // Capacity should be rounded to 128 (next power of 2)
    ASSERT_EQ(tracker.GetCapacity(), 128);
}

TEST(EdgeCases, LifetimeTracker_MaxCapacity) {
    SeededRNG rng(42);
    IAllocator* alloc = &core::GetDefaultAllocator();
    
    // Very large capacity (will be rounded to power of 2)
    const u32 largeCapacity = 1024 * 1024;  // 1M
    LifetimeTracker tracker(largeCapacity, LifetimeModel::Random, rng, alloc);
    
    ASSERT_TRUE(tracker.isValid());
    ASSERT_GE(tracker.GetCapacity(), largeCapacity);
}

// =============================================================================
// LifetimeTracker Track Error Cases
// =============================================================================

TEST(EdgeCases, LifetimeTracker_TrackNullPointer) {
    SeededRNG rng(42);
    IAllocator* alloc = &core::GetDefaultAllocator();
    LifetimeTracker tracker(100, LifetimeModel::Fifo, rng, alloc);
    
    auto result = tracker.Track(nullptr, 64, 8, 0, 0);
    
    ASSERT_FALSE(result.tracked);
    ASSERT_EQ(result.error, TrackError::InvalidInput);
}

TEST(EdgeCases, LifetimeTracker_TrackZeroSize) {
    SeededRNG rng(42);
    IAllocator* alloc = &core::GetDefaultAllocator();
    LifetimeTracker tracker(100, LifetimeModel::Fifo, rng, alloc);
    
    void* dummyPtr = reinterpret_cast<void*>(0x1000);
    auto result = tracker.Track(dummyPtr, 0, 8, 0, 0);
    
    ASSERT_FALSE(result.tracked);
    ASSERT_EQ(result.error, TrackError::InvalidInput);
}

TEST(EdgeCases, LifetimeTracker_BoundedModelForcedFree) {
    SeededRNG rng(42);
    IAllocator* alloc = &core::GetDefaultAllocator();
    const u32 capacity = 8;  // Small capacity to trigger overflow
    LifetimeTracker tracker(capacity, LifetimeModel::Bounded, rng, alloc);
    
    // Fill to capacity
    for (u32 i = 0; i < tracker.GetCapacity(); ++i) {
        AllocationRequest req{};
        req.size = 64;
        req.alignment = 8;
        void* ptr = alloc->Allocate(req);
        ASSERT_NE(ptr, nullptr);
        
        auto result = tracker.Track(ptr, 64, 8, 0, i);
        ASSERT_TRUE(result.tracked);
    }
    
    ASSERT_EQ(tracker.GetLiveCount(), tracker.GetCapacity());
    
    // Next track should force-free oldest
    AllocationRequest req{};
    req.size = 64;
    req.alignment = 8;
    void* ptr = alloc->Allocate(req);
    ASSERT_NE(ptr, nullptr);
    
    auto result = tracker.Track(ptr, 64, 8, 0, tracker.GetCapacity());
    ASSERT_TRUE(result.tracked);
    ASSERT_TRUE(result.forcedFree);  // Should have freed oldest
    
    // Cleanup
    alloc->Deallocate({ptr, 64, 8, 0});
}

// =============================================================================
// Next() with liveCount Edge Cases
// =============================================================================

TEST(EdgeCases, OperationStream_NextWithZeroLiveCount) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;
    
    SeededRNG rng(params.seed);
    OperationStream stream(params, rng);
    
    // When liveCount is 0, should never generate Free
    for (u32 i = 0; i < 50 && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);  // liveCount = 0
        ASSERT_EQ(op.type, OpType::Alloc) << "Should only generate Alloc when liveCount=0";
    }
}
