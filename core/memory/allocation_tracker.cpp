#include "allocation_tracker.hpp"
#include "../base/core_assert.hpp"
#include "../concurrency/core_atomic.hpp"
#include "../concurrency/core_sync.hpp"
#include "../concurrency/tls_ptr.hpp"

namespace core {

#if CORE_MEMORY_TRACKING

namespace detail {

struct ListenerEntry {
    AllocationListenerFn callback = nullptr;
    void* userData = nullptr;
    u32 handle = 0;  // 0 = unused slot
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

atomic_u64 _currentAllocated{0};
atomic_u64 _peakAllocated{0};
atomic_u64 _totalAllocations{0};
atomic_u64 _totalDeallocations{0};

TagStatsEntry _tagStats[CORE_ALLOCATION_TRACKER_MAX_TAGS];
spin_mutex _tagStatsMutex;
spin_mutex _listenersMutex;

atomic_u32 _trackingEnabled{0};
using InsideTrackerTLS = tls_value<bool>;

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
    
    TagStatsEntry* existing = FindTagEntry(tag);
    if (existing) {
        return existing;
    }
    
    for (auto& entry : _tagStats) {
        if (entry.tag == 0) {
            entry.tag = tag;
            return &entry;
        }
    }
    
    return nullptr;
}

void UpdateStatsOnAllocation(memory_size size, memory_tag tag) noexcept {
    u64 newCurrent = _currentAllocated.fetch_add(size, memory_order::relaxed) + size;
    
    // CAS loop for peak tracking
    u64 currentPeak = _peakAllocated.load(memory_order::relaxed);
    while (newCurrent > currentPeak) {
        if (_peakAllocated.compare_exchange_weak(currentPeak, newCurrent, 
            memory_order::relaxed, memory_order::relaxed)) {
            break;
        }
    }
    
    (void)_totalAllocations.fetch_add(1, memory_order::relaxed);
    
    if (tag != 0) {
        scoped_lock<spin_mutex> lock(_tagStatsMutex);
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
    (void)_currentAllocated.fetch_sub(size, memory_order::relaxed);
    (void)_totalDeallocations.fetch_add(1, memory_order::relaxed);
    
    if (tag != 0) {
        scoped_lock<spin_mutex> lock(_tagStatsMutex);
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
    AllocationListenerFn callbacks[CORE_ALLOCATION_TRACKER_MAX_LISTENERS];
    void* userDatas[CORE_ALLOCATION_TRACKER_MAX_LISTENERS];
    u32 count = 0;
    
    {
        scoped_lock<spin_mutex> lock(_listenersMutex);
        for (const auto& entry : _listeners) {
            if (entry.callback != nullptr) {
                callbacks[count] = entry.callback;
                userDatas[count] = entry.userData;
                ++count;
            }
        }
    }
    
    for (u32 i = 0; i < count; ++i) {
        callbacks[i](event, allocator, request, info, userDatas[i]);
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
    
    if (_trackingEnabled.load(memory_order::relaxed) == 0) {
        return;
    }
    
    if (InsideTrackerTLS::get()) {
        return;
    }
    InsideTrackerTLS::set(true);
    
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
    
    NotifyListeners(event, allocator, request, info);
    
    InsideTrackerTLS::set(false);
}

AllocationListenerHandle RegisterAllocationListenerImpl(
    AllocationListenerFn callback,
    void* userData) noexcept
{
    if (!callback) {
        return kInvalidListenerHandle;
    }
    
    scoped_lock<spin_mutex> lock(_listenersMutex);
    
    for (auto& entry : _listeners) {
        if (entry.callback == nullptr) {
            entry.callback = callback;
            entry.userData = userData;
            entry.handle = _nextHandle++;
            if (_nextHandle == kInvalidListenerHandle) {
                _nextHandle = 1;
            }
            return entry.handle;
        }
    }
    
    return kInvalidListenerHandle;
}

bool UnregisterAllocationListenerImpl(AllocationListenerHandle handle) noexcept {
    if (handle == kInvalidListenerHandle) {
        return false;
    }
    
    scoped_lock<spin_mutex> lock(_listenersMutex);
    
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
    scoped_lock<spin_mutex> lock(_listenersMutex);
    
    for (auto& entry : _listeners) {
        entry.callback = nullptr;
        entry.userData = nullptr;
        entry.handle = 0;
    }
}

void EnableAllocationTrackingImpl() noexcept {
    _trackingEnabled.store(1, memory_order::relaxed);
}

void DisableAllocationTrackingImpl() noexcept {
    _trackingEnabled.store(0, memory_order::relaxed);
}

bool IsAllocationTrackingEnabledImpl() noexcept {
    return _trackingEnabled.load(memory_order::relaxed) != 0;
}

AllocationTrackerStats GetAllocationTrackerStatsImpl() noexcept {
    AllocationTrackerStats stats;
    stats.current_allocated = _currentAllocated.load(memory_order::relaxed);
    stats.peak_allocated = _peakAllocated.load(memory_order::relaxed);
    stats.total_allocations = _totalAllocations.load(memory_order::relaxed);
    stats.total_deallocations = _totalDeallocations.load(memory_order::relaxed);
    return stats;
}

void ResetAllocationTrackerStatsImpl() noexcept {
    _currentAllocated.store(0, memory_order::relaxed);
    _peakAllocated.store(0, memory_order::relaxed);
    _totalAllocations.store(0, memory_order::relaxed);
    _totalDeallocations.store(0, memory_order::relaxed);
}

bool GetTagStatsImpl(memory_tag tag, TagStats& outStats) noexcept {
    scoped_lock<spin_mutex> lock(_tagStatsMutex);
    
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
    
    scoped_lock<spin_mutex> lock(_tagStatsMutex);
    
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
    scoped_lock<spin_mutex> lock(_tagStatsMutex);
    
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
    bool registered = AddAllocationHook(detail::TrackerHookCallback, nullptr);
    CORE_MEM_ASSERT(registered && "Failed to register allocation tracker hook");
    
    detail::_trackingEnabled.store(1, memory_order::relaxed);
#endif
}

void ShutdownAllocationTracker() noexcept {
#if CORE_MEMORY_TRACKING
    detail::_trackingEnabled.store(0, memory_order::relaxed);
    RemoveAllocationHook(detail::TrackerHookCallback);
#endif
}

} // namespace core

