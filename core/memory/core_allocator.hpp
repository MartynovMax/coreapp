#pragma once

// =============================================================================
// core_allocator.hpp
// Base allocator interface and allocation helpers.
//
// Provides:
//   - IAllocator interface
//   - AllocationRequest / AllocationInfo structures
//   - Typed helpers: AllocateObject, AllocateArray, DeallocateObject, DeallocateArray
//   - Allocator adapters: AllocatorRef, TypedAllocator<T>
//   - Default allocator accessors: DefaultAllocator(), SystemAllocator()
//   - Allocation hooks for instrumentation
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_assert.hpp"
#include "../base/core_types.hpp"
#include "../base/core_inline.hpp"

#include "core_memory.hpp"
#include "memory_traits.hpp"

namespace core {

// ----------------------------------------------------------------------------
// Allocation flags and structures
// ----------------------------------------------------------------------------

enum class AllocationFlags : u32 {
    None = 0u,
    Transient = 1u << 0,
    Persistent = 1u << 1,
    ZeroInitialize = 1u << 2,
    NoFail = 1u << 3,
};

CORE_FORCE_INLINE constexpr AllocationFlags operator|(AllocationFlags a, AllocationFlags b) noexcept {
    return static_cast<AllocationFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

CORE_FORCE_INLINE constexpr AllocationFlags operator&(AllocationFlags a, AllocationFlags b) noexcept {
    return static_cast<AllocationFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

CORE_FORCE_INLINE constexpr bool Any(AllocationFlags f) noexcept {
    return static_cast<u32>(f) != 0u;
}

struct AllocationRequest {
    memory_size size = 0;
    memory_alignment alignment = CORE_DEFAULT_ALIGNMENT;
    memory_tag tag = 0;
    AllocationFlags flags = AllocationFlags::None;
};

struct AllocationInfo {
    void* ptr = nullptr;
    memory_size size = 0;
    memory_alignment alignment = CORE_DEFAULT_ALIGNMENT;
    memory_tag tag = 0;
};

struct AllocatorStats {
    memory_size bytes_allocated_current = 0;
    memory_size bytes_allocated_peak = 0;
    memory_size bytes_allocated_total = 0;
    memory_size alloc_count_total = 0;
    memory_size free_count_total = 0;
};

// ----------------------------------------------------------------------------
// Base allocator interface
// ----------------------------------------------------------------------------
//
// Contract:
//   - Allocate() must return memory aligned to request.alignment (or default)
//   - Allocate() returns nullptr on failure (unless NoFail flag is set)
//   - Deallocate() must accept nullptr (no-op)
//   - Deallocate() info must match original Allocate() request
//   - tag is optional metadata for tracking/debugging (0 is valid)
//   - Thread-safety is allocator-specific (document in derived classes)

class IAllocator {
public:
    virtual ~IAllocator() = default;

    virtual void* Allocate(const AllocationRequest& request) noexcept = 0;
    virtual void Deallocate(const AllocationInfo& info) noexcept = 0;

    virtual bool TryGetStats(AllocatorStats& out_stats) const noexcept {
        CORE_UNUSED(out_stats);
        return false;
    }
};

// ----------------------------------------------------------------------------
// Memory hook declarations
// ----------------------------------------------------------------------------

using AllocationHookFn = void (*)(const AllocationRequest& req,
                                   const AllocationInfo& info,
                                   void* user) noexcept;

using DeallocationHookFn = void (*)(const AllocationInfo& info,
                                     void* user) noexcept;

bool RegisterAllocationHook(AllocationHookFn hook, void* user) noexcept;
bool UnregisterAllocationHook(AllocationHookFn hook, void* user) noexcept;

bool RegisterDeallocationHook(DeallocationHookFn hook, void* user) noexcept;
bool UnregisterDeallocationHook(DeallocationHookFn hook, void* user) noexcept;

} // namespace core

