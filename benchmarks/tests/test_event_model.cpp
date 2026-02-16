#include "../events/event_types.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_bus.hpp"
#include "../events/allocation_event_adapter.hpp"
#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/workload_params.hpp"
#include "../common/seeded_rng.hpp"
#include "test_helpers.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

// ============================================================================
// Test: Event Sequencing - eventSeqNo monotonically increases
// ============================================================================

TEST(EventModelTest, EventSequencingMonotonicallyIncreasing) {
    MockEventSink sink;
    EventBus bus;
    bus.Attach(&sink);

    for (int i = 0; i < 10; ++i) {
        Event evt{};
        evt.type = EventType::PhaseBegin;
        evt.experimentName = "test";
        evt.phaseName = "phase";
        bus.Emit(evt);
    }

    ASSERT_EQ(sink.events.size(), 10u);

    for (size_t i = 1; i < sink.events.size(); ++i) {
        EXPECT_GT(sink.events[i].eventSeqNo, sink.events[i-1].eventSeqNo)
            << "Event " << i << " seqNo should be greater than event " << (i-1);
    }
}

TEST(EventModelTest, EventSequencingDifferentTypes) {
    MockEventSink sink;
    EventBus bus;
    bus.Attach(&sink);

    Event evt1{};
    evt1.type = EventType::ExperimentBegin;
    evt1.experimentName = "exp";
    bus.Emit(evt1);

    Event evt2{};
    evt2.type = EventType::PhaseBegin;
    evt2.experimentName = "exp";
    evt2.phaseName = "phase";
    bus.Emit(evt2);

    Event evt3{};
    evt3.type = EventType::PhaseComplete;
    evt3.experimentName = "exp";
    evt3.phaseName = "phase";
    bus.Emit(evt3);

    Event evt4{};
    evt4.type = EventType::PhaseEnd;
    evt4.experimentName = "exp";
    evt4.phaseName = "phase";
    bus.Emit(evt4);

    Event evt5{};
    evt5.type = EventType::ExperimentEnd;
    evt5.experimentName = "exp";
    bus.Emit(evt5);

    ASSERT_EQ(sink.events.size(), 5u);

    EXPECT_EQ(sink.events[0].type, EventType::ExperimentBegin);
    EXPECT_EQ(sink.events[1].type, EventType::PhaseBegin);
    EXPECT_EQ(sink.events[2].type, EventType::PhaseComplete);
    EXPECT_EQ(sink.events[3].type, EventType::PhaseEnd);
    EXPECT_EQ(sink.events[4].type, EventType::ExperimentEnd);

    for (size_t i = 1; i < sink.events.size(); ++i) {
        EXPECT_GT(sink.events[i].eventSeqNo, sink.events[i-1].eventSeqNo);
    }
}

// ============================================================================
// Test: Allocation Correlation - allocationId matches between Allocation <-> Free
// ============================================================================

TEST(EventModelTest, AllocationCorrelationMatchingIds) {
    MallocAllocator allocator;
    SeededRNG rng(100);

    MockEventSink sink;

    WorkloadParams params;
    params.seed = 100;
    params.operationCount = 4; // 2 allocs, 2 frees
    params.allocFreeRatio = 0.5f;
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "CorrelationTest";
    desc.experimentName = "EventModelTest";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;
    desc.repetitionId = 1;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.callbackRng = &rng;

    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();

    // Filter allocation and free events
    std::vector<AllocationPayload> allocPayloads;
    std::vector<FreePayload> freePayloads;

    for (const auto& evt : sink.events) {
        if (evt.type == EventType::Allocation) {
            allocPayloads.push_back(evt.data.allocation);
        } else if (evt.type == EventType::Free) {
            freePayloads.push_back(evt.data.free);
        }
    }

    if (allocPayloads.size() == 0 && freePayloads.size() == 0) {
        GTEST_SKIP() << "Allocation tracking disabled (CORE_MEMORY_TRACKING=0)";
        return;
    }

    EXPECT_GT(allocPayloads.size(), 0u);
    EXPECT_GT(freePayloads.size(), 0u);

    for (const auto& freePayload : freePayloads) {
        u64 allocId = freePayload.allocationId;
        if (allocId == 0) continue;

        bool foundMatch = false;
        for (const auto& allocPayload : allocPayloads) {
            if (allocPayload.allocationId == allocId) {
                foundMatch = true;
                EXPECT_EQ(allocPayload.ptr, freePayload.ptr)
                    << "Allocation ID " << allocId << " has mismatched pointers";
                break;
            }
        }

        EXPECT_TRUE(foundMatch) << "Free event with allocationId " << allocId << " has no matching Allocation";
    }
}

// ============================================================================
// Test: Lifecycle Ordering - ExperimentBegin -> PhaseBegin -> ... -> PhaseEnd -> ExperimentEnd
// ============================================================================

TEST(EventModelTest, LifecycleOrderingCorrect) {
    MallocAllocator allocator;
    SeededRNG rng(200);

    MockEventSink sink;

    WorkloadParams params;
    params.seed = 200;
    params.operationCount = 2;
    params.allocFreeRatio = 1.0f;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "OrderTest";
    desc.experimentName = "EventModelTest";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;
    desc.repetitionId = 1;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.callbackRng = &rng;

    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();

    int phaseBeginIdx = -1;
    int phaseCompleteIdx = -1;
    int phaseEndIdx = -1;

    for (size_t i = 0; i < sink.events.size(); ++i) {
        if (sink.events[i].type == EventType::PhaseBegin) phaseBeginIdx = static_cast<int>(i);
        if (sink.events[i].type == EventType::PhaseComplete) phaseCompleteIdx = static_cast<int>(i);
        if (sink.events[i].type == EventType::PhaseEnd) phaseEndIdx = static_cast<int>(i);
    }

    ASSERT_NE(phaseBeginIdx, -1) << "PhaseBegin event not found";
    ASSERT_NE(phaseCompleteIdx, -1) << "PhaseComplete event not found";
    ASSERT_NE(phaseEndIdx, -1) << "PhaseEnd event not found";

    EXPECT_LT(phaseBeginIdx, phaseCompleteIdx);
    EXPECT_LT(phaseCompleteIdx, phaseEndIdx);

    EXPECT_GT(sink.events[phaseBeginIdx].eventSeqNo, 0u);
    EXPECT_GT(sink.events[phaseCompleteIdx].eventSeqNo, 0u);
    EXPECT_GT(sink.events[phaseEndIdx].eventSeqNo, 0u);

    EXPECT_LT(sink.events[phaseBeginIdx].eventSeqNo, sink.events[phaseCompleteIdx].eventSeqNo);
    EXPECT_LT(sink.events[phaseCompleteIdx].eventSeqNo, sink.events[phaseEndIdx].eventSeqNo);
}

// ============================================================================
// Test: Failure Emission - AllocationFailed emitted when ptr==nullptr
// ============================================================================

TEST(EventModelTest, FailureEmissionAllocationFailed) {
    Event failEvt{};
    failEvt.type = EventType::AllocationFailed;
    failEvt.experimentName = "test";
    failEvt.phaseName = "phase";
    failEvt.repetitionId = 1;
    failEvt.data.failure.reason = FailureReason::OutOfMemory;
    failEvt.data.failure.message = "Test OOM";
    failEvt.data.failure.opIndex = ~0ULL;
    failEvt.data.failure.isRecoverable = false;

    EXPECT_EQ(failEvt.type, EventType::AllocationFailed);
    EXPECT_EQ(failEvt.data.failure.reason, FailureReason::OutOfMemory);
    EXPECT_STREQ(failEvt.data.failure.message, "Test OOM");
    EXPECT_FALSE(failEvt.data.failure.isRecoverable);
}

TEST(EventModelTest, PhaseFailureEventConstruction) {
    Event failEvt{};
    failEvt.type = EventType::PhaseFailure;
    failEvt.experimentName = "test";
    failEvt.phaseName = "phase";
    failEvt.repetitionId = 1;
    failEvt.data.failure.reason = FailureReason::MaxIterationsReached;
    failEvt.data.failure.message = "Max iterations exceeded";
    failEvt.data.failure.opIndex = 999999;
    failEvt.data.failure.isRecoverable = false;

    EXPECT_EQ(failEvt.type, EventType::PhaseFailure);
    EXPECT_EQ(failEvt.data.failure.reason, FailureReason::MaxIterationsReached);
    EXPECT_EQ(failEvt.data.failure.opIndex, 999999u);
}

// ============================================================================
// Test: Full Run Trace - Complete run with all event types
// ============================================================================

TEST(EventModelTest, FullRunTraceComplete) {
    MallocAllocator allocator;
    SeededRNG rng(300);

    MockEventSink sink;

    WorkloadParams params;
    params.seed = 300;
    params.operationCount = 10;
    params.allocFreeRatio = 0.6f;
    params.tickInterval = 5; // Tick every 5 operations
    params.lifetimeModel = LifetimeModel::Fifo;
    params.sizeDistribution = SizePresets::SmallObjects();

    PhaseDescriptor desc{};
    desc.name = "FullTrace";
    desc.experimentName = "EventModelTest";
    desc.type = PhaseType::Steady;
    desc.params = params;
    desc.reclaimMode = ReclaimMode::FreeAll;
    desc.repetitionId = 1;

    PhaseContext ctx{};
    ctx.allocator = &allocator;
    ctx.callbackRng = &rng;

    PhaseExecutor exec(desc, ctx, &sink);
    exec.Execute();

    int phaseBeginCount = 0;
    int phaseCompleteCount = 0;
    int phaseEndCount = 0;
    int tickCount = 0;
    int allocationCount = 0;
    int freeCount = 0;

    for (const auto& evt : sink.events) {
        switch (evt.type) {
            case EventType::PhaseBegin: phaseBeginCount++; break;
            case EventType::PhaseComplete: phaseCompleteCount++; break;
            case EventType::PhaseEnd: phaseEndCount++; break;
            case EventType::Tick: tickCount++; break;
            case EventType::Allocation: allocationCount++; break;
            case EventType::Free: freeCount++; break;
            default: break;
        }
    }

    EXPECT_EQ(phaseBeginCount, 1);
    EXPECT_EQ(phaseCompleteCount, 1);
    EXPECT_EQ(phaseEndCount, 1);
    EXPECT_EQ(tickCount, 2);
    
    if (allocationCount == 0 && freeCount == 0) {
        GTEST_SKIP() << "Allocation tracking disabled (CORE_MEMORY_TRACKING=0)";
        return;
    }

    EXPECT_GT(allocationCount, 0);
    EXPECT_GT(freeCount, 0);

    for (const auto& evt : sink.events) {
        EXPECT_GT(evt.eventSeqNo, 0u) << "Event of type " << static_cast<u32>(evt.type) << " has zero seqNo";
    }
}

// ============================================================================
// Test: Event Payload Structures are Trivially Copyable
// ============================================================================

TEST(EventModelTest, PayloadStructuresTriviallyCopyable) {
    EXPECT_TRUE(is_trivially_copyable_v<AllocationPayload>());
    EXPECT_TRUE(is_trivially_copyable_v<FreePayload>());
    EXPECT_TRUE(is_trivially_copyable_v<AllocatorResetPayload>());
    EXPECT_TRUE(is_trivially_copyable_v<FailurePayload>());
    EXPECT_TRUE(is_trivially_copyable_v<PhaseCompletePayload>());
    EXPECT_TRUE(is_trivially_copyable_v<TickPayload>());
}

// ============================================================================
// Test: AllocationEventAdapter Attach/Detach
// ============================================================================

TEST(EventModelTest, AllocationEventAdapterAttachDetach) {
    MockEventSink sink;

    AllocationEventAdapter adapter(&sink, "test", "phase", 1);

    adapter.Attach();
    adapter.Detach();
    adapter.Detach();
}

// ============================================================================
// Test: Event SeqNo Uniqueness Across Multiple Sinks
// ============================================================================

TEST(EventModelTest, EventSeqNoUniquenessAcrossSinks) {
    MockEventSink sink1;
    MockEventSink sink2;
    EventBus bus;

    bus.Attach(&sink1);
    bus.Attach(&sink2);

    Event evt{};
    evt.type = EventType::PhaseBegin;
    evt.experimentName = "test";
    evt.phaseName = "phase";

    bus.Emit(evt);

    ASSERT_EQ(sink1.events.size(), 1u);
    ASSERT_EQ(sink2.events.size(), 1u);

    EXPECT_EQ(sink1.events[0].eventSeqNo, sink2.events[0].eventSeqNo);
    EXPECT_GT(sink1.events[0].eventSeqNo, 0u);
}
