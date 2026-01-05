#include "malloc_allocator.hpp"
#include "system_allocator.hpp"

namespace core {

MallocAllocator::MallocAllocator() noexcept 
    : backing_(&SystemAllocator::Instance()) 
{
}

MallocAllocator::MallocAllocator(IAllocator& backing) noexcept 
    : backing_(&backing) 
{
}

void* MallocAllocator::Allocate(const AllocationRequest& request) noexcept {
    return backing_->Allocate(request);
}

void MallocAllocator::Deallocate(const AllocationInfo& info) noexcept {
    backing_->Deallocate(info);
}

IAllocator& GetDefaultAllocator() noexcept {
    static MallocAllocator instance;
    return instance;
}

} // namespace core

