#pragma once

// =============================================================================
// allocator_capabilities.hpp
// Compile-time capability descriptors for benchmark allocators.
//
// Keeps the benchmark runner generic: reclaim strategy, workload compatibility,
// and lifetime restrictions are all derived from capabilities — not from
// hardcoded AllocatorType checks.
//
// Keep in sync with: benchmarks/analysis/allocator_capabilities.py
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "../experiments/alloc_bench_experiment.hpp"  // AllocatorType

namespace core::bench {

// ----------------------------------------------------------------------------
// AllocatorCaps — bitfield of allocator capabilities
// ----------------------------------------------------------------------------

enum class AllocatorCaps : u32 {
    None              = 0u,
    Allocate          = 1u << 0,  // supports per-object Allocate()
    Deallocate        = 1u << 1,  // supports per-object Deallocate()
    BulkReclaim       = 1u << 2,  // supports O(1) Reset() / destroy-all
    Rewind            = 1u << 3,  // supports marker/checkpoint rewind
    VariableSize      = 1u << 4,  // accepts arbitrary byte sizes
    FixedSizeOptimized= 1u << 5,  // tuned for a single object size
    ThreadLocal       = 1u << 6,  // assumes single-threaded ownership
};

inline constexpr AllocatorCaps operator|(AllocatorCaps a, AllocatorCaps b) noexcept {
    return static_cast<AllocatorCaps>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline constexpr AllocatorCaps operator&(AllocatorCaps a, AllocatorCaps b) noexcept {
    return static_cast<AllocatorCaps>(static_cast<u32>(a) & static_cast<u32>(b));
}
inline constexpr bool HasCap(AllocatorCaps flags, AllocatorCaps cap) noexcept {
    return (static_cast<u32>(flags) & static_cast<u32>(cap)) != 0u;
}

// ----------------------------------------------------------------------------
// Per-allocator capability lookup (constexpr)
// ----------------------------------------------------------------------------

inline constexpr AllocatorCaps GetAllocatorCaps(AllocatorType type) noexcept {
    using C = AllocatorCaps;
    switch (type) {
        case AllocatorType::Malloc:
            return C::Allocate | C::Deallocate | C::VariableSize;

        case AllocatorType::MonotonicArena:
            return C::Allocate | C::BulkReclaim | C::Rewind
                 | C::VariableSize | C::ThreadLocal;

        case AllocatorType::Pool:
            return C::Allocate | C::Deallocate | C::BulkReclaim
                 | C::FixedSizeOptimized;

        case AllocatorType::SegregatedList:
            // Handles multiple fixed size classes AND falls back for oversized requests,
            // so it effectively supports variable-size allocations.
            return C::Allocate | C::Deallocate | C::VariableSize | C::FixedSizeOptimized;
    }
    return C::None;
}

// ----------------------------------------------------------------------------
// Derived queries — replace scattered AllocatorType == checks
// ----------------------------------------------------------------------------

/// True when the allocator does NOT support per-object Deallocate().
/// BulkReclaim (arena Reset) must be used instead.
inline constexpr bool IsResetOnly(AllocatorType type) noexcept {
    return !HasCap(GetAllocatorCaps(type), AllocatorCaps::Deallocate);
}

/// True when the allocator supports per-object Deallocate().
inline constexpr bool SupportsDeallocate(AllocatorType type) noexcept {
    return HasCap(GetAllocatorCaps(type), AllocatorCaps::Deallocate);
}

/// True when the allocator supports variable-size allocations.
inline constexpr bool SupportsVariableSize(AllocatorType type) noexcept {
    return HasCap(GetAllocatorCaps(type), AllocatorCaps::VariableSize);
}

/// True when the allocator supports O(1) bulk reclaim (Reset / destroy-all).
inline constexpr bool SupportsBulkReclaim(AllocatorType type) noexcept {
    return HasCap(GetAllocatorCaps(type), AllocatorCaps::BulkReclaim);
}

// ----------------------------------------------------------------------------
// Scenario compatibility validation
// ----------------------------------------------------------------------------

/// Check whether an allocator + workload + lifetime combination is valid.
/// Returns nullptr on success, or a human-readable error string on failure.
inline const char* ValidateScenarioCompatibility(
    AllocatorType allocator,
    WorkloadProfile workload,
    LifetimeModel lifetime) noexcept
{
    const AllocatorCaps caps = GetAllocatorCaps(allocator);

    // Reset-only allocators only make sense with LongLived lifetime
    if (!HasCap(caps, AllocatorCaps::Deallocate)) {
        if (lifetime != LifetimeModel::LongLived) {
            return "Reset-only allocator requires LongLived lifetime "
                   "(per-object free is not supported)";
        }
    }

    // Fixed-size-only allocators cannot run variable_size workloads
    if (!HasCap(caps, AllocatorCaps::VariableSize)) {
        if (workload == WorkloadProfile::VariableSize ||
            workload == WorkloadProfile::Churn) {
            return "Allocator does not support variable-size allocations";
        }
    }

    return nullptr; // compatible
}

} // namespace core::bench

