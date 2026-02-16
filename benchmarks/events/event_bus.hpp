#pragma once

// =============================================================================
// event_bus.hpp
// Event bus for dispatching experiment events to sinks.
// =============================================================================

#include "event_sink.hpp"
#include "event_types.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// EventBus - Dispatches events to registered sinks
// ----------------------------------------------------------------------------

class EventBus {
public:
    EventBus() noexcept = default;
    ~EventBus() noexcept = default;

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // Attach event sink
    void Attach(IEventSink* sink) noexcept;

    // Emit event to all sinks
    void Emit(const Event& event) noexcept;

    // Check if any sinks attached
    bool HasSinks() const noexcept { return _count > 0; }

private:
    static constexpr u32 kMaxSinks = 8;
    
    IEventSink* _sinks[kMaxSinks];
    u32 _count = 0;
};

// ----------------------------------------------------------------------------
// EventBusSink - IEventSink wrapper that routes through EventBus
// ----------------------------------------------------------------------------

class EventBusSink : public IEventSink {
public:
    explicit EventBusSink(IEventSink* target) noexcept : _target(target) {
        if (_target) {
            _bus.Attach(_target);
        }
    }

    void OnEvent(const Event& event) noexcept override {
        _bus.Emit(event);
    }

private:
    EventBus _bus;
    IEventSink* _target;
};

} // namespace bench
} // namespace core
