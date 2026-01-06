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
//
// Example - Basic tracking:
//   InitializeAllocationTracker();
//   
//   // Allocate some memory
//   void* ptr = AllocateBytes(allocator, 1024, 16, 42);
//   
//   // Query statistics
//   auto stats = GetAllocationTrackerStats();
//   // stats.current_allocated == 1024
//   // stats.total_allocations == 1
//   
//   DeallocateBytes(allocator, ptr, 1024, 16, 42);
//   ShutdownAllocationTracker();
//
// Example - Custom listener:
//   void MyListener(AllocationEvent event, const IAllocator* allocator,
//                   const AllocationRequest* request, const AllocationInfo* info,
//                   void* userData) {
//       if (event == AllocationEvent::AllocateEnd && info) {
//           printf("Allocated %zu bytes at %p\n", info->size, info->ptr);
//       }
//   }
//   
//   InitializeAllocationTracker();
//   auto handle = RegisterAllocationListener(MyListener, nullptr);
//   // ... allocations will call MyListener ...
//   UnregisterAllocationListener(handle);
//   ShutdownAllocationTracker();
//
// Example - Per-tag tracking:
//   constexpr memory_tag RENDER_TAG = 1;
//   constexpr memory_tag PHYSICS_TAG = 2;
//   
//   InitializeAllocationTracker();
//   
//   void* renderMem = AllocateBytes(allocator, 2048, 16, RENDER_TAG);
//   void* physicsMem = AllocateBytes(allocator, 4096, 16, PHYSICS_TAG);
//   
//   TagStats renderStats;
//   if (GetTagStats(RENDER_TAG, renderStats)) {
//       // renderStats.current_allocated == 2048
//       // renderStats.alloc_count == 1
//   }
//   
//   // Enumerate all tracked tags
//   EnumerateTagStats([](const TagStats& stats, void*) {
//       printf("Tag %u: %zu bytes\n", stats.tag, stats.current_allocated);
//   }, nullptr);
//   
//   ShutdownAllocationTracker();
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
//
// Parameters:
//   event     - Type of allocation event (AllocateBegin/End, DeallocateBegin/End)
//   allocator - Allocator that performed the operation (may be nullptr)
//   request   - Allocation request (valid for Allocate* events, nullptr otherwise)
//   info      - Allocation info (nullptr for AllocateBegin, valid otherwise)
//   userData  - User data passed during registration
using AllocationListenerFn = void (*)(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info,
    void* userData
) noexcept;

// Register allocation listener
// Returns handle for later unregistration, or kInvalidListenerHandle if table is full
AllocationListenerHandle RegisterAllocationListener(
    AllocationListenerFn callback,
    void* userData = nullptr
) noexcept;

// Unregister allocation listener by handle
// Returns true if listener was found and removed
bool UnregisterAllocationListener(AllocationListenerHandle handle) noexcept;

// Clear all registered listeners
void ClearAllocationListeners() noexcept;

// ----------------------------------------------------------------------------
// Tracking control
// ----------------------------------------------------------------------------

// Enable allocation tracking (enabled by default after InitializeAllocationTracker)
void EnableAllocationTracking() noexcept;

// Disable allocation tracking (statistics and listeners won't be called)
void DisableAllocationTracking() noexcept;

// Check if tracking is currently enabled
bool IsAllocationTrackingEnabled() noexcept;

// ----------------------------------------------------------------------------
// Global statistics
// ----------------------------------------------------------------------------

// Global allocation statistics
struct AllocationTrackerStats {
    memory_size current_allocated;    // Currently allocated bytes
    memory_size peak_allocated;       // Peak allocated bytes (high water mark)
    memory_size total_allocations;    // Total number of allocations
    memory_size total_deallocations;  // Total number of deallocations
};

// Get current global statistics
AllocationTrackerStats GetAllocationTrackerStats() noexcept;

// Reset global statistics to zero
void ResetAllocationTrackerStats() noexcept;

// ----------------------------------------------------------------------------
// Per-tag statistics
// ----------------------------------------------------------------------------

// Per-tag allocation statistics
// Useful for tracking memory usage by subsystem (Render, Physics, Audio, etc.)
struct TagStats {
    memory_tag tag;                   // Tag identifier
    memory_size current_allocated;    // Currently allocated bytes for this tag
    memory_size peak_allocated;       // Peak allocated bytes for this tag
    memory_size alloc_count;          // Number of allocations with this tag
    memory_size dealloc_count;        // Number of deallocations with this tag
};

// Get statistics for a specific tag
// Returns true if tag has been used, false if not found
bool GetTagStats(memory_tag tag, TagStats& outStats) noexcept;

// Enumerate all tracked tags
// Callback is invoked once for each active tag
void EnumerateTagStats(
    void (*callback)(const TagStats& stats, void* user),
    void* userData = nullptr
) noexcept;

// Reset all per-tag statistics
void ResetTagStats() noexcept;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

// Initialize allocation tracker
// Registers tracker as a hook in the allocation system and enables tracking
// Call once at application startup (before any tracked allocations)
void InitializeAllocationTracker() noexcept;

// Shutdown allocation tracker
// Unregisters tracker hook and disables tracking
// Call at application shutdown (after all tracked allocations are freed)
void ShutdownAllocationTracker() noexcept;

} // namespace core

