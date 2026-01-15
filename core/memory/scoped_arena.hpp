#pragma once

// =============================================================================
// scoped_arena.hpp
// RAII helper for automatic arena state management.
//
// ScopedArena captures an ArenaMarker on construction and automatically
// rewinds the arena to that marker on destruction, providing exception-safe
// temporary allocations within a C++ scope.
//
// Supports nested scopes: multiple ScopedArena instances can operate on the
// same arena, rewinding in LIFO order. Memory allocated in a scope is
// invalidated when that scope exits.
//
// Move semantics: ScopedArena is movable but not copyable. Moving transfers
// ownership of the marker; the moved-from object will not perform rewind.
// =============================================================================

#include "arena.hpp"
#include "core_memory.hpp"

namespace core {

class ScopedArena final {
public:
    // Constructor - captures arena state on construction
    explicit ScopedArena(IArena& arena) noexcept
        : _arena(&arena)
        , _marker(arena.GetMarker())
    {
    }
    
    // Destructor - rewinds arena to captured marker
    ~ScopedArena() noexcept {
        if (_arena) {
            _arena->RewindTo(_marker);
        }
    }
    
    // Non-copyable
    ScopedArena(const ScopedArena& other) = delete;
    ScopedArena& operator=(const ScopedArena& other) = delete;
    
    // Movable - transfers ownership of marker
    ScopedArena(ScopedArena&& other) noexcept
        : _arena(other._arena)
        , _marker(other._marker)
    {
        other._arena = nullptr;
    }
    
    ScopedArena& operator=(ScopedArena&& other) noexcept {
        if (this != &other) {
            if (_arena) {
                _arena->RewindTo(_marker);
            }
            
            _arena = other._arena;
            _marker = other._marker;
            other._arena = nullptr;
        }
        return *this;
    }
    
    // Check if this ScopedArena is valid (not moved-from)
    bool IsValid() const noexcept {
        return _arena != nullptr;
    }
    
    // Arena access
    IArena& GetArena() const noexcept {
        CORE_MEM_ASSERT(_arena && "Cannot use moved-from ScopedArena");
        return *_arena;
    }
    
    // Allocation helper
    void* Allocate(memory_size size, memory_alignment alignment = CORE_DEFAULT_ALIGNMENT) noexcept {
        CORE_MEM_ASSERT(_arena && "Cannot use moved-from ScopedArena");
        return _arena->Allocate(size, alignment);
    }

private:
    IArena* _arena = nullptr;
    ArenaMarker _marker{};
};

} // namespace core

