#include "segregated_list_allocator.hpp"

namespace core {

SegregatedListAllocator::SegregatedListAllocator() noexcept
    : _classCount(0)
{
}

SegregatedListAllocator::~SegregatedListAllocator() noexcept {
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

