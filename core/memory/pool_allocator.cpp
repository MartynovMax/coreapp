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
        _freeCount = 0;
        return;
    }
    
    u8* alignedBegin = AlignPtrUp(static_cast<u8*>(buffer), CORE_DEFAULT_ALIGNMENT);
    const memory_size alignmentOffset = static_cast<memory_size>(alignedBegin - static_cast<u8*>(buffer));
    
    if (alignmentOffset >= bufferSize) {
        _begin = nullptr;
        _end = nullptr;
        _freeList = nullptr;
        _blockSize = 0;
        _blockCount = 0;
        _freeCount = 0;
        return;
    }
    
    const memory_size adjustedSize = bufferSize - alignmentOffset;
    
    _blockSize = detail::ComputeBlockSize(blockSize, CORE_DEFAULT_ALIGNMENT);
    _blockCount = adjustedSize / _blockSize;
    
    if (_blockCount == 0) {
        _begin = nullptr;
        _end = nullptr;
        _freeList = nullptr;
        _blockSize = 0;
        _freeCount = 0;
        return;
    }
    
    _begin = alignedBegin;
    _end = _begin + (_blockSize * _blockCount);
    _freeCount = _blockCount;
    
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
        _freeCount = 0;
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
        _freeCount = 0;
        return;
    }
    
    _end = _begin + totalSize;
    _freeCount = _blockCount;
    
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

    const memory_alignment reqAlignment = detail::NormalizeAlignment(request.alignment);
    
    if (reqAlignment > CORE_DEFAULT_ALIGNMENT) {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(false && 
                        "PoolAllocator: requested alignment exceeds block alignment");
#endif
        return nullptr;
    }
    
    if (request.size > _blockSize) {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(false && 
                        "PoolAllocator: requested size exceeds block size");
#endif
        return nullptr;
    }

    if (_freeList == nullptr) {
        if (Any(request.flags & AllocationFlags::NoFail)) {
            FATAL("PoolAllocator: out of memory with NoFail flag");
        }
        return nullptr;
    }

    void* block = _freeList;
    _freeList = detail::GetNextFreeBlock(_freeList);
    --_freeCount;

    if (Any(request.flags & AllocationFlags::ZeroInitialize)) {
        memory_zero(block, request.size);
    }

    return block;
}

void PoolAllocator::Deallocate(const AllocationInfo& info) noexcept {
    if (info.ptr == nullptr) {
        return;
    }

#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(Owns(info.ptr) && 
                    "PoolAllocator: pointer does not belong to this pool");
    
    CORE_MEM_ASSERT(detail::IsBlockAligned(info.ptr, _begin, _blockSize) &&
                    "PoolAllocator: pointer is not aligned to block boundary");
    
    CORE_MEM_ASSERT(_freeCount < _blockCount &&
                    "PoolAllocator: double free or corruption detected");
#endif

    void* block = info.ptr;
    detail::SetNextFreeBlock(block, _freeList);
    _freeList = block;
    ++_freeCount;
}

bool PoolAllocator::Owns(const void* ptr) const noexcept {
    if (ptr == nullptr || _begin == nullptr) {
        return false;
    }
    const u8* p = static_cast<const u8*>(ptr);
    return p >= _begin && p < _end;
}

memory_size PoolAllocator::BlockSize() const noexcept {
    return _blockSize;
}

memory_size PoolAllocator::BlockCount() const noexcept {
    return _blockCount;
}

memory_size PoolAllocator::FreeBlocks() const noexcept {
    return _freeCount;
}

memory_size PoolAllocator::UsedBlocks() const noexcept {
    return _blockCount - _freeCount;
}

memory_size PoolAllocator::CapacityBytes() const noexcept {
    return _blockSize * _blockCount;
}

} // namespace core

