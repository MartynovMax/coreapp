#pragma once

// =============================================================================
// bump_allocator.hpp
// Simple linear allocator that bumps a pointer forward for each allocation.
//
// Allocates by incrementing a pointer; does not support individual frees.
// Ideal for temporary/scoped memory: frame allocations, parsing, scratch buffers.
//
// AllocationFlags:
//   - ZeroInitialize: Supported via memory_zero
//   - NoFail: Asserts in debug if out of memory; returns nullptr in release
//   - Other flags: Currently ignored
//
// Thread-safety:
//   - NOT thread-safe
//   - Requires external synchronization or per-thread instances
//
// Deallocation:
//   - Deallocate() is a no-op
//   - Use Reset() to reclaim all memory at once
//
// Tracking integration:
//   - Hooks are dispatched via AllocateBytes/DeallocateBytes helpers (owning mode)
//   - Direct Allocate/Deallocate calls do not trigger hooks (by design)
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

class BumpAllocator final : public IAllocator {
public:
    BumpAllocator(void* buffer, memory_size size) noexcept;
    BumpAllocator(memory_size capacity, IAllocator& upstream) noexcept;
    
    ~BumpAllocator() noexcept override;

    BumpAllocator(const BumpAllocator&) = delete;
    BumpAllocator& operator=(const BumpAllocator&) = delete;

    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;

    bool Owns(const void* ptr) const noexcept override;
    
    void Reset() noexcept;
    
    memory_size Used() const noexcept;
    memory_size Capacity() const noexcept;
    memory_size Remaining() const noexcept;

private:
    u8* _begin;    // Start of the memory region
    u8* _current;  // Current allocation pointer (bumps forward)
    u8* _end;      // End of the memory region
    IAllocator* _upstream;
};

} // namespace core

