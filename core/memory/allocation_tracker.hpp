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

} // namespace core

