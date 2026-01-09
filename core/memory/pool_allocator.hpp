#pragma once

// =============================================================================
// pool_allocator.hpp
// Fixed-size block allocator that manages a contiguous pool using a free-list.
//
// Block layout (intrusive design):
//   - Free block: first sizeof(void*) bytes store next pointer; rest is unused
//   - Allocated block: all bytes are user data (including first sizeof(void*))
//   - Important: allocator may overwrite first sizeof(void*) bytes after free
//
// Constructor parameters:
//   - block_size: Maximum allocation size (stride); must be >= sizeof(void*)
//   - Actual block stride = max(block_size, sizeof(void*)), aligned to CORE_DEFAULT_ALIGNMENT
//
// AllocationFlags:
//   - ZeroInitialize: Zeroes requested size (not entire block)
//   - NoFail: Fatal error on out-of-memory (terminates program in debug and release)
//
// Thread-safety: NOT thread-safe
//
// Deallocation contract:
//   - Can be called in any order (non-LIFO)
//   - Passing invalid pointer is UB (checked only in debug via Owns/IsBlockAligned)
//   - Caller must ensure pointer was allocated by this pool
//
// Tracking:
//   - Hooks dispatched via AllocateBytes/DeallocateBytes for pool buffer (owning mode)
//   - Individual block allocations are NOT tracked by hooks (by design)
//   - This matches the pattern used by other allocators in the project
//
// Alignment:
//   - Non-owning mode: buffer is automatically aligned to CORE_DEFAULT_ALIGNMENT
//   - Block size is aligned to CORE_DEFAULT_ALIGNMENT
//   - Requests with alignment > CORE_DEFAULT_ALIGNMENT will fail
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
    
    const usize p_addr = reinterpret_cast<usize>(ptr);
    const usize base_addr = reinterpret_cast<usize>(pool_begin);
    
    if (p_addr < base_addr) {
        return false;
    }
    
    const memory_size offset = static_cast<memory_size>(p_addr - base_addr);
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
    memory_size _freeCount;
    IAllocator* _upstream;
};

} // namespace core

