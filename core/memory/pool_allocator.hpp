#pragma once

// =============================================================================
// pool_allocator.hpp
// Fixed-size block allocator that manages a contiguous pool using a free-list.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

namespace detail {

constexpr memory_size kMinBlockSize = sizeof(void*);

CORE_FORCE_INLINE void* GetNextFreeBlock(void* block) noexcept {
    return *static_cast<void**>(block);
}

CORE_FORCE_INLINE void SetNextFreeBlock(void* block, void* next) noexcept {
    *static_cast<void**>(block) = next;
}

CORE_FORCE_INLINE memory_size ComputeBlockSize(memory_size requested_size, memory_alignment alignment) noexcept {
    memory_size size = (requested_size < kMinBlockSize) ? kMinBlockSize : requested_size;
    size = AlignUp(size, alignment);
    return size;
}

CORE_FORCE_INLINE bool IsBlockAligned(const void* ptr, const void* pool_begin, memory_size block_size) noexcept {
    if (block_size == 0) {
        return false;
    }
    const u8* p = static_cast<const u8*>(ptr);
    const u8* base = static_cast<const u8*>(pool_begin);
    const memory_size offset = static_cast<memory_size>(p - base);
    return (offset % block_size) == 0;
}

} // namespace detail

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
    u8* _begin;
    u8* _end;
    void* _freeList;
    memory_size _blockSize;
    memory_size _blockCount;
    IAllocator* _upstream;
};

} // namespace core

