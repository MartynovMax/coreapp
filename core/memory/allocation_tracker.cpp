#include "allocation_tracker.hpp"
#include "../base/core_assert.hpp"

namespace core {

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

} // namespace detail

// ----------------------------------------------------------------------------
// Listener API
// ----------------------------------------------------------------------------

AllocationListenerHandle RegisterAllocationListener(
    AllocationListenerFn callback,
    void* userData) noexcept
{
    if (!callback) {
        return kInvalidListenerHandle;
    }
    
    // Find free slot
    for (auto& entry : detail::_listeners) {
        if (entry.callback == nullptr) {
            entry.callback = callback;
            entry.userData = userData;
            entry.handle = detail::_nextHandle++;
            return entry.handle;
        }
    }
    
    return kInvalidListenerHandle;
}

bool UnregisterAllocationListener(AllocationListenerHandle handle) noexcept {
    if (handle == kInvalidListenerHandle) {
        return false;
    }
    
    for (auto& entry : detail::_listeners) {
        if (entry.handle == handle) {
            entry.callback = nullptr;
            entry.userData = nullptr;
            entry.handle = 0;
            return true;
        }
    }
    
    return false;
}

void ClearAllocationListeners() noexcept {
    for (auto& entry : detail::_listeners) {
        entry.callback = nullptr;
        entry.userData = nullptr;
        entry.handle = 0;
    }
}

// ----------------------------------------------------------------------------
// Global statistics
// ----------------------------------------------------------------------------

AllocationTrackerStats GetAllocationTrackerStats() noexcept {
    AllocationTrackerStats stats;
    stats.current_allocated = detail::_globalStats.currentAllocated;
    stats.peak_allocated = detail::_globalStats.peakAllocated;
    stats.total_allocations = detail::_globalStats.totalAllocations;
    stats.total_deallocations = detail::_globalStats.totalDeallocations;
    return stats;
}

void ResetAllocationTrackerStats() noexcept {
    detail::_globalStats.currentAllocated = 0;
    detail::_globalStats.peakAllocated = 0;
    detail::_globalStats.totalAllocations = 0;
    detail::_globalStats.totalDeallocations = 0;
}

// ----------------------------------------------------------------------------
// Per-tag statistics
// ----------------------------------------------------------------------------

bool GetTagStats(memory_tag tag, TagStats& outStats) noexcept {
    detail::TagStatsEntry* entry = detail::FindTagEntry(tag);
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

void EnumerateTagStats(
    void (*callback)(const TagStats& stats, void* user),
    void* userData) noexcept
{
    if (!callback) {
        return;
    }
    
    for (const auto& entry : detail::_tagStats) {
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

void ResetTagStats() noexcept {
    for (auto& entry : detail::_tagStats) {
        entry.tag = 0;
        entry.currentAllocated = 0;
        entry.peakAllocated = 0;
        entry.allocCount = 0;
        entry.deallocCount = 0;
    }
}

} // namespace core

