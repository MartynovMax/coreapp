#include "allocation_tracker.hpp"
#include "../base/core_assert.hpp"

namespace core {

#if CORE_MEMORY_TRACKING

namespace detail {

struct ListenerEntry {
    AllocationListenerFn callback = nullptr;
    void* userData = nullptr;
    u32 handle = 0;  // 0 = unused slot
};

struct GlobalStats {
    memory_size currentAllocated = 0;
    memory_size peakAllocated = 0;
    memory_size totalAllocations = 0;
    memory_size totalDeallocations = 0;
};

struct TagStatsEntry {
    memory_tag tag = 0;  // 0 = unused slot
    memory_size currentAllocated = 0;
    memory_size peakAllocated = 0;
    memory_size allocCount = 0;
    memory_size deallocCount = 0;
};

ListenerEntry _listeners[CORE_ALLOCATION_TRACKER_MAX_LISTENERS];
u32 _nextHandle = 1;
GlobalStats _globalStats;
TagStatsEntry _tagStats[CORE_ALLOCATION_TRACKER_MAX_TAGS];

bool _trackingEnabled = false;
bool _insideTracker = false;  // Re-entrancy guard

TagStatsEntry* FindTagEntry(memory_tag tag) noexcept {
    if (tag == 0) {
        return nullptr;
    }
    
    for (auto& entry : _tagStats) {
        if (entry.tag == tag) {
            return &entry;
        }
    }
    return nullptr;
}

TagStatsEntry* FindOrCreateTagEntry(memory_tag tag) noexcept {
    if (tag == 0) {
        return nullptr;
    }
    
    // Try to find existing
    TagStatsEntry* existing = FindTagEntry(tag);
    if (existing) {
        return existing;
    }
    
    // Find free slot
    for (auto& entry : _tagStats) {
        if (entry.tag == 0) {
            entry.tag = tag;
            return &entry;
        }
    }
    
    return nullptr;  // Table full
}

void UpdateStatsOnAllocation(memory_size size, memory_tag tag) noexcept {
    // Update global stats
    // NOTE: Not thread-safe - may race under concurrent allocations
    // Full thread-safety will be added in epic #95
    _globalStats.currentAllocated += size;
    _globalStats.totalAllocations++;
    
    if (_globalStats.currentAllocated > _globalStats.peakAllocated) {
        _globalStats.peakAllocated = _globalStats.currentAllocated;
    }
    
    // Update per-tag stats
    if (tag != 0) {
        TagStatsEntry* entry = FindOrCreateTagEntry(tag);
        if (entry) {
            entry->currentAllocated += size;
            entry->allocCount++;
            
            if (entry->currentAllocated > entry->peakAllocated) {
                entry->peakAllocated = entry->currentAllocated;
            }
        }
    }
}

void UpdateStatsOnDeallocation(memory_size size, memory_tag tag) noexcept {
    // Update global stats
    // NOTE: Not thread-safe - may race under concurrent deallocations
    _globalStats.currentAllocated -= size;
    _globalStats.totalDeallocations++;
    
    // Update per-tag stats
    if (tag != 0) {
        TagStatsEntry* entry = FindTagEntry(tag);
        if (entry) {
            entry->currentAllocated -= size;
            entry->deallocCount++;
        }
    }
}

void NotifyListeners(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info) noexcept
{
    for (const auto& entry : _listeners) {
        if (entry.callback != nullptr) {
            entry.callback(event, allocator, request, info, entry.userData);
        }
    }
}

void TrackerHookCallback(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info,
    void* user) noexcept
{
    CORE_UNUSED(user);
    
    // Check if tracking is enabled
    if (!_trackingEnabled) {
        return;
    }
    
    // Re-entrancy guard
    if (_insideTracker) {
        return;
    }
    _insideTracker = true;
    
    // Update statistics
    switch (event) {
        case AllocationEvent::AllocateEnd:
            if (info && info->ptr && request) {
                UpdateStatsOnAllocation(info->size, request->tag);
            }
            break;
            
        case AllocationEvent::DeallocateBegin:
            if (info && info->ptr) {
                UpdateStatsOnDeallocation(info->size, info->tag);
            }
            break;
            
        default:
            break;
    }
    
    // Notify listeners
    NotifyListeners(event, allocator, request, info);
    
    _insideTracker = false;
}

AllocationListenerHandle RegisterAllocationListenerImpl(
    AllocationListenerFn callback,
    void* userData) noexcept
{
    if (!callback) {
        return kInvalidListenerHandle;
    }
    
    // Find free slot
    for (auto& entry : _listeners) {
        if (entry.callback == nullptr) {
            entry.callback = callback;
            entry.userData = userData;
            entry.handle = _nextHandle++;
            return entry.handle;
        }
    }
    
    return kInvalidListenerHandle;
}

bool UnregisterAllocationListenerImpl(AllocationListenerHandle handle) noexcept {
    if (handle == kInvalidListenerHandle) {
        return false;
    }
    
    for (auto& entry : _listeners) {
        if (entry.handle == handle) {
            entry.callback = nullptr;
            entry.userData = nullptr;
            entry.handle = 0;
            return true;
        }
    }
    
    return false;
}

void ClearAllocationListenersImpl() noexcept {
    for (auto& entry : _listeners) {
        entry.callback = nullptr;
        entry.userData = nullptr;
        entry.handle = 0;
    }
}

void EnableAllocationTrackingImpl() noexcept {
    _trackingEnabled = true;
}

void DisableAllocationTrackingImpl() noexcept {
    _trackingEnabled = false;
}

bool IsAllocationTrackingEnabledImpl() noexcept {
    return _trackingEnabled;
}

AllocationTrackerStats GetAllocationTrackerStatsImpl() noexcept {
    AllocationTrackerStats stats;
    stats.current_allocated = _globalStats.currentAllocated;
    stats.peak_allocated = _globalStats.peakAllocated;
    stats.total_allocations = _globalStats.totalAllocations;
    stats.total_deallocations = _globalStats.totalDeallocations;
    return stats;
}

void ResetAllocationTrackerStatsImpl() noexcept {
    _globalStats.currentAllocated = 0;
    _globalStats.peakAllocated = 0;
    _globalStats.totalAllocations = 0;
    _globalStats.totalDeallocations = 0;
}

bool GetTagStatsImpl(memory_tag tag, TagStats& outStats) noexcept {
    TagStatsEntry* entry = FindTagEntry(tag);
    if (!entry) {
        return false;
    }
    
    outStats.tag = entry->tag;
    outStats.current_allocated = entry->currentAllocated;
    outStats.peak_allocated = entry->peakAllocated;
    outStats.alloc_count = entry->allocCount;
    outStats.dealloc_count = entry->deallocCount;
    return true;
}

void EnumerateTagStatsImpl(
    void (*callback)(const TagStats& stats, void* user),
    void* userData) noexcept
{
    if (!callback) {
        return;
    }
    
    for (const auto& entry : _tagStats) {
        if (entry.tag != 0) {
            TagStats stats;
            stats.tag = entry.tag;
            stats.current_allocated = entry.currentAllocated;
            stats.peak_allocated = entry.peakAllocated;
            stats.alloc_count = entry.allocCount;
            stats.dealloc_count = entry.deallocCount;
            
            callback(stats, userData);
        }
    }
}

void ResetTagStatsImpl() noexcept {
    for (auto& entry : _tagStats) {
        entry.tag = 0;
        entry.currentAllocated = 0;
        entry.peakAllocated = 0;
        entry.allocCount = 0;
        entry.deallocCount = 0;
    }
}

} // namespace detail

#endif // CORE_MEMORY_TRACKING

// ----------------------------------------------------------------------------
// Listener API
// ----------------------------------------------------------------------------

AllocationListenerHandle RegisterAllocationListener(
    AllocationListenerFn callback,
    void* userData) noexcept
{
#if CORE_MEMORY_TRACKING
    return detail::RegisterAllocationListenerImpl(callback, userData);
#else
    CORE_UNUSED(callback);
    CORE_UNUSED(userData);
    return kInvalidListenerHandle;
#endif
}

bool UnregisterAllocationListener(AllocationListenerHandle handle) noexcept {
#if CORE_MEMORY_TRACKING
    return detail::UnregisterAllocationListenerImpl(handle);
#else
    CORE_UNUSED(handle);
    return false;
#endif
}

void ClearAllocationListeners() noexcept {
#if CORE_MEMORY_TRACKING
    detail::ClearAllocationListenersImpl();
#endif
}

// ----------------------------------------------------------------------------
// Tracking control
// ----------------------------------------------------------------------------

void EnableAllocationTracking() noexcept {
#if CORE_MEMORY_TRACKING
    detail::EnableAllocationTrackingImpl();
#endif
}

void DisableAllocationTracking() noexcept {
#if CORE_MEMORY_TRACKING
    detail::DisableAllocationTrackingImpl();
#endif
}

bool IsAllocationTrackingEnabled() noexcept {
#if CORE_MEMORY_TRACKING
    return detail::IsAllocationTrackingEnabledImpl();
#else
    return false;
#endif
}

// ----------------------------------------------------------------------------
// Global statistics
// ----------------------------------------------------------------------------

AllocationTrackerStats GetAllocationTrackerStats() noexcept {
#if CORE_MEMORY_TRACKING
    return detail::GetAllocationTrackerStatsImpl();
#else
    return {};
#endif
}

void ResetAllocationTrackerStats() noexcept {
#if CORE_MEMORY_TRACKING
    detail::ResetAllocationTrackerStatsImpl();
#endif
}

// ----------------------------------------------------------------------------
// Per-tag statistics
// ----------------------------------------------------------------------------

bool GetTagStats(memory_tag tag, TagStats& outStats) noexcept {
#if CORE_MEMORY_TRACKING
    return detail::GetTagStatsImpl(tag, outStats);
#else
    CORE_UNUSED(tag);
    CORE_UNUSED(outStats);
    return false;
#endif
}

void EnumerateTagStats(
    void (*callback)(const TagStats& stats, void* user),
    void* userData) noexcept
{
#if CORE_MEMORY_TRACKING
    detail::EnumerateTagStatsImpl(callback, userData);
#else
    CORE_UNUSED(callback);
    CORE_UNUSED(userData);
#endif
}

void ResetTagStats() noexcept {
#if CORE_MEMORY_TRACKING
    detail::ResetTagStatsImpl();
#endif
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void InitializeAllocationTracker() noexcept {
#if CORE_MEMORY_TRACKING
    // Register tracker as a hook
    bool registered = AddAllocationHook(detail::TrackerHookCallback, nullptr);
    CORE_MEM_ASSERT(registered && "Failed to register allocation tracker hook");
    
    // Enable tracking by default
    detail::_trackingEnabled = true;
#endif
}

void ShutdownAllocationTracker() noexcept {
#if CORE_MEMORY_TRACKING
    // Disable tracking
    detail::_trackingEnabled = false;
    
    // Unregister hook
    RemoveAllocationHook(detail::TrackerHookCallback);
#endif
}

} // namespace core

