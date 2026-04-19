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
struct Operation;

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
using PhaseOperationCallback = void(*)(PhaseContext& ctx, const Operation& op) noexcept;

// Callback for checking phase completion (returns true when phase should end)
using PhaseCompletionCallback = bool(*)(const PhaseContext& ctx) noexcept;

// Callback for querying allocator footprint (reserved/capacity bytes) at phase end.
// Returns 0 if footprint is unavailable for this allocator.
using FootprintCallback = u64(*)(void* userData) noexcept;

// Callback for querying allocator-specific fallback allocation count at phase end.
// Returns 0 if not applicable for this allocator.
using FallbackCountCallback = u64(*)(void* userData) noexcept;

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
    FootprintCallback footprintCallback = nullptr;     // optional; queried at phase end
    FallbackCountCallback fallbackCountCallback = nullptr; // optional; queried at phase end

    u64 maxIterations = 100000;
    
    // Strict metrics validation for customOperation callbacks
    // When enabled, framework verifies that callbacks properly update metrics
    // Enable in tests to enforce correct metric handling
    bool strictMetricsValidation = false;
    
    void* userData = nullptr;
};

} // namespace core::bench
