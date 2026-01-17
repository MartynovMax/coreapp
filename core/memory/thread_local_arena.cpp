#include "thread_local_arena.hpp"
#include "bump_arena.hpp"
#include "system_allocator.hpp"
#include "core_memory.hpp"
#include <new>

namespace core {
namespace detail {

constexpr memory_size kDefaultThreadArenaCapacity = 1024 * 1024;

struct ThreadLocalArenaState {
    alignas(BumpArena) u8 _arenaStorage[sizeof(BumpArena)];
    char _nameBuffer[64];
    bool _initialized = false;
    
    IArena* GetArena() noexcept {
        if (!_initialized) {
            return nullptr;
        }
        return reinterpret_cast<IArena*>(_arenaStorage);
    }
    
    const IArena* GetArena() const noexcept {
        if (!_initialized) {
            return nullptr;
        }
        return reinterpret_cast<const IArena*>(_arenaStorage);
    }
    
    void CreateArena() noexcept {
        CORE_MEM_ASSERT(!_initialized && "Arena already initialized");
        
        IAllocator& upstream = SystemAllocator::Instance();
        void* storage = static_cast<void*>(_arenaStorage);
        new (storage) BumpArena(kDefaultThreadArenaCapacity, upstream);
        
        _initialized = true;
    }
    
    void DestroyArena() noexcept {
        if (!_initialized) {
            return;
        }
        
        BumpArena* arena = reinterpret_cast<BumpArena*>(_arenaStorage);
        arena->~BumpArena();
        _initialized = false;
    }
    
    ~ThreadLocalArenaState() noexcept {
        DestroyArena();
    }
};

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

