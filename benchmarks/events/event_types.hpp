#pragma once

// =============================================================================
// event_types.hpp
// Basic event type definitions for runner integration.
//
// Note: PhaseBegin/PhaseEnd are emitted by experiments (workload level),
//       ExperimentBegin/End are emitted by runner.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "workload/phase_descriptor.hpp"
#include "event_payloads.hpp"

namespace core {
namespace bench {

enum class EventType : u32 {
    ExperimentBegin,
    ExperimentEnd,
    PhaseBegin,
    PhaseEnd,
    PhaseComplete,
    Tick,
};

struct Event {
    EventType type;
    const char* experimentName = nullptr;
    const char* phaseName = nullptr;
    u32 repetitionId = 0;
    u64 timestamp = 0;
    union {
        PhaseCompletePayload phaseComplete;
    } data;
};

} // namespace bench
} // namespace core
