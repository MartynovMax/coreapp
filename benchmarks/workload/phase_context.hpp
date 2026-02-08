#pragma once

// =============================================================================
// phase_context.hpp
// Execution context for phase operations.
//
// Provides access to allocator, lifetime tracker, RNG, event sink,
// and runtime metrics during phase execution.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "core/memory/core_allocator.hpp"
#include "phase_types.hpp"

namespace core::bench {
class LifetimeTracker;
class SeededRNG;
class IEventSink;

// ----------------------------------------------------------------------------
// PhaseContext - Execution context for phase operations
// ----------------------------------------------------------------------------

struct PhaseContext {
    // Core infrastructure
    IAllocator* allocator = nullptr;
    SeededRNG* callbackRng = nullptr;           // RNG for callbacks only (not workload)
    IEventSink* eventSink = nullptr;
    const char* phaseName = nullptr;
    const char* experimentName = nullptr;
    core::bench::PhaseType phaseType = core::bench::PhaseType::Steady;
    u32 repetitionId = 0;

    // Runtime metrics
    u64 currentOpIndex = 0;
    u64 allocCount = 0;
    u64 freeCount = 0;
    u64 bytesAllocated = 0;
    u64 bytesFreed = 0;
    u64 internalFreeCount = 0;
    u64 internalBytesFreed = 0;
    u64 reclaimFreeCount = 0;
    u64 reclaimBytesFreed = 0;
    u64 failedAllocCount = 0;

    // Live-set tracking (mutually exclusive)
    LifetimeTracker* liveSetTracker = nullptr;          // Primary source for live-set metrics
    LifetimeTracker* externalLifetimeTracker = nullptr; // Legacy support

    // User data for callbacks
    void* userData = nullptr;
};

} // namespace core::bench
