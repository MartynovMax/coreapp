#pragma once

// =============================================================================
// phase_context.hpp
// Execution context for phase operations.
//
// Provides access to allocator, lifetime tracker, RNG, event sink,
// and runtime metrics during phase execution.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// Forward declarations
class IAllocator;
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
    
    // Runtime metrics:
    u64 currentOpIndex = 0;
    u64 allocCount = 0;
    u64 freeCount = 0;
    u64 bytesAllocated = 0;
    u64 bytesFreed = 0;
    
    // User data for callbacks:
    void* userData = nullptr;
};

} // namespace bench
} // namespace core
