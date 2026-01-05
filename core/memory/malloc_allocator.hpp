#pragma once

// =============================================================================
// malloc_allocator.hpp
// General-purpose allocator built on top of SystemAllocator.
//
// This is a thin wrapper around SystemAllocator that can be extended with
// additional features like bookkeeping, statistics, or custom behavior.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"

namespace core {

class SystemAllocator;

class MallocAllocator final : public IAllocator {
public:
    MallocAllocator() noexcept;
    
    explicit MallocAllocator(IAllocator& backing) noexcept;
    
    ~MallocAllocator() noexcept override = default;

    MallocAllocator(const MallocAllocator&) = delete;
    MallocAllocator& operator=(const MallocAllocator&) = delete;

    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;

    IAllocator& GetBacking() const noexcept { return *backing_; }

private:
    IAllocator* backing_;
};

} // namespace core

