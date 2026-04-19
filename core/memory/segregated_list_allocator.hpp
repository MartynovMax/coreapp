#pragma once

// =============================================================================
// segregated_list_allocator.hpp
// Allocator that manages multiple size classes, each backed by its own pool.
//
// Tuning: Configure size classes based on allocation patterns. Typical: 16, 32,
//         64, 128, 256, 512 bytes. More classes = better fit, more overhead.
//
// Extension: Upstream allocator creates pools, fallback handles large sizes.
//            Introspection methods help profile and adjust configuration.
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
    static constexpr u32 kMaxSizeClasses = 32;
    static constexpr u32 kInvalidClass = static_cast<u32>(-1);

    SegregatedListAllocator(
        const SizeClassConfig* configs,
        u32 configCount,
        IAllocator& upstream,
        IAllocator& fallback) noexcept;
    
    ~SegregatedListAllocator() noexcept override;

    SegregatedListAllocator(const SegregatedListAllocator&) = delete;
    SegregatedListAllocator& operator=(const SegregatedListAllocator&) = delete;

    // IAllocator interface
    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;
    bool Owns(const void* ptr) const noexcept override;

    // Introspection
    u32 SizeClassCount() const noexcept;
    memory_size MaxClassSize() const noexcept;
    memory_size ClassBlockSize(u32 classIndex) const noexcept;
    memory_size ClassBlockCount(u32 classIndex) const noexcept;
    memory_size ClassFreeBlocks(u32 classIndex) const noexcept;
    memory_size ClassUsedBlocks(u32 classIndex) const noexcept;
    memory_size ClassCapacityBytes(u32 classIndex) const noexcept;

    // Fallback allocation counter: incremented whenever an allocation is
    // routed to the fallback allocator (oversized, alignment mismatch, pool full).
    u64 FallbackCount() const noexcept { return _fallbackCount; }
    void ResetFallbackCount() noexcept { _fallbackCount = 0; }

private:

    u32 SelectSizeClass(memory_size size) const noexcept;

    detail::SizeClass _classes[kMaxSizeClasses];
    u32 _classCount;
    memory_size _maxClassSize;
    IAllocator* _upstream;
    IAllocator* _fallback;
    u64 _fallbackCount = 0;
};

} // namespace core

