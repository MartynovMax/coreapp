#include "segregated_list_allocator.hpp"
#include "pool_allocator.hpp"

namespace core {

SegregatedListAllocator::SegregatedListAllocator(
    const SizeClassConfig* configs,
    memory_size configCount,
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

    for (memory_size i = 0; i < configCount; ++i) {
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
    for (memory_size i = 0; i < _classCount; ++i) {
        if (_classes[i].pool != nullptr) {
            _classes[i].pool->~PoolAllocator();
            DeallocateObject(*_upstream, _classes[i].pool);
        }
    }
}

void* SegregatedListAllocator::Allocate(const AllocationRequest& request) noexcept {
    CORE_UNUSED(request);
    return nullptr;
}

void SegregatedListAllocator::Deallocate(const AllocationInfo& info) noexcept {
    CORE_UNUSED(info);
}

bool SegregatedListAllocator::Owns(const void* ptr) const noexcept {
    CORE_UNUSED(ptr);
    return false;
}

memory_size SegregatedListAllocator::SelectSizeClass(memory_size size) const noexcept {
    for (memory_size i = 0; i < _classCount; ++i) {
        if (size <= _classes[i].block_size) {
            return i;
        }
    }
    return kInvalidClass;
}

} // namespace core

