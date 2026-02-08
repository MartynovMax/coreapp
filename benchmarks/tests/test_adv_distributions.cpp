#include "test_helpers.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>
#include <unordered_set>
#include <vector>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;
using namespace core::bench::test::advanced;

static void RunDistributionPhase(const SizeDistribution& dist, DistributionType type) {
    MallocAllocator allocator;
    SeededRNG rng(800);
    WorkloadParams params{};
    params.seed = 800;
    params.operationCount = 2000;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = dist;

    PhaseDescriptor desc{};
    desc.name = "Dist";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.callbackRng = &rng;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, params.operationCount);
    EXPECT_GT(stats.bytesAllocated, 0u);
    EXPECT_EQ(dist.type, type);
}

TEST(AdvancedWorkloadTest, NormalDistributionExperiment) {
    SizeDistribution dist{};
    dist.type = DistributionType::Normal;
    dist.minSize = 8;
    dist.maxSize = 1024;
    dist.mean = 128.0f;
    dist.stddev = 64.0f;
    RunDistributionPhase(dist, DistributionType::Normal);
}

TEST(AdvancedWorkloadTest, LogNormalDistributionExperiment) {
    RunDistributionPhase(SizePresets::WebServer(), DistributionType::LogNormal);
}

TEST(AdvancedWorkloadTest, ParetoDistributionExperiment) {
    SizeDistribution dist{};
    dist.type = DistributionType::Pareto;
    dist.minSize = 16;
    dist.maxSize = 2048;
    dist.shape = 1.2f;
    RunDistributionPhase(dist, DistributionType::Pareto);
}

TEST(AdvancedWorkloadTest, BimodalDistributionExperiment) {
    RunDistributionPhase(SizePresets::GameEntityPool(), DistributionType::Bimodal);
}

TEST(AdvancedWorkloadTest, CustomBucketsDistributionExperiment) {
    static const u32 buckets[3] = {4096, 8192, 16384};
    static const f32 weights[3] = {0.2f, 0.5f, 0.3f};
    SizeDistribution dist{};
    dist.type = DistributionType::CustomBuckets;
    dist.minSize = 4096;
    dist.maxSize = 16384;
    dist.buckets = buckets;
    dist.weights = weights;
    dist.bucketCount = 3;
    RunDistributionPhase(dist, DistributionType::CustomBuckets);
}

TEST(AdvancedWorkloadTest, CustomBucketsSamplesMatchBuckets) {
    static const u32 buckets[3] = {4096, 8192, 16384};
    static const f32 weights[3] = {0.2f, 0.5f, 0.3f};
    WorkloadParams params{};
    params.seed = 900;
    params.operationCount = 100;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution.type = DistributionType::CustomBuckets;
    params.sizeDistribution.minSize = 4096;
    params.sizeDistribution.maxSize = 16384;
    params.sizeDistribution.buckets = buckets;
    params.sizeDistribution.weights = weights;
    params.sizeDistribution.bucketCount = 3;

    std::vector<u32> sizes = SampleSizes(params, 100, 900);
    std::unordered_set<u32> allowed{4096, 8192, 16384};
    ASSERT_FALSE(sizes.empty());
    for (u32 size : sizes) {
        EXPECT_TRUE(allowed.find(size) != allowed.end());
    }
}
