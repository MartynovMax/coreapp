#pragma once

// =============================================================================
// core_memory.hpp
// Lightweight foundational header for Core memory subsystem.
//
// This header defines:
//   - Memory configuration macros
//   - Fundamental memory types (size, alignment, tag)
//   - Allocation request/info structures
//   - Base allocator interface
//   - Basic alignment utilities
//   - Memory hook declarations
//
// This header serves as the root of the memory subsystem API and must remain
// extremely lightweight and broadly includable.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_assert.hpp"
#include "../base/core_types.hpp"
#include "../base/core_inline.hpp"

namespace core {

// ----------------------------------------------------------------------------
// Memory configuration macros
// ----------------------------------------------------------------------------

#ifndef CORE_MEMORY_DEBUG
#define CORE_MEMORY_DEBUG CORE_DEBUG
#endif

#ifndef CORE_MEMORY_TRACKING
#define CORE_MEMORY_TRACKING 0
#endif

#ifndef CORE_DEFAULT_ALIGNMENT
#define CORE_DEFAULT_ALIGNMENT 16u
#endif

#ifndef CORE_CACHE_LINE_SIZE
#define CORE_CACHE_LINE_SIZE 64u
#endif

#ifndef CORE_MEM_ASSERT
#define CORE_MEM_ASSERT(expr) ASSERT(expr)
#endif

// ----------------------------------------------------------------------------
// Memory types
// ----------------------------------------------------------------------------

using memory_size = usize;
using memory_alignment = u32;
using memory_tag = u32;

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

} // namespace core

