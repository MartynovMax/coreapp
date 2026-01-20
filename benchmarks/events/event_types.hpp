#pragma once

// =============================================================================
// event_types.hpp
// Basic event type definitions for runner integration.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

enum class EventType : u8 {
    ExperimentBegin,
    ExperimentEnd,
    PhaseBegin,
    PhaseEnd,
};

struct Event {
    EventType type;
    const char* experimentName = nullptr;
    const char* phaseName = nullptr;
    u32 repetitionId = 0;
    u64 timestamp = 0;
};

} // namespace bench
} // namespace core
