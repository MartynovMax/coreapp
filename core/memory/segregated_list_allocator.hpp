#pragma once

// =============================================================================
// segregated_list_allocator.hpp
// Allocator that manages multiple size classes, each backed by its own pool.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

class PoolAllocator;

// Configuration for a single size class
struct SizeClassConfig {
    memory_size block_size;
    memory_size block_count;
};

namespace detail {

struct SizeClass {
    memory_size block_size;
    PoolAllocator* pool;
    memory_size block_count;
};

} // namespace detail

class SegregatedListAllocator final : public IAllocator {
public:
    // Maximum number of size classes supported
    static constexpr memory_size kMaxSizeClasses = 32;

    SegregatedListAllocator() noexcept;
    ~SegregatedListAllocator() noexcept override;

    SegregatedListAllocator(const SegregatedListAllocator&) = delete;
    SegregatedListAllocator& operator=(const SegregatedListAllocator&) = delete;

    // IAllocator interface
    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;
    bool Owns(const void* ptr) const noexcept override;

private:
    detail::SizeClass _classes[kMaxSizeClasses];  // Array of size classes
    memory_size _class_count;                     // Number of active size classes
};

} // namespace core

