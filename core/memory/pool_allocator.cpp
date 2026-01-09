#include "pool_allocator.hpp"
#include "memory_ops.hpp"

namespace core {

namespace {

void InitializePool(u8* begin, memory_size blockSize, memory_size blockCount, void*& outFreeList) noexcept {
    if (begin == nullptr || blockSize == 0 || blockCount == 0) {
        outFreeList = nullptr;
        return;
    }
    
    outFreeList = begin;
    u8* current = begin;
    
    for (memory_size i = 0; i < blockCount - 1; ++i) {
        u8* next = current + blockSize;
        detail::SetNextFreeBlock(current, next);
        current = next;
    }
    
    detail::SetNextFreeBlock(current, nullptr);
}

} // anonymous namespace

PoolAllocator::PoolAllocator(void* buffer, memory_size bufferSize, memory_size blockSize) noexcept
    : _upstream(nullptr)
{
    if (buffer == nullptr || bufferSize == 0 || blockSize == 0) {
        _begin = nullptr;
        _end = nullptr;
        _freeList = nullptr;
        _blockSize = 0;
        _blockCount = 0;
        return;
    }
    
    _blockSize = detail::ComputeBlockSize(blockSize, CORE_DEFAULT_ALIGNMENT);
    _blockCount = bufferSize / _blockSize;
    
    if (_blockCount == 0) {
        _begin = nullptr;
        _end = nullptr;
        _freeList = nullptr;
        _blockSize = 0;
        return;
    }
    
    _begin = static_cast<u8*>(buffer);
    _end = _begin + (_blockSize * _blockCount);
    
    InitializePool(_begin, _blockSize, _blockCount, _freeList);
}

PoolAllocator::PoolAllocator(memory_size blockSize, memory_size blockCount, IAllocator& upstream) noexcept
    : _upstream(&upstream)
{
    if (blockSize == 0 || blockCount == 0) {
        _begin = nullptr;
        _end = nullptr;
        _freeList = nullptr;
        _blockSize = 0;
        _blockCount = 0;
        return;
    }
    
    _blockSize = detail::ComputeBlockSize(blockSize, CORE_DEFAULT_ALIGNMENT);
    _blockCount = blockCount;
    
    const memory_size totalSize = _blockSize * _blockCount;
    _begin = static_cast<u8*>(AllocateBytes(upstream, totalSize));
    
    if (_begin == nullptr) {
        _end = nullptr;
        _freeList = nullptr;
        _blockSize = 0;
        _blockCount = 0;
        return;
    }
    
    _end = _begin + totalSize;
    
    InitializePool(_begin, _blockSize, _blockCount, _freeList);
}

PoolAllocator::~PoolAllocator() noexcept {
    if (_upstream && _begin) {
        const memory_size totalSize = _blockSize * _blockCount;
        DeallocateBytes(*_upstream, _begin, totalSize);
    }
}

void* PoolAllocator::Allocate(const AllocationRequest& request) noexcept {
    if (request.size == 0) {
        return nullptr;
    }
    
    if (_begin == nullptr) {
        return nullptr;
    }

#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(request.size <= _blockSize && 
                    "PoolAllocator: requested size exceeds block size");
    
    const memory_alignment reqAlignment = detail::NormalizeAlignment(request.alignment);
    CORE_MEM_ASSERT(reqAlignment <= CORE_DEFAULT_ALIGNMENT && 
                    "PoolAllocator: requested alignment exceeds block alignment");
#endif

    if (_freeList == nullptr) {
#if CORE_MEMORY_DEBUG
        if (Any(request.flags & AllocationFlags::NoFail)) {
            CORE_MEM_ASSERT(false && "PoolAllocator: out of memory with NoFail flag");
        }
#endif
        return nullptr;
    }

    void* block = _freeList;
    _freeList = detail::GetNextFreeBlock(_freeList);

    if (Any(request.flags & AllocationFlags::ZeroInitialize)) {
        memory_zero(block, _blockSize);
    }

    return block;
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

