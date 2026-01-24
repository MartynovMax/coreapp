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
    void* ptr;
    u32 size;
    u64 allocTime; // Operation index when allocated
};

class LifetimeTracker {
public:
    LifetimeTracker(LifetimeModel model, u32 maxLiveObjects,
                    SeededRNG& rng, IAllocator* allocator) noexcept;
    ~LifetimeTracker() noexcept;

    // Register a new allocation
    void Track(void* ptr, u32 size, u64 opIndex) noexcept;

    // Select an object to free (according to model)
    void* SelectForFree() noexcept;

    // Get all live objects
    void GetAllLive(AllocInfo** outArray, u32* outCount) noexcept;

    // Clear all tracks (bulk reclaim)
    void Clear() noexcept;

    // Metrics
    u32 GetLiveCount() const noexcept;
    u64 GetLiveBytes() const noexcept;
    u64 GetPeakBytes() const noexcept;

    void Remove(const void* ptr) noexcept;

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
};

} // namespace bench
} // namespace core
