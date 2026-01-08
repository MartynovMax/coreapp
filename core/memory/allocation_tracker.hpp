#pragma once

// =============================================================================
// allocation_tracker.hpp
// Global allocation tracking API for observing memory allocations.
//
// Thread-safe: Uses lock-free atomics for statistics and mutexes for listeners.
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

// WARNING: Listeners MUST NOT perform allocations to avoid recursion
using AllocationListenerFn = void (*)(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info,
    void* userData
) noexcept;

AllocationListenerHandle RegisterAllocationListener(
    AllocationListenerFn callback,
    void* userData = nullptr
) noexcept;

bool UnregisterAllocationListener(AllocationListenerHandle handle) noexcept;
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
    memory_size current_allocated;    // Currently allocated bytes
    memory_size peak_allocated;       // Peak allocated bytes (high water mark)
    memory_size total_allocations;    // Total number of allocations
    memory_size total_deallocations;  // Total number of deallocations
};

AllocationTrackerStats GetAllocationTrackerStats() noexcept;
void ResetAllocationTrackerStats() noexcept;

// ----------------------------------------------------------------------------
// Per-tag statistics
// ----------------------------------------------------------------------------

struct TagStats {
    memory_tag tag;                   // Tag identifier
    memory_size current_allocated;    // Currently allocated bytes for this tag
    memory_size peak_allocated;       // Peak allocated bytes for this tag
    memory_size alloc_count;          // Number of allocations with this tag
    memory_size dealloc_count;        // Number of deallocations with this tag
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

