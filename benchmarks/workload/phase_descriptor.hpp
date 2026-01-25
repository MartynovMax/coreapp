#pragma once

// =============================================================================
// phase_descriptor.hpp
// Phase description and callback-based reclaim model.
//
// Defines phase types, reclaim modes, and phase descriptors with callback
// support for flexible allocator-specific operations.
// =============================================================================

#include "workload_params.hpp"
#include "phase_context.hpp"
#include "phase_types.hpp"

namespace core::bench {

// Forward declarations
class PhaseExecutor;

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
    const char* name = nullptr;
    const char* experimentName = nullptr;
    PhaseType type = PhaseType::Steady;
    u32 repetitionId = 0;
    WorkloadParams params;
    ReclaimMode reclaimMode = ReclaimMode::None;
    ReclaimCallback reclaimCallback = nullptr;
    PhaseOperationCallback customOperation = nullptr;
    PhaseCompletionCallback completionCheck = nullptr;
    void* userData = nullptr;
};

} // namespace core::bench
