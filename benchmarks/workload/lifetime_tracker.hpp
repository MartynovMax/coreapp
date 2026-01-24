#pragma once

// =============================================================================
// lifetime_tracker.hpp
// Tracking of live objects and implementation of lifetime models.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "workload_params.hpp"
#include "common/seeded_rng.hpp"
#include "core/memory/core_allocator.hpp"

namespace core {
namespace bench {

struct AllocInfo {
    void* ptr = nullptr;
    u32 size = 0;

    core::memory_alignment alignment = 0; // 0 => allocator default (but usually you store explicit)
    core::memory_tag tag = 0;

    u64 allocTime = 0; // Operation index when allocated
};

class LifetimeTracker {
public:
    LifetimeTracker(LifetimeModel model, u32 maxLiveObjects,
                    SeededRNG& rng, IAllocator* allocator) noexcept;
    ~LifetimeTracker() noexcept;

    // Register a new allocation
    void Track(void* ptr, u32 size, core::memory_alignment alignment, core::memory_tag tag, u64 opIndex) noexcept;

    // Select an object to free (according to model) and remove it from tracking.
    // Returns false if nothing should be freed.
    bool PopForFree(AllocInfo& out_info) noexcept;

    // Free all tracked allocations via allocator (if available) and clear tracking.
    void FreeAll() noexcept;

    // Get all live objects (view)
    void GetAllLive(AllocInfo** outArray, u32* outCount) const noexcept;

    // Clear all tracks (does NOT deallocate!)
    void Clear() noexcept;

    // Metrics
    u32 GetLiveCount() const noexcept;
    u64 GetLiveBytes() const noexcept;
    u64 GetPeakBytes() const noexcept;
    u32 GetPeakCount() const noexcept;

private:
    LifetimeModel _model;
    u32 _maxLiveObjects;
    SeededRNG& _rng;
    IAllocator* _allocator;

    AllocInfo* _buffer = nullptr;
    u32 _capacity = 0;
    u32 _count = 0;

    u64 _totalLiveBytes = 0;
    u64 _peakLiveBytes = 0;
    u32 _peakLiveCount = 0;

    void RemoveIndex(u32 idx) noexcept;
};

} // namespace bench
} // namespace core
