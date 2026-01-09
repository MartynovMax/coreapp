#include "stack_allocator.hpp"

namespace core {

StackAllocator::StackAllocator(void* buffer, memory_size size) noexcept
    : _begin(nullptr)
    , _current(nullptr)
    , _end(nullptr)
    , _upstream(nullptr)
{
    CORE_UNUSED(buffer);
    CORE_UNUSED(size);
}

StackAllocator::StackAllocator(memory_size capacity, IAllocator& upstream) noexcept
    : _begin(nullptr)
    , _current(nullptr)
    , _end(nullptr)
    , _upstream(nullptr)
{
    CORE_UNUSED(capacity);
    CORE_UNUSED(upstream);
}

StackAllocator::~StackAllocator() noexcept {
}

void* StackAllocator::Allocate(const AllocationRequest& request) noexcept {
    CORE_UNUSED(request);
    return nullptr;
}

void StackAllocator::Deallocate(const AllocationInfo& info) noexcept {
    CORE_UNUSED(info);
}

bool StackAllocator::Owns(const void* ptr) const noexcept {
    CORE_UNUSED(ptr);
    return false;
}

} // namespace core

