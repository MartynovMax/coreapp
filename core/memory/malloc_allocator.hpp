#pragma once

// =============================================================================
// malloc_allocator.hpp
// Simple allocator wrapping C runtime malloc/free.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"

namespace core {

class MallocAllocator : public IAllocator {
public:
    MallocAllocator() noexcept = default;
    ~MallocAllocator() noexcept override = default;

    MallocAllocator(const MallocAllocator&) = delete;
    MallocAllocator& operator=(const MallocAllocator&) = delete;

    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;
};

} // namespace core

