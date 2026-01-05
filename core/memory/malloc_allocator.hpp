#pragma once

// =============================================================================
// malloc_allocator.hpp
// General-purpose allocator built on top of SystemAllocator.
//
// This is a thin wrapper around SystemAllocator that can be extended with
// additional features like bookkeeping, statistics, or custom behavior.
//
// Alignment guarantees:
//   - Inherits alignment guarantees from backing allocator
//   - Default backing: SystemAllocator (page-aligned, typically 4KB)
//   - Custom backing: depends on provided allocator
//
// Thread-safety:
//   - Thread-safe if backing allocator is thread-safe
//   - Default backing (SystemAllocator) is thread-safe
//   - Stateless wrapper, safe to use from multiple threads
//
// Default instance:
//   - GetDefaultAllocator() returns a global MallocAllocator instance
//   - Uses SystemAllocator::Instance() as backing
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

