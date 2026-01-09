#include "pool_allocator.hpp"
#include "memory_ops.hpp"

namespace core {

namespace {

void InitializePool(u8* begin, memory_size block_size, memory_size block_count, void*& out_free_list) noexcept {
    if (begin == nullptr || block_size == 0 || block_count == 0) {
        out_free_list = nullptr;
        return;
    }
    
    out_free_list = begin;
    u8* current = begin;
    
    for (memory_size i = 0; i < block_count - 1; ++i) {
        u8* next = current + block_size;
        detail::SetNextFreeBlock(current, next);
        current = next;
    }
    
    detail::SetNextFreeBlock(current, nullptr);
}

} // anonymous namespace

PoolAllocator::PoolAllocator(void* buffer, memory_size buffer_size, memory_size block_size) noexcept
    : _upstream(nullptr)
{
    if (buffer == nullptr || buffer_size == 0 || block_size == 0) {
        _begin = nullptr;
        _end = nullptr;
        _free_list = nullptr;
        _block_size = 0;
        _block_count = 0;
        return;
    }
    
    _block_size = detail::ComputeBlockSize(block_size, CORE_DEFAULT_ALIGNMENT);
    _block_count = buffer_size / _block_size;
    
    if (_block_count == 0) {
        _begin = nullptr;
        _end = nullptr;
        _free_list = nullptr;
        _block_size = 0;
        return;
    }
    
    _begin = static_cast<u8*>(buffer);
    _end = _begin + (_block_size * _block_count);
    
    InitializePool(_begin, _block_size, _block_count, _free_list);
}

PoolAllocator::PoolAllocator(memory_size block_size, memory_size block_count, IAllocator& upstream) noexcept
    : _upstream(&upstream)
{
    if (block_size == 0 || block_count == 0) {
        _begin = nullptr;
        _end = nullptr;
        _free_list = nullptr;
        _block_size = 0;
        _block_count = 0;
        return;
    }
    
    _block_size = detail::ComputeBlockSize(block_size, CORE_DEFAULT_ALIGNMENT);
    _block_count = block_count;
    
    const memory_size total_size = _block_size * _block_count;
    _begin = static_cast<u8*>(AllocateBytes(upstream, total_size));
    
    if (_begin == nullptr) {
        _end = nullptr;
        _free_list = nullptr;
        _block_size = 0;
        _block_count = 0;
        return;
    }
    
    _end = _begin + total_size;
    
    InitializePool(_begin, _block_size, _block_count, _free_list);
}

PoolAllocator::~PoolAllocator() noexcept {
    if (_upstream && _begin) {
        const memory_size total_size = _block_size * _block_count;
        DeallocateBytes(*_upstream, _begin, total_size);
    }
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

