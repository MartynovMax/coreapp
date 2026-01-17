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
        
        GenerateName();
        
        new (storage) BumpArena(kDefaultThreadArenaCapacity, upstream, _nameBuffer);
        
        _initialized = true;
    }
    
    void GenerateName() noexcept {
        uintptr_t threadId = reinterpret_cast<uintptr_t>(this);
        
        char* p = _nameBuffer;
        const char* prefix = "thread_arena_0x";
        while (*prefix) {
            *p++ = *prefix++;
        }
        
        for (int i = sizeof(uintptr_t) * 2 - 1; i >= 0; --i) {
            u32 nibble = (threadId >> (i * 4)) & 0xF;
            *p++ = nibble < 10 ? ('0' + nibble) : ('a' + nibble - 10);
        }
        *p = '\0';
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
    if (!detail::tlsArenaState._initialized) {
        detail::tlsArenaState.CreateArena();
    }
    
    IArena* arena = detail::tlsArenaState.GetArena();
    CORE_MEM_ASSERT(arena && "Arena creation failed");
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

void DestroyThreadLocalArena() noexcept {
    detail::tlsArenaState.DestroyArena();
}

} // namespace core

