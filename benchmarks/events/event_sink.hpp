#pragma once

// =============================================================================
// event_sink.hpp
// Integration point for event instrumentation.
//
// Note: This is a minimal interface stub for runner integration.
// Full event types and bus implementation are in separate task.
// =============================================================================

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// IEventSink - Observer interface for experiment events
// ----------------------------------------------------------------------------

class IEventSink {
public:
    virtual ~IEventSink() = default;
};

} // namespace bench
} // namespace core
