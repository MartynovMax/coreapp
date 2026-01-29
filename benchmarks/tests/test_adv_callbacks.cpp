#include "test_helpers.hpp"
#include "core/memory/malloc_allocator.hpp"
#include "core/memory/bump_arena.hpp"
#include "core/memory/arena.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

TEST(AdvancedWorkloadTest, ArenaCallbacksResetRewindGetMarker) {
    std::vector<u8> buffer(4096);
    BumpArena arena(buffer.data(), buffer.size());

    const auto marker = arena.GetMarker();
    void* p1 = arena.Allocate(128, CORE_DEFAULT_ALIGNMENT);
    void* p2 = arena.Allocate(256, CORE_DEFAULT_ALIGNMENT);
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_GT(arena.Used(), 0u);

    arena.RewindTo(marker);
    EXPECT_EQ(arena.Used(), 0u);

    arena.Allocate(64, CORE_DEFAULT_ALIGNMENT);
    EXPECT_GT(arena.Used(), 0u);
    arena.Reset();
    EXPECT_EQ(arena.Used(), 0u);
}

struct ThresholdState {
    std::vector<void*> ptrs;
    u32 allocSize = 0;
};

static ThresholdState* gThresholdState = nullptr;

static void ThresholdOp(PhaseContext& ctx) noexcept {
    if (!gThresholdState) return;
    AllocationRequest req{};
    req.size = gThresholdState->allocSize;
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    void* ptr = ctx.allocator->Allocate(req);
    if (!ptr) return;
    gThresholdState->ptrs.push_back(ptr);
    ctx.allocCount += 1;
    ctx.bytesAllocated += gThresholdState->allocSize;
}

static bool ThresholdComplete(const PhaseContext& ctx) noexcept {
    return ctx.bytesAllocated >= 1024;
}

TEST(AdvancedWorkloadTest, ConditionalPhaseCompletionMemoryThreshold) {
    MallocAllocator allocator;
    SeededRNG rng(300);
    WorkloadParams params{};
    params.seed = 300;
    params.operationCount = 1000;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    ThresholdState state{};
    state.allocSize = 256;
    gThresholdState = &state;

    PhaseDescriptor desc{};
    desc.name = "Threshold";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Custom;
    desc.params = params;
    desc.customOperation = &ThresholdOp;
    desc.completionCheck = &ThresholdComplete;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 4u);
    EXPECT_EQ(stats.bytesAllocated, 1024u);

    for (void* ptr : state.ptrs) {
        AllocationInfo info{};
        info.ptr = ptr;
        info.size = state.allocSize;
        info.alignment = CORE_DEFAULT_ALIGNMENT;
        allocator.Deallocate(info);
    }
    gThresholdState = nullptr;
}

struct BatchState {
    std::vector<void*> ptrs;
    u32 batchSize = 0;
    u32 allocSize = 0;
};

static BatchState* gBatchState = nullptr;

static void BatchAllocOp(PhaseContext& ctx) noexcept {
    if (!gBatchState) return;
    for (u32 i = 0; i < gBatchState->batchSize; ++i) {
        AllocationRequest req{};
        req.size = gBatchState->allocSize;
        req.alignment = CORE_DEFAULT_ALIGNMENT;
        void* ptr = ctx.allocator->Allocate(req);
        if (!ptr) continue;
        gBatchState->ptrs.push_back(ptr);
        ctx.allocCount += 1;
        ctx.bytesAllocated += gBatchState->allocSize;
    }
}

TEST(AdvancedWorkloadTest, CustomOperationBatchedAllocations) {
    MallocAllocator allocator;
    SeededRNG rng(400);
    WorkloadParams params{};
    params.seed = 400;
    params.operationCount = 3;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    BatchState state{};
    state.batchSize = 4;
    state.allocSize = 64;
    gBatchState = &state;

    PhaseDescriptor desc{};
    desc.name = "Batch";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Custom;
    desc.params = params;
    desc.customOperation = &BatchAllocOp;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();
    const PhaseStats& stats = exec.GetStats();
    EXPECT_EQ(stats.allocCount, 12u);
    EXPECT_EQ(stats.bytesAllocated, 12u * 64u);

    for (void* ptr : state.ptrs) {
        AllocationInfo info{};
        info.ptr = ptr;
        info.size = state.allocSize;
        info.alignment = CORE_DEFAULT_ALIGNMENT;
        allocator.Deallocate(info);
    }
    gBatchState = nullptr;
}

struct UserDataState {
    void* expected = nullptr;
    u32 opCalls = 0;
    u32 reclaimCalls = 0;
    bool opOk = false;
    bool reclaimOk = false;
};

static UserDataState* gUserDataState = nullptr;

static void UserDataOp(PhaseContext& ctx) noexcept {
    if (!gUserDataState) return;
    gUserDataState->opCalls += 1;
    gUserDataState->opOk = (ctx.userData == gUserDataState->expected);
}

static void UserDataReclaim(PhaseContext& ctx) noexcept {
    if (!gUserDataState) return;
    gUserDataState->reclaimCalls += 1;
    gUserDataState->reclaimOk = (ctx.userData == gUserDataState->expected);
}

TEST(AdvancedWorkloadTest, UserDataPropagationThroughCallbacks) {
    MallocAllocator allocator;
    SeededRNG rng(500);
    WorkloadParams params{};
    params.seed = 500;
    params.operationCount = 5;
    params.allocFreeRatio = 1.0f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    UserDataState state{};
    state.expected = &state;
    gUserDataState = &state;

    PhaseDescriptor desc{};
    desc.name = "UserData";
    desc.experimentName = "Advanced";
    desc.type = PhaseType::Custom;
    desc.params = params;
    desc.customOperation = &UserDataOp;
    desc.reclaimMode = ReclaimMode::Custom;
    desc.reclaimCallback = &UserDataReclaim;
    desc.userData = &state;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    PhaseExecutor exec(desc, ctx);
    exec.Execute();

    EXPECT_EQ(state.opCalls, 5u);
    EXPECT_EQ(state.reclaimCalls, 1u);
    EXPECT_TRUE(state.opOk);
    EXPECT_TRUE(state.reclaimOk);
    gUserDataState = nullptr;
}
