#pragma once

// =============================================================================
// bump_allocator.hpp
// Simple linear allocator that bumps a pointer forward for each allocation.
//
// Allocates by incrementing a pointer; does not support individual frees.
// Ideal for temporary/scoped memory: frame allocations, parsing, scratch buffers.
//
// AllocationFlags:
//   - ZeroInitialize: Supported via memset after allocation
//   - Other flags: Currently ignored
//
// Thread-safety:
//   - NOT thread-safe
//   - Requires external synchronization or per-thread instances
//
// Deallocation:
//   - Deallocate() is a no-op
//   - Use reset() to reclaim all memory at once
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

class BumpAllocator final : public IAllocator {
public:
    ~BumpAllocator() noexcept override = default;

    BumpAllocator(const BumpAllocator&) = delete;
    BumpAllocator& operator=(const BumpAllocator&) = delete;

    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;
};

} // namespace core

