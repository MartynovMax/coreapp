#pragma once

// =============================================================================
// lifetime_tracker.hpp
// Tracking of live objects and implementation of lifetime models.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "workload_params.hpp"
#include "common/seeded_rng.hpp"
#include "core/memory/core_allocator.hpp"

namespace core::bench {

struct AllocInfo {
    void* ptr = nullptr;
    u32 size = 0;

    core::memory_alignment alignment = 0; // 0 => allocator default (but usually you store explicit)
    core::memory_tag tag = 0;

    u64 allocTime = 0; // Operation index when allocated
};

struct TrackResult {
    bool tracked = false;
    bool forcedFree = false;
    AllocInfo freedInfo{};
};

class LifetimeTracker {
public:
    using TrackResult = ::core::bench::TrackResult;

    LifetimeTracker(u32 capacity, LifetimeModel model, SeededRNG& rng, IAllocator* allocator) noexcept;
    ~LifetimeTracker() noexcept;

    // Register a new allocation
    TrackResult Track(void* ptr, u32 size, core::memory_alignment alignment, core::memory_tag tag, u64 opIndex) noexcept;

    // Select an object to free (according to model) and remove it from tracking.
    // Returns false if nothing should be freed.
    bool PopForFree(AllocInfo& out_info) noexcept;

    // Free all tracked allocations via allocator (if available) and clear tracking.
    void FreeAll() noexcept;
    void FreeAll(core::u64* outCount, core::u64* outBytes) noexcept;

    // Get all live objects (view)
    void GetAllLive(AllocInfo** outArray, u32* outCount, u32* outHead = nullptr, bool* outRingMode = nullptr) const noexcept;

    // Clear all tracks (does NOT deallocate!)
    void Clear() noexcept;

    // Metrics
    [[nodiscard]] u32 GetLiveCount() const noexcept;
    [[nodiscard]] u64 GetLiveBytes() const noexcept;
    [[nodiscard]] u64 GetPeakBytes() const noexcept;
    [[nodiscard]] u32 GetPeakCount() const noexcept;
    [[nodiscard]] u32 GetCapacity() const noexcept { return _capacity; }

    // Iterate all live objects in logical order (safe for ring mode)
    using LiveObjectCallback = void(*)(const AllocInfo& info, void* userData);
    void ForEachLive(LiveObjectCallback callback, void* userData) const noexcept;

private:
    LifetimeModel _model;
    u32 _capacity;
    SeededRNG& _rng;
    IAllocator* _allocator;

    AllocInfo* _buffer = nullptr;
    u32 _count = 0;
    u32 _head = 0;
    u32 _tail = 0;
    bool _ringMode = false;

    u64 _totalLiveBytes = 0;
    u64 _peakLiveBytes = 0;
    u32 _peakLiveCount = 0;

    void RemoveIndex(u32 idx) noexcept;
public:
    [[nodiscard]] bool isValid() const noexcept { return _buffer != nullptr && _capacity > 0; }
};

} // namespace core::bench
