#include "../events/event_bus.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace core;
using namespace core::bench;

// Mock event sink that records events
class MockEventSink : public IEventSink {
public:
    std::vector<Event> receivedEvents;

    void OnEvent(const Event& event) noexcept override {
        receivedEvents.push_back(event);
    }
};

// Test event sink attachment
TEST(EventBusTest, AttachSink) {
    EventBus bus;
    MockEventSink sink;

    EXPECT_FALSE(bus.HasSinks());

    bus.Attach(&sink);

    EXPECT_TRUE(bus.HasSinks());
}

// Test event emission
TEST(EventBusTest, EmitEvent) {
    EventBus bus;
    MockEventSink sink;

    bus.Attach(&sink);

    Event event;
    event.type = EventType::ExperimentBegin;
    event.experimentName = "test";
    event.repetitionId = 1;

    bus.Emit(event);

    ASSERT_EQ(sink.receivedEvents.size(), 1u);
    EXPECT_EQ(sink.receivedEvents[0].type, EventType::ExperimentBegin);
    EXPECT_STREQ(sink.receivedEvents[0].experimentName, "test");
    EXPECT_EQ(sink.receivedEvents[0].repetitionId, 1u);
}

// Test event order
TEST(EventBusTest, EventOrder) {
    EventBus bus;
    MockEventSink sink;

    bus.Attach(&sink);

    Event event1;
    event1.type = EventType::ExperimentBegin;
    event1.experimentName = "exp1";

    Event event2;
    event2.type = EventType::PhaseBegin;
    event2.experimentName = "exp1";
    event2.phaseName = "phase1";

    Event event3;
    event3.type = EventType::PhaseEnd;
    event3.experimentName = "exp1";
    event3.phaseName = "phase1";

    Event event4;
    event4.type = EventType::ExperimentEnd;
    event4.experimentName = "exp1";

    bus.Emit(event1);
    bus.Emit(event2);
    bus.Emit(event3);
    bus.Emit(event4);

    ASSERT_EQ(sink.receivedEvents.size(), 4u);
    EXPECT_EQ(sink.receivedEvents[0].type, EventType::ExperimentBegin);
    EXPECT_EQ(sink.receivedEvents[1].type, EventType::PhaseBegin);
    EXPECT_EQ(sink.receivedEvents[2].type, EventType::PhaseEnd);
    EXPECT_EQ(sink.receivedEvents[3].type, EventType::ExperimentEnd);
}

// Test no-op mode (no sinks attached)
TEST(EventBusTest, NoOpMode) {
    EventBus bus;

    EXPECT_FALSE(bus.HasSinks());

    Event event;
    event.type = EventType::ExperimentBegin;
    event.experimentName = "test";

    // Should not crash
    bus.Emit(event);

    EXPECT_FALSE(bus.HasSinks());
}

// Test multiple sinks
TEST(EventBusTest, MultipleSinks) {
    EventBus bus;
    MockEventSink sink1;
    MockEventSink sink2;

    bus.Attach(&sink1);
    bus.Attach(&sink2);

    Event event;
    event.type = EventType::ExperimentBegin;
    event.experimentName = "test";

    bus.Emit(event);

    ASSERT_EQ(sink1.receivedEvents.size(), 1u);
    ASSERT_EQ(sink2.receivedEvents.size(), 1u);
    EXPECT_EQ(sink1.receivedEvents[0].type, EventType::ExperimentBegin);
    EXPECT_EQ(sink2.receivedEvents[0].type, EventType::ExperimentBegin);
}
