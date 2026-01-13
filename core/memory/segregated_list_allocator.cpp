#include "segregated_list_allocator.hpp"
#include "pool_allocator.hpp"

// Placement new operator (no STL dependency)
inline void* operator new(core::usize, void* ptr) noexcept {
    return ptr;
}

namespace core {

SegregatedListAllocator::SegregatedListAllocator(
    const SizeClassConfig* configs,
    u32 configCount,
    IAllocator& upstream,
    IAllocator& fallback) noexcept
    : _classCount(0)
    , _maxClassSize(0)
    , _upstream(&upstream)
    , _fallback(&fallback)
{
    if (configs == nullptr || configCount == 0 || configCount > kMaxSizeClasses) {
        return;
    }

    for (u32 i = 0; i < configCount; ++i) {
        memory_size blockSize = configs[i].block_size;
        memory_size blockCount = configs[i].block_count;

        if (blockSize == 0 || blockCount == 0) {
            continue;
        }

        PoolAllocator* pool = AllocateObject<PoolAllocator>(upstream);
        if (pool == nullptr) {
            break;
        }

        new (pool) PoolAllocator(blockSize, blockCount, upstream);

        _classes[_classCount].block_size = pool->BlockSize();
        _classes[_classCount].pool = pool;
        _classes[_classCount].block_count = blockCount;

        if (_classes[_classCount].block_size > _maxClassSize) {
            _maxClassSize = _classes[_classCount].block_size;
        }

        ++_classCount;
    }
}

SegregatedListAllocator::~SegregatedListAllocator() noexcept {
    for (u32 i = 0; i < _classCount; ++i) {
        if (_classes[i].pool != nullptr) {
            _classes[i].pool->~PoolAllocator();
            DeallocateObject(*_upstream, _classes[i].pool);
        }
    }
}

void* SegregatedListAllocator::Allocate(const AllocationRequest& request) noexcept {
    if (request.size == 0) {
        return nullptr;
    }

    const memory_alignment reqAlignment = detail::NormalizeAlignment(request.alignment);

    if (reqAlignment > CORE_DEFAULT_ALIGNMENT) {
        return _fallback->Allocate(request);
    }

    if (request.size > _maxClassSize) {
        return _fallback->Allocate(request);
    }

    u32 classIndex = SelectSizeClass(request.size);
    if (classIndex == kInvalidClass) {
        return _fallback->Allocate(request);
    }

    void* ptr = _classes[classIndex].pool->Allocate(request);
    
    if (ptr == nullptr) {
        return _fallback->Allocate(request);
    }

    return ptr;
}

void SegregatedListAllocator::Deallocate(const AllocationInfo& info) noexcept {
    if (info.ptr == nullptr) {
        return;
    }

    if (info.size > 0 && info.size <= _maxClassSize) {
        u32 classIndex = SelectSizeClass(info.size);
        if (classIndex != kInvalidClass) {
            _classes[classIndex].pool->Deallocate(info);
            return;
        }
    }

    for (u32 i = 0; i < _classCount; ++i) {
        if (_classes[i].pool->Owns(info.ptr)) {
            _classes[i].pool->Deallocate(info);
            return;
        }
    }

    _fallback->Deallocate(info);
}

bool SegregatedListAllocator::Owns(const void* ptr) const noexcept {
    if (ptr == nullptr) {
        return false;
    }

    for (u32 i = 0; i < _classCount; ++i) {
        if (_classes[i].pool->Owns(ptr)) {
            return true;
        }
    }

    return _fallback->Owns(ptr);
}

u32 SegregatedListAllocator::SelectSizeClass(memory_size size) const noexcept {
    for (u32 i = 0; i < _classCount; ++i) {
        if (size <= _classes[i].block_size) {
            return i;
        }
    }
    return kInvalidClass;
}

u32 SegregatedListAllocator::SizeClassCount() const noexcept {
    return _classCount;
}

memory_size SegregatedListAllocator::MaxClassSize() const noexcept {
    return _maxClassSize;
}

memory_size SegregatedListAllocator::ClassBlockSize(u32 classIndex) const noexcept {
    if (classIndex >= _classCount) {
        return 0;
    }
    return _classes[classIndex].block_size;
}

memory_size SegregatedListAllocator::ClassBlockCount(u32 classIndex) const noexcept {
    if (classIndex >= _classCount) {
        return 0;
    }
    return _classes[classIndex].block_count;
}

memory_size SegregatedListAllocator::ClassFreeBlocks(u32 classIndex) const noexcept {
    if (classIndex >= _classCount) {
        return 0;
    }
    return _classes[classIndex].pool->FreeBlocks();
}

memory_size SegregatedListAllocator::ClassUsedBlocks(u32 classIndex) const noexcept {
    if (classIndex >= _classCount) {
        return 0;
    }
    return _classes[classIndex].pool->UsedBlocks();
}

memory_size SegregatedListAllocator::ClassCapacityBytes(u32 classIndex) const noexcept {
    if (classIndex >= _classCount) {
        return 0;
    }
    return _classes[classIndex].pool->CapacityBytes();
}

} // namespace core

