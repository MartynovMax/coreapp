#include "bump_allocator.hpp"

namespace core {

void* BumpAllocator::Allocate(const AllocationRequest& request) noexcept {
    // Implementation will be added in the next step
    CORE_UNUSED(request);
    return nullptr;
}

void BumpAllocator::Deallocate(const AllocationInfo& info) noexcept {
    // Intentional no-op: bump allocators don't support individual frees
    CORE_UNUSED(info);
}

} // namespace core

