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
    // Core infrastructure:
    IAllocator* allocator = nullptr;
    LifetimeTracker* lifetimeTracker = nullptr;
    SeededRNG* rng = nullptr;
    IEventSink* eventSink = nullptr;
    const char* phaseName = nullptr;
    const char* experimentName = nullptr;
    core::bench::PhaseType phaseType = core::bench::PhaseType::Steady;
    u32 repetitionId = 0;

    u64 currentOpIndex = 0;        // Current operation index in phase
    u64 allocCount = 0;             // Number of allocations performed
    u64 freeCount = 0;              // Number of explicit frees performed
    u64 bytesAllocated = 0;         // Total bytes allocated
    u64 bytesFreed = 0;             // Total bytes freed (explicit)
    u64 internalFreeCount = 0;      // Internal cleanup (e.g., tracker overflow)
    u64 internalBytesFreed = 0;     // Bytes freed internally
    u64 reclaimFreeCount = 0;       // Frees during reclaim phase
    u64 reclaimBytesFreed = 0;      // Bytes freed during reclaim
    u64 failedAllocCount = 0;       // Failed allocation attempts

    // User data for callbacks (mutable):
    void* userData = nullptr;


    LifetimeTracker* externalLifetimeTracker = nullptr;
};

} // namespace core::bench
