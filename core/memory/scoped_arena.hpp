#pragma once

// =============================================================================
// scoped_arena.hpp
// RAII helper for automatic arena state management.
//
// ScopedArena captures an ArenaMarker on construction and automatically
// rewinds the arena to that marker on destruction, providing exception-safe
// temporary allocations within a C++ scope.
// =============================================================================

#include "arena.hpp"
#include "core_memory.hpp"

namespace core {

// ----------------------------------------------------------------------------
// ScopedArena - RAII arena state manager
// ----------------------------------------------------------------------------

class ScopedArena {
public:
    // Constructor - captures arena state on construction
    explicit ScopedArena(IArena& arena) noexcept
        : _arena(&arena)
        , _marker(arena.GetMarker())
    {
    }
    
    // Destructor - rewinds arena to captured marker
    ~ScopedArena() noexcept {
        _arena->RewindTo(_marker);
    }
    
    // Non-copyable
    ScopedArena(const ScopedArena& other) = delete;
    ScopedArena& operator=(const ScopedArena& other) = delete;
    
    // Non-movable
    ScopedArena(ScopedArena&& other) noexcept = delete;
    ScopedArena& operator=(ScopedArena&& other) noexcept = delete;
    
    // Arena access
    IArena& GetArena() const noexcept;
    
    // Allocation helper
    void* Allocate(memory_size size, memory_alignment alignment = CORE_DEFAULT_ALIGNMENT) noexcept;

private:
    IArena* _arena;
    ArenaMarker _marker;
};

} // namespace core

