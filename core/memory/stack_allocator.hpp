#pragma once

// =============================================================================
// stack_allocator.hpp
// LIFO (stack-style) allocator with per-allocation deallocation support.
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
    u32 padding_bytes;       // For alignment and future use
#endif
};

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

    // Stack marker for scope-based rewind
    struct Marker {
        u8* position;
        
        bool operator==(const Marker& other) const noexcept {
            return position == other.position;
        }
        bool operator!=(const Marker& other) const noexcept {
            return position != other.position;
        }
    };
    
    Marker GetMarker() const noexcept;
    void RewindToMarker(Marker marker) noexcept;
    void Reset() noexcept;

private:
    u8* _begin;
    u8* _current;
    u8* _end;
    IAllocator* _upstream;
};

} // namespace core

