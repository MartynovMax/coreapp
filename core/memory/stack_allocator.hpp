#pragma once

// =============================================================================
// stack_allocator.hpp
// LIFO (stack-style) allocator with per-allocation deallocation support.
//
// Manages a contiguous memory region, allocates by bumping a pointer forward,
// but supports LIFO frees (free must happen in reverse order of allocate).
//
// AllocationFlags:
//   - ZeroInitialize: Supported via memory_zero
//   - NoFail: Asserts in debug if out of memory; returns nullptr in release
//   - Other flags: Currently ignored
//
// Thread-safety: NOT thread-safe
//
// Deallocation:
//   - Deallocate() must be called in LIFO order (reverse of allocation)
//   - Out-of-order deallocation triggers assertion in debug builds
//   - Use RewindToMarker() to free multiple allocations at once
//
// Tracking integration:
//   - Hooks are dispatched by AllocateBytes/DeallocateBytes wrappers (core_allocator.hpp)
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

namespace detail {

struct StackAllocationHeader {
    memory_size total_size;  // Total size from block start to end of user data
    memory_size user_size;   // Size of user data only
    
#if CORE_MEMORY_DEBUG
    u32 magic;               // Magic number for validation (0xDEADBEEF)
    u32 padding_bytes;       // Reserved / future use
#endif
};

// Ensure header can be placed immediately before user ptr without breaking alignment
static_assert((sizeof(StackAllocationHeader) % alignof(StackAllocationHeader)) == 0,
              "StackAllocationHeader size must be a multiple of its alignment");

} // namespace detail

class StackAllocator final : public IAllocator {
public:
    StackAllocator(void* buffer, memory_size size) noexcept;
    StackAllocator(memory_size capacity, IAllocator& upstream) noexcept;
    
    ~StackAllocator() noexcept override;

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;

    bool Owns(const void* ptr) const noexcept override;

    struct Marker {
        u8* position;
        
        bool operator==(const Marker& other) const noexcept { return position == other.position; }
        bool operator!=(const Marker& other) const noexcept { return position != other.position; }
    };
    
    Marker GetMarker() const noexcept;
    void RewindToMarker(Marker marker) noexcept;
    void Reset() noexcept;

    // Introspection
    memory_size Used() const noexcept;
    memory_size Capacity() const noexcept;
    memory_size Remaining() const noexcept;

private:
    u8* _begin;
    u8* _current;
    u8* _end;
    IAllocator* _upstream;
};

} // namespace core

