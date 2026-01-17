#include "thread_local_arena.hpp"
#include "bump_allocator.hpp"
#include "system_allocator.hpp"
#include "core_memory.hpp"

namespace core {
namespace detail {

// =============================================================================
// ThreadLocalArenaState - Per-thread arena storage
// =============================================================================

struct ThreadLocalArenaState {
    // Arena instance storage (placement new/delete)
    alignas(BumpAllocator) u8 _arena_storage[sizeof(BumpAllocator)];
    
    // Name buffer for debug/introspection
    char _name_buffer[64];
    
    // Initialization flag
    bool _initialized = false;
    
    // Get arena pointer (nullptr if not initialized)
    IArena* GetArena() noexcept {
        if (!_initialized) {
            return nullptr;
        }
        return reinterpret_cast<IArena*>(_arena_storage);
    }
    
    // Get arena pointer (const version)
    const IArena* GetArena() const noexcept {
        if (!_initialized) {
            return nullptr;
        }
        return reinterpret_cast<const IArena*>(_arena_storage);
    }
};

// Thread-local storage - each thread gets its own instance
inline thread_local ThreadLocalArenaState tlsArenaState;

} // namespace detail

IArena& GetThreadLocalArena() noexcept {
    IArena* arena = detail::tlsArenaState.GetArena();
    CORE_MEM_ASSERT(arena && "Arena not initialized");
    return *arena;
}

bool HasThreadLocalArena() noexcept {
    return detail::tlsArenaState._initialized;
}

void ResetThreadLocalArena() noexcept {
    IArena* arena = detail::tlsArenaState.GetArena();
    if (arena) {
        arena->Reset();
    }
}

} // namespace core

