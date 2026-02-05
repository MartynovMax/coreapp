#include "test_helpers.hpp"
#include "core/memory/malloc_allocator.hpp"
#include "core/memory/pool_allocator.hpp"
#include "core/memory/stack_allocator.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

TEST(AdvancedWorkloadTest, PoolAllocatorReclaimSemantics) {
    std::vector<u8> buffer(32 * 64);
    PoolAllocator poolAllocator(buffer.data(), buffer.size(), 32);
    SeededRNG rng(600);

    WorkloadParams params{};
    params.seed = 600;
    params.operationCount = 4;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::LongLived;
    params.sizeDistribution = SizeDistribution{DistributionType::Uniform, 8, 16};
    params.alignmentDistribution.type = AlignmentDistributionType::Fixed;
    params.alignmentDistribution.fixedAlignment = CORE_DEFAULT_ALIGNMENT;

    PhaseDescriptor desc{};
    desc.name = "Pool";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::None;

    PhaseContext ctx{};
    ctx.allocator = &poolAllocator;
    ctx.rng = &rng;

    u32 usedBefore = poolAllocator.UsedBlocks();
    
    {
        PhaseExecutor exec(desc, ctx);
        exec.Execute();
    }

    EXPECT_GT(poolAllocator.UsedBlocks(), usedBefore);
    EXPECT_EQ(ctx.allocCount, 4u);
}

struct StackState {
    u32 allocSize = 0;
};

static void StackLifoOp(PhaseContext& ctx, StackState& state) noexcept {
    AllocationRequest req{};
    req.size = state.allocSize;
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    void* ptr = ctx.allocator->Allocate(req);
    if (!ptr) return;
    ctx.allocCount += 1;
    ctx.bytesAllocated += state.allocSize;
    AllocationInfo info{};
    info.ptr = ptr;
    info.size = state.allocSize;
    info.alignment = CORE_DEFAULT_ALIGNMENT;
    ctx.allocator->Deallocate(info);
    ctx.freeCount += 1;
    ctx.bytesFreed += state.allocSize;
}

static void CustomStackOperation(PhaseContext& ctx, const Operation& /*op*/) noexcept {
    auto* state = static_cast<StackState*>(ctx.userData);
    StackLifoOp(ctx, *state);
}

TEST(AdvancedWorkloadTest, StackAllocatorBoundedArena) {
    std::vector<u8> buffer(4096);
    StackAllocator allocator(buffer.data(), buffer.size());
    SeededRNG rng(700);

    WorkloadParams params{};
    params.seed = 700;
    params.operationCount = 50;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Bounded;
    params.maxLiveObjects = 1;
    params.sizeDistribution = SizePresets::SmallObjects();

    StackState state{};
    state.allocSize = 64;

    PhaseDescriptor desc{};
    desc.name = "Stack";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Custom;
    desc.params = params;
    desc.userData = &state;
    desc.customOperation = CustomStackOperation;
    desc.reclaimMode = ReclaimMode::FreeAll;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    {
        PhaseExecutor exec(desc, ctx);
        exec.Execute();
    }

    EXPECT_EQ(allocator.Used(), 0u);
}
