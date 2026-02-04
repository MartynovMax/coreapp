#include "../events/event_types.hpp"
#include "../events/event_sink.hpp"
#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/workload_params.hpp"
#include "../common/seeded_rng.hpp"
#include "test_helpers.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <type_traits>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

TEST(EventTypesTest, PhaseCompletePayloadDesignatedInitializers) {
    PhaseCompletePayload payload{
        .experimentName = "exp",
        .phaseName = "phase",
        .phaseType = PhaseType::Steady,
        .repetitionId = 3,
        .startTimestamp = 10,
        .endTimestamp = 30,
        .durationNs = 20,
        .allocCount = 7,
        .freeCount = 5,
        .totalOperations = 12,
        .bytesAllocated = 1024,
        .bytesFreed = 512,
        .peakLiveCount = 9,
        .peakLiveBytes = 2048,
        .preReclaimLiveCount = 3,
        .preReclaimLiveBytes = 384,
        .finalLiveCount = 2,
        .finalLiveBytes = 256,
        .internalFreeCount = 1,
        .internalBytesFreed = 64,
        .reclaimFreeCount = 2,
        .reclaimBytesFreed = 128,
        .totalFreeCount = 8,
        .totalBytesFreed = 704,
        .opsPerSec = 1.5,
        .throughput = 2.5,
    };

    Event evt{};
    evt.type = EventType::PhaseComplete;
    evt.experimentName = "exp";
    evt.phaseName = "phase";
    evt.repetitionId = 3;
    evt.data.phaseComplete = payload;

    EXPECT_EQ(evt.data.phaseComplete.allocCount, 7u);
    EXPECT_EQ(evt.data.phaseComplete.totalOperations, 12u);
    EXPECT_EQ(evt.data.phaseComplete.peakLiveBytes, 2048u);
    EXPECT_EQ(evt.data.phaseComplete.finalLiveBytes, 256u);
    EXPECT_EQ(evt.data.phaseComplete.phaseType, PhaseType::Steady);
}

TEST(EventTypesTest, PhaseCompleteEventSizeAndAlignment) {
    EXPECT_GE(alignof(PhaseCompletePayload), alignof(u64));
    EXPECT_GE(alignof(Event), alignof(PhaseCompletePayload));
    EXPECT_GE(sizeof(Event), sizeof(PhaseCompletePayload));
}

struct IEventSinkExtension : public IEventSink {
    void OnEvent(const Event& event) noexcept override {
        if (base) {
            base->OnEvent(event);
        }
    }
    void OnEventEx(const Event& event) noexcept {
        if (base) {
            base->OnEvent(event);
        }
    }
    IEventSink* base = nullptr;
};

TEST(EventTypesTest, IEventSinkExtensionCompilesWithExistingImplementations) {
    MockEventSink sink;
    IEventSinkExtension ext;
    ext.base = &sink;
    Event evt{};
    evt.type = EventType::PhaseBegin;
    ext.OnEvent(evt);
    ext.OnEventEx(evt);
    EXPECT_EQ(sink.events.size(), 2u);
}

TEST(EventTypesTest, MockEventSinkCapturesPhaseEvents) {
    MallocAllocator allocator;
    SeededRNG rng(1);
    WorkloadParams params;
    params.seed = 1;
    params.operationCount = 2;
    params.allocFreeRatio = 1.0f;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "phase";
    desc.experimentName = "exp";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    MockEventSink sink;
    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();

    bool hasBegin = false;
    bool hasComplete = false;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::PhaseBegin) hasBegin = true;
        if (evt.type == EventType::PhaseComplete) hasComplete = true;
    }
    EXPECT_TRUE(hasBegin);
    EXPECT_TRUE(hasComplete);
}

TEST(EventTypesTest, MockEventSinkCapturesTickEvents) {
    MallocAllocator allocator;
    SeededRNG rng(2);
    WorkloadParams params;
    params.seed = 2;
    params.operationCount = 6;
    params.allocFreeRatio = 1.0f;
    params.tickInterval = 2;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "phase";
    desc.experimentName = "exp";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    MockEventSink sink;
    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();

    u32 tickCount = 0;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::Tick) tickCount++;
    }
    EXPECT_EQ(tickCount, 3u);
}

TEST(EventTypesTest, EventOrderingPhaseBeginTickPhaseComplete) {
    MallocAllocator allocator;
    SeededRNG rng(3);
    WorkloadParams params;
    params.seed = 3;
    params.operationCount = 5;
    params.allocFreeRatio = 1.0f;
    params.tickInterval = 2;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "phase";
    desc.experimentName = "exp";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.rng = &rng;

    MockEventSink sink;
    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();

    ASSERT_GE(sink.events.size(), 3u);
    EXPECT_EQ(sink.events.front().type, EventType::PhaseBegin);
    EXPECT_EQ(sink.events[sink.events.size() - 2].type, EventType::PhaseComplete);
    EXPECT_EQ(sink.events.back().type, EventType::PhaseEnd);

    bool seenTick = false;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::Tick) {
            seenTick = true;
            break;
        }
    }
    EXPECT_TRUE(seenTick);
}
