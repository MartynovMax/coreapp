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
    static constexpr memory_size kMaxSizeClasses = 32;
    static constexpr memory_size kInvalidClass = static_cast<memory_size>(-1);

    SegregatedListAllocator(
        const SizeClassConfig* configs,
        memory_size configCount,
        IAllocator& upstream,
        IAllocator& fallback) noexcept;
    
    ~SegregatedListAllocator() noexcept override;

    SegregatedListAllocator(const SegregatedListAllocator&) = delete;
    SegregatedListAllocator& operator=(const SegregatedListAllocator&) = delete;

    // IAllocator interface
    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;
    bool Owns(const void* ptr) const noexcept override;

private:

    memory_size SelectSizeClass(memory_size size) const noexcept;

    detail::SizeClass _classes[kMaxSizeClasses];
    memory_size _classCount;
    memory_size _maxClassSize;
    IAllocator* _upstream;
    IAllocator* _fallback;
};

} // namespace core

