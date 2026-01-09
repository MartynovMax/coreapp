#pragma once

// =============================================================================
// pool_allocator.hpp
// Fixed-size block allocator that manages a contiguous pool using a free-list.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

class PoolAllocator final : public IAllocator {
public:
    PoolAllocator(void* buffer, memory_size buffer_size, memory_size block_size) noexcept;
    
    PoolAllocator(memory_size block_size, memory_size block_count, IAllocator& upstream) noexcept;
    
    ~PoolAllocator() noexcept override;

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    // IAllocator interface
    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;
    bool Owns(const void* ptr) const noexcept override;

    // Pool introspection
    memory_size BlockSize() const noexcept;
    memory_size BlockCount() const noexcept;
    memory_size FreeBlocks() const noexcept;
    memory_size UsedBlocks() const noexcept;
    memory_size CapacityBytes() const noexcept;

private:
    u8* _begin;                  // Start of the pool memory
    u8* _end;                    // End of the pool memory
    void* _free_list;            // Head of the free-block singly linked list
    memory_size _block_size;     // Aligned size of each block
    memory_size _block_count;    // Total number of blocks in the pool
    IAllocator* _upstream;       // Upstream allocator (owning mode) or nullptr
};

} // namespace core

