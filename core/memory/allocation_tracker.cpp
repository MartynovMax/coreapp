#include "allocation_tracker.hpp"
#include "../base/core_assert.hpp"

namespace core {

namespace detail {

struct ListenerEntry {
    AllocationListenerFn callback = nullptr;
    void* userData = nullptr;
    u32 handle = 0;  // 0 = unused slot
};

ListenerEntry _listeners[CORE_ALLOCATION_TRACKER_MAX_LISTENERS];
u32 _nextHandle = 1;

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

} // namespace core

