#include "benchmarks/workload/operation_stream.hpp"
#include "benchmarks/workload/workload_params.hpp"
#include "../common/seeded_rng.hpp"
#include <gtest/gtest.h>
#include <set>
#include <algorithm>

using namespace core;
using namespace core::bench;

TEST(OperationStream, Determinism_SameSeed) {
    WorkloadParams params;
    params.seed = 42;
    params.operationCount = 1000;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;

    OperationStream s1(params);
    OperationStream s2(params);

    for (core::usize i = 0; i < params.operationCount; ++i) {
        auto op1 = s1.Next(0);
        auto op2 = s2.Next(0);
        ASSERT_EQ(op1.type, op2.type);
        ASSERT_EQ(op1.size, op2.size);
    }
}

TEST(OperationStream, Determinism_DifferentSeeds) {
    WorkloadParams params1;
    params1.seed = 1;
    params1.operationCount = 100;
    params1.sizeDistribution = SizePresets::SmallObjects();
    params1.allocFreeRatio = 0.5f;

    WorkloadParams params2 = params1;
    params2.seed = 2;

    OperationStream s1(params1);
    OperationStream s2(params2);

    bool any_diff = false;
    for (core::usize i = 0; i < params1.operationCount; ++i) {
        const auto op1 = s1.Next(0);
        const auto op2 = s2.Next(0);
        if (op1.type != op2.type || op1.size != op2.size)
            any_diff = true;
    }
    ASSERT_TRUE(any_diff);
}

TEST(OperationStream, AllocFreeRatio_OnlyAlloc) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    for (core::usize i = 0; i < params.operationCount; ++i)
        ASSERT_EQ(s.Next(0).type, OpType::Alloc);
}

TEST(OperationStream, AllocFreeRatio_OnlyFree) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 100;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.0f;

    OperationStream s(params);
    for (core::usize i = 0; i < params.operationCount; ++i)
        ASSERT_EQ(s.Next(0).type, OpType::Free);
}

TEST(OperationStream, AllocFreeRatio_Half) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 1000;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;

    OperationStream s(params);
    int allocs = 0, frees = 0;
    u64 liveCount = 0;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        const auto t = s.Next(liveCount).type;
        if (t == OpType::Alloc) {
            ++allocs;
            ++liveCount;
        } else if (t == OpType::Free) {
            ++frees;
            if (liveCount > 0) --liveCount;
        }
    }
    const core::f64 ratio = static_cast<core::f64>(allocs) / static_cast<core::f64>(allocs + frees);
    ASSERT_NEAR(ratio, 0.5, 0.1);
}

TEST(OperationStream, OperationCount_Limit) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 123;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 0.5f;

    OperationStream s(params);
    int count = 0;
    while (s.HasNext()) {
        s.Next(0);
        ++count;
    }
    ASSERT_EQ(count, 123);
}

TEST(OperationStream, UniformDistribution_Range) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 1000;
    params.sizeDistribution.type = DistributionType::Uniform;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 64;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    for (core::usize i = 0; i < params.operationCount; ++i) {
        auto sz = s.Next(0).size;
        ASSERT_GE(sz, 8);
        ASSERT_LE(sz, 64);
    }
}

TEST(OperationStream, PowerOfTwoDistribution_OnlyPowersOfTwo) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 1000;
    params.sizeDistribution.type = DistributionType::PowerOfTwo;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 1024;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    for (core::usize i = 0; i < params.operationCount; ++i) {
        auto sz = s.Next(0).size;
        ASSERT_TRUE((sz & (sz - 1)) == 0);
        ASSERT_GE(sz, 8);
        ASSERT_LE(sz, 1024);
    }
}

TEST(OperationStream, NormalDistribution_Stats) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 10000;
    params.sizeDistribution.type = DistributionType::Normal;
    params.sizeDistribution.mean = 128.0f;
    params.sizeDistribution.stddev = 32.0f;
    params.sizeDistribution.minSize = 32;
    params.sizeDistribution.maxSize = 256;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    core::f64 sum = 0, sum2 = 0;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        auto sz = s.Next(0).size;
        sum += static_cast<core::f64>(sz);
        sum2 += static_cast<core::f64>(sz) * static_cast<core::f64>(sz);
        ASSERT_GE(sz, 32);
        ASSERT_LE(sz, 256);
    }
    const core::f64 mean = sum / static_cast<core::f64>(params.operationCount);
    const core::f64 var = sum2 / static_cast<core::f64>(params.operationCount) - mean * mean;
    ASSERT_NEAR(mean, 128, 8);
    ASSERT_NEAR(std::sqrt(var), 32, 8);
}

TEST(OperationStream, LogNormalDistribution_Shape) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 10000;
    params.sizeDistribution.type = DistributionType::LogNormal;
    params.sizeDistribution.mean = 256.0f;
    params.sizeDistribution.stddev = 512.0f;
    params.sizeDistribution.minSize = 16;
    params.sizeDistribution.maxSize = 4096;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    int small = 0, large = 0;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        const auto sz = s.Next(0).size;
        if (sz < 256) ++small;
        if (sz > 1024) ++large;
    }
    ASSERT_GT(small, 2000);
    ASSERT_GT(large, 500);
}

TEST(OperationStream, ParetoDistribution_8020) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 10000;
    params.sizeDistribution.type = DistributionType::Pareto;
    params.sizeDistribution.minSize = 16;
    params.sizeDistribution.maxSize = 4096;
    params.sizeDistribution.shape = 2.0f;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    int small = 0, large = 0;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        const auto sz = s.Next(0).size;
        if (sz < 128) ++small;
        if (sz > 1024) ++large;
    }
    ASSERT_GT(small, 7000);
    ASSERT_LT(large, 2000);
}

TEST(OperationStream, SmallBiasedDistribution_MostlySmall) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 1000;
    params.sizeDistribution.type = DistributionType::SmallBiased;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 1024;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    int small = 0;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        if (s.Next(0).size <= 64) ++small;
    }
    ASSERT_GT(small, 800);
}

TEST(OperationStream, LargeBiasedDistribution_MostlyLarge) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 1000;
    params.sizeDistribution.type = DistributionType::LargeBiased;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 1024;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);

    const u32 largeMin = (params.sizeDistribution.minSize + 64 < params.sizeDistribution.maxSize)
        ? (params.sizeDistribution.minSize + 64)
        : params.sizeDistribution.minSize;

    int large = 0;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        if (s.Next(0).size >= largeMin) ++large;
    }
    ASSERT_GT(large, params.operationCount * 0.85);
}

TEST(OperationStream, BimodalDistribution_TwoPeaks) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 10000;
    params.sizeDistribution.type = DistributionType::Bimodal;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 8192;
    params.sizeDistribution.peak1Weight = 0.85f;
    params.sizeDistribution.peak1Min = 16;
    params.sizeDistribution.peak1Max = 64;
    params.sizeDistribution.peak2Min = 1024;
    params.sizeDistribution.peak2Max = 2048;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    int peak1 = 0, peak2 = 0;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        const auto sz = s.Next(0).size;
        if (sz >= 16 && sz <= 64) ++peak1;
        if (sz >= 1024 && sz <= 2048) ++peak2;
    }
    ASSERT_GT(peak1, 7000);
    ASSERT_GT(peak2, 1000);
}

TEST(OperationStream, CustomBuckets_ExactMatch) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 1000;
    params.sizeDistribution.type = DistributionType::CustomBuckets;
    static const u32 bucketSizes[3] = {16, 64, 256};
    static const f32 weights[3] = {0.3f, 0.3f, 0.4f};
    params.sizeDistribution.buckets = bucketSizes;
    params.sizeDistribution.weights = weights;
    params.sizeDistribution.bucketCount = 3;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    std::set<u32> seen;
    for (core::usize i = 0; i < params.operationCount; ++i) {
        auto sz = s.Next(0).size;
        seen.insert(sz);
        ASSERT_TRUE(sz == 16 || sz == 64 || sz == 256);
    }
    ASSERT_EQ(seen.size(), 3);
}

TEST(OperationStream, EdgeCase_MinEqualsMax) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 100;
    params.sizeDistribution.type = DistributionType::Uniform;
    params.sizeDistribution.minSize = 42;
    params.sizeDistribution.maxSize = 42;
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    for (core::usize i = 0; i < params.operationCount; ++i)
        ASSERT_EQ(s.Next(0).size, 42);
}

TEST(OperationStream, EdgeCase_SingleOperation) {
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 1;
    params.sizeDistribution = SizePresets::SmallObjects();
    params.allocFreeRatio = 1.0f;

    OperationStream s(params);
    ASSERT_TRUE(s.HasNext());
    s.Next(0);
    ASSERT_FALSE(s.HasNext());
}

TEST(OperationStream, StressTest_1M_Complex) {
    WorkloadParams params;
    params.seed = 123;
    params.operationCount = 1000000;
    params.sizeDistribution.type = DistributionType::LogNormal;
    params.sizeDistribution.mean = 256.0f;
    params.sizeDistribution.stddev = 512.0f;
    params.sizeDistribution.minSize = 8;
    params.sizeDistribution.maxSize = 8192;
    params.allocFreeRatio = 0.6f;

    OperationStream s(params);
    core::usize count = 0;
    while (s.HasNext()) {
        s.Next(0);
        ++count;
    }
    ASSERT_EQ(count, 1000000);
}
