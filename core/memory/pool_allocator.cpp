#include "pool_allocator.hpp"
#include "memory_ops.hpp"

namespace core {

PoolAllocator::PoolAllocator(void* buffer, memory_size buffer_size, memory_size block_size) noexcept
    : _upstream(nullptr)
{
    // TODO: Implement in step 3 (initialization routine)
    _begin = nullptr;
    _end = nullptr;
    _free_list = nullptr;
    _block_size = 0;
    _block_count = 0;
}

PoolAllocator::PoolAllocator(memory_size block_size, memory_size block_count, IAllocator& upstream) noexcept
    : _upstream(&upstream)
{
    _begin = nullptr;
    _end = nullptr;
    _free_list = nullptr;
    _block_size = 0;
    _block_count = 0;
}

PoolAllocator::~PoolAllocator() noexcept {
}

void* PoolAllocator::Allocate(const AllocationRequest& request) noexcept {
    CORE_UNUSED(request);
    return nullptr;
}

void PoolAllocator::Deallocate(const AllocationInfo& info) noexcept {
    CORE_UNUSED(info);
}

bool PoolAllocator::Owns(const void* ptr) const noexcept {
    CORE_UNUSED(ptr);
    return false;
}

memory_size PoolAllocator::BlockSize() const noexcept {
    return 0;
}

memory_size PoolAllocator::BlockCount() const noexcept {
    return 0;
}

memory_size PoolAllocator::FreeBlocks() const noexcept {
    return 0;
}

memory_size PoolAllocator::UsedBlocks() const noexcept {
    return 0;
}

memory_size PoolAllocator::CapacityBytes() const noexcept {
    return 0;
}

} // namespace core

