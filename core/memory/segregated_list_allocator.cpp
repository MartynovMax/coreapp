#include "segregated_list_allocator.hpp"

namespace core {

SegregatedListAllocator::SegregatedListAllocator() noexcept {
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

} // namespace core

