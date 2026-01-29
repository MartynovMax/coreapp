#include "../workload/tick_manager.hpp"
#include "../workload/phase_context.hpp"
#include "../events/event_types.hpp"
#include "../events/event_sink.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

static PhaseContext MakePhaseContext(IEventSink* sink) {
    PhaseContext ctx{};
    ctx.eventSink = sink;
    ctx.phaseName = "phase";
    ctx.experimentName = "experiment";
    ctx.repetitionId = 1;
    return ctx;
}

static TickContext MakeTickContext(const u64 opIndex) {
    TickContext ctx{};
    ctx.opIndex = opIndex;
    ctx.allocCount = 10;
    ctx.freeCount = 5;
    ctx.bytesAllocated = 1000;
    ctx.bytesFreed = 400;
    ctx.peakLiveCount = 7;
    ctx.peakLiveBytes = 700;
    return ctx;
}

TEST(TickManagerTest, TickIntervalZeroNoTicks) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(0);
    for (u64 i = 0; i < 10; ++i) {
        TickContext tickCtx = MakeTickContext(i);
        mgr.OnOperation(tickCtx, phaseCtx);
    }
    EXPECT_TRUE(sink.events.empty());
}

TEST(TickManagerTest, TickEveryThousandOperations) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(1000);
    for (u64 i = 0; i < 3000; ++i) {
        TickContext tickCtx = MakeTickContext(i);
        mgr.OnOperation(tickCtx, phaseCtx);
    }
    ASSERT_EQ(sink.events.size(), 3u);
    EXPECT_EQ(sink.events[0].data.tick.opIndex, 999u);
    EXPECT_EQ(sink.events[1].data.tick.opIndex, 1999u);
    EXPECT_EQ(sink.events[2].data.tick.opIndex, 2999u);
}

TEST(TickManagerTest, TickEventContainsCorrectOpIndex) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(4);
    for (u64 i = 0; i < 8; ++i) {
        TickContext tickCtx = MakeTickContext(i);
        mgr.OnOperation(tickCtx, phaseCtx);
    }
    ASSERT_EQ(sink.events.size(), 2u);
    EXPECT_EQ(sink.events[0].data.tick.opIndex, 3u);
    EXPECT_EQ(sink.events[1].data.tick.opIndex, 7u);
}

TEST(TickManagerTest, TickEventContainsMetricsFromPhaseContext) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(2);
    TickContext tickCtx = MakeTickContext(1);
    tickCtx.allocCount = 11;
    tickCtx.freeCount = 9;
    tickCtx.bytesAllocated = 1234;
    tickCtx.bytesFreed = 567;
    tickCtx.peakLiveCount = 42;
    tickCtx.peakLiveBytes = 4242;
    mgr.OnOperation(tickCtx, phaseCtx);
    ASSERT_EQ(sink.events.size(), 1u);
    const auto& payload = sink.events[0].data.tick;
    EXPECT_EQ(payload.allocCount, 11u);
    EXPECT_EQ(payload.freeCount, 9u);
    EXPECT_EQ(payload.bytesAllocated, 1234u);
    EXPECT_EQ(payload.bytesFreed, 567u);
    EXPECT_EQ(payload.peakLiveCount, 42u);
    EXPECT_EQ(payload.peakLiveBytes, 4242u);
}

TEST(TickManagerTest, MultipleTicksLongRunningPhase) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(1000);
    for (u64 i = 0; i < 100000; ++i) {
        TickContext tickCtx = MakeTickContext(i);
        mgr.OnOperation(tickCtx, phaseCtx);
    }
    EXPECT_EQ(sink.events.size(), 100u);
}

TEST(TickManagerTest, NoTicksIfOperationCountBelowInterval) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(1000);
    for (u64 i = 0; i < 999; ++i) {
        TickContext tickCtx = MakeTickContext(i);
        mgr.OnOperation(tickCtx, phaseCtx);
    }
    EXPECT_TRUE(sink.events.empty());
}

TEST(TickManagerTest, FirstTickAtExactInterval) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(10);
    for (u64 i = 0; i < 10; ++i) {
        TickContext tickCtx = MakeTickContext(i);
        mgr.OnOperation(tickCtx, phaseCtx);
    }
    ASSERT_EQ(sink.events.size(), 1u);
    EXPECT_EQ(sink.events[0].data.tick.opIndex, 9u);
}

TEST(TickManagerTest, MockEventSinkCapturesAllTickEvents) {
    MockEventSink sink;
    const PhaseContext phaseCtx = MakePhaseContext(&sink);
    TickManager mgr(5);
    for (u64 i = 0; i < 20; ++i) {
        TickContext tickCtx = MakeTickContext(i);
        mgr.OnOperation(tickCtx, phaseCtx);
    }
    ASSERT_EQ(sink.events.size(), 4u);
    for (const auto& evt : sink.events) {
        EXPECT_EQ(evt.type, EventType::Tick);
    }
}
