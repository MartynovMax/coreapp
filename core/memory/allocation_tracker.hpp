#pragma once

// =============================================================================
// allocation_tracker.hpp
// Global allocation tracking API for observing memory allocations.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"

namespace core {

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

#ifndef CORE_ALLOCATION_TRACKER_MAX_LISTENERS
#define CORE_ALLOCATION_TRACKER_MAX_LISTENERS 8
#endif

#ifndef CORE_ALLOCATION_TRACKER_MAX_TAGS
#define CORE_ALLOCATION_TRACKER_MAX_TAGS 64
#endif

// ----------------------------------------------------------------------------
// Listener API
// ----------------------------------------------------------------------------

using AllocationListenerHandle = u32;
constexpr AllocationListenerHandle kInvalidListenerHandle = 0;

// Listener callback signature
using AllocationListenerFn = void (*)(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info,
    void* userData
) noexcept;

// Register allocation listener
AllocationListenerHandle RegisterAllocationListener(
    AllocationListenerFn callback,
    void* userData = nullptr
) noexcept;

// Unregister allocation listener
bool UnregisterAllocationListener(AllocationListenerHandle handle) noexcept;

// Clear all listeners
void ClearAllocationListeners() noexcept;

// ----------------------------------------------------------------------------
// Tracking control
// ----------------------------------------------------------------------------

void EnableAllocationTracking() noexcept;
void DisableAllocationTracking() noexcept;
bool IsAllocationTrackingEnabled() noexcept;

// ----------------------------------------------------------------------------
// Global statistics
// ----------------------------------------------------------------------------

struct AllocationTrackerStats {
    memory_size current_allocated;
    memory_size peak_allocated;
    memory_size total_allocations;
    memory_size total_deallocations;
};

AllocationTrackerStats GetAllocationTrackerStats() noexcept;
void ResetAllocationTrackerStats() noexcept;

// ----------------------------------------------------------------------------
// Per-tag statistics
// ----------------------------------------------------------------------------

struct TagStats {
    memory_tag tag;
    memory_size current_allocated;
    memory_size peak_allocated;
    memory_size alloc_count;
    memory_size dealloc_count;
};

bool GetTagStats(memory_tag tag, TagStats& outStats) noexcept;

void EnumerateTagStats(
    void (*callback)(const TagStats& stats, void* user),
    void* userData = nullptr
) noexcept;

void ResetTagStats() noexcept;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void InitializeAllocationTracker() noexcept;
void ShutdownAllocationTracker() noexcept;

} // namespace core

