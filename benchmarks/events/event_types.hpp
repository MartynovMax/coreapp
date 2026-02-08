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
#include "core/memory/memory_ops.hpp"
#include "core/memory/memory_traits.hpp"

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
    const char* experimentName;
    const char* phaseName;
    u32 repetitionId;
    u64 timestamp;
    union {
        PhaseCompletePayload phaseComplete;
        TickPayload tick;
    } data;
    Event()
        : type(EventType::PhaseBegin),
          experimentName(nullptr),
          phaseName(nullptr),
          repetitionId(0),
          timestamp(0)
    {
        // Ensure the union contains only trivial types to allow safe zero-initialization
        static_assert(core::is_trivially_copyable_v<PhaseCompletePayload>(), 
                      "PhaseCompletePayload must be trivially copyable for union safety");
        static_assert(core::is_trivially_copyable_v<TickPayload>(), 
                      "TickPayload must be trivially copyable for union safety");
        static_assert(core::is_trivially_copyable_v<decltype(data)>(), 
                      "Event::data union must be trivially copyable");
        
        core::memory_zero(&data, sizeof(data));
    }
};

} // namespace bench
} // namespace core
