#pragma once

// =============================================================================
// phase_descriptor.hpp
// Phase description and callback-based reclaim model.
//
// Defines phase types, reclaim modes, and phase descriptors with callback
// support for flexible allocator-specific operations.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "workload_params.hpp"

namespace core {
namespace bench {

// Forward declarations
struct PhaseContext;
class PhaseExecutor;

// ----------------------------------------------------------------------------
// PhaseType - Types of workload phases
// ----------------------------------------------------------------------------

enum class PhaseType {
    RampUp,             // Accumulation phase (typically alloc-only)
    Steady,             // Steady-state phase (bounded live-set, continuous churn)
    Drain,              // Gradual release phase
    BulkReclaim,        // Mass reclaim phase (reset/free-all)
    Evolution,          // Long-running evolution with periodic ticks
    Custom,             // Custom phase with user-defined behavior
};

// ----------------------------------------------------------------------------
// ReclaimMode - Memory reclaim strategies
// ----------------------------------------------------------------------------

enum class ReclaimMode {
    None,               // No reclaim operation
    FreeAll,            // Free all tracked objects via LifetimeTracker
    Custom,             // Custom reclaim via callback
};

} // namespace bench
} // namespace core
