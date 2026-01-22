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
#include "phase_context.hpp"

namespace core {
namespace bench {

// Forward declarations
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

// ----------------------------------------------------------------------------
// Callback signatures
// ----------------------------------------------------------------------------

// Callback for reclaim operations
using ReclaimCallback = void(*)(PhaseContext& ctx) noexcept;

// Callback for custom operations in phase (called each iteration)
using PhaseOperationCallback = void(*)(PhaseContext& ctx, u64 opIndex) noexcept;

// Callback for checking phase completion (returns true when phase should end)
using PhaseCompletionCallback = bool(*)(const PhaseContext& ctx) noexcept;

// ----------------------------------------------------------------------------
// PhaseDescriptor - Complete phase description
// ----------------------------------------------------------------------------

struct PhaseDescriptor {
    const char* name;
    PhaseType type;
    WorkloadParams params;
    
    // Reclaim configuration:
    ReclaimMode reclaimMode = ReclaimMode::None;
    ReclaimCallback reclaimCallback = nullptr;
    
    // Optional callbacks for customization:
    PhaseOperationCallback customOperation = nullptr;  // Called each iteration
    PhaseCompletionCallback completionCheck = nullptr; // Check for phase completion
    
    // User data for callbacks:
    void* userData = nullptr;
    
    // Metadata:
    const char* description = nullptr;
    bool measureMetrics = true;
};

} // namespace bench
} // namespace core
