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
    
    // Helper methods for customOperation callbacks
    // These centralize metric updates to ensure consistency
    
    /// Record an allocation operation
    /// @param ptr The allocated pointer (nullptr indicates failure)
    /// @param size The size of the allocation
    /// @param alignment The alignment of the allocation (optional)
    void RecordAlloc(void* ptr, u32 size, memory_alignment alignment = 0) noexcept {
        if (ptr) {
            allocCount++;
            bytesAllocated += size;
        } else {
            failedAllocCount++;
        }
    }
    
    /// Record a free operation
    /// @param size The size of the freed memory
    void RecordFree(u32 size) noexcept {
        freeCount++;
        bytesFreed += size;
    }
};

} // namespace core::bench
