#pragma once

// =============================================================================
// segregated_list_allocator.hpp
// Allocator that manages multiple size classes, each backed by its own pool.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

class SegregatedListAllocator final : public IAllocator {
public:
    SegregatedListAllocator() noexcept;
    ~SegregatedListAllocator() noexcept override;

    SegregatedListAllocator(const SegregatedListAllocator&) = delete;
    SegregatedListAllocator& operator=(const SegregatedListAllocator&) = delete;

    // IAllocator interface
    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;
    bool Owns(const void* ptr) const noexcept override;
};

} // namespace core

