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

} // namespace bench
} // namespace core
