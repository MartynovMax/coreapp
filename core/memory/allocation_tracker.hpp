#pragma once

// =============================================================================
// allocation_tracker.hpp
// Global allocation tracking API for observing memory allocations.
//
// Thread-safety:
//   This implementation is NOT fully thread-safe. It is intended for
//   single-threaded use or environments where allocations are externally
//   synchronized. Full thread-safe implementation will be added in epic #95
//   (Concurrency-aware Allocators).
//
//   Current limitations:
//   - Statistics updates may race under concurrent allocations
//   - Listener registration/unregistration should not occur during allocations
//
// Usage:
//   1. Call InitializeAllocationTracker() at startup
//   2. Register listeners via RegisterAllocationListener()
//   3. Query statistics via GetAllocationTrackerStats()
//   4. Call ShutdownAllocationTracker() at shutdown
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
// WARNING: Listeners MUST NOT perform allocations to avoid recursion
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

