#include "core_allocator.hpp"

namespace core {

namespace detail {

struct HookEntry {
    AllocationHookFn fn = nullptr;
    void* user = nullptr;
};

constexpr usize kMaxHooks = 8;

HookEntry _hooks[kMaxHooks];
u32 _hookCount = 0;

} // namespace detail

bool AddAllocationHook(AllocationHookFn fn, void* user) noexcept {
    if (!fn) {
        return false;
    }
    
    // Check if already registered
    for (u32 i = 0; i < detail::_hookCount; ++i) {
        if (detail::_hooks[i].fn == fn) {
            return false;
        }
    }
    
    // Find free slot
    if (detail::_hookCount < detail::kMaxHooks) {
        detail::_hooks[detail::_hookCount].fn = fn;
        detail::_hooks[detail::_hookCount].user = user;
        detail::_hookCount++;
        return true;
    }
    
    return false;
}

bool RemoveAllocationHook(AllocationHookFn fn) noexcept {
    if (!fn) {
        return false;
    }
    
    for (u32 i = 0; i < detail::_hookCount; ++i) {
        if (detail::_hooks[i].fn == fn) {
            // Shift remaining hooks
            for (u32 j = i; j < detail::_hookCount - 1; ++j) {
                detail::_hooks[j] = detail::_hooks[j + 1];
            }
            detail::_hookCount--;
            detail::_hooks[detail::_hookCount].fn = nullptr;
            detail::_hooks[detail::_hookCount].user = nullptr;
            return true;
        }
    }
    
    return false;
}

bool HasAllocationHooks() noexcept {
    return detail::_hookCount > 0;
}

void ClearAllocationHooks() noexcept {
    for (u32 i = 0; i < detail::_hookCount; ++i) {
        detail::_hooks[i].fn = nullptr;
        detail::_hooks[i].user = nullptr;
    }
    detail::_hookCount = 0;
}

} // namespace core

