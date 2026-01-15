// tests/core_memory/test_helpers.hpp
// Common test utilities for memory subsystem tests

#pragma once

#include "core/memory/arena.hpp"
#include "core/memory/core_memory.hpp"
#include "core/memory/memory_ops.hpp"

namespace core::test {

// =============================================================================
// SimpleTestArena - Minimal arena implementation for testing
// =============================================================================
//
// Lightweight bump allocator arena for validating IArena interface behavior.
// Supports markers and debug validation.

class SimpleTestArena final : public IArena {
public:
    explicit SimpleTestArena(void* buffer, memory_size size) noexcept
        : _begin(static_cast<u8*>(buffer))
        , _current(static_cast<u8*>(buffer))
        , _end(static_cast<u8*>(buffer) + size)
        , _name("SimpleTestArena")
#if CORE_MEMORY_DEBUG
        , _arenaId(GenerateArenaId())
        , _generation(0)
#endif
    {}
    
    void* Allocate(memory_size size, memory_alignment alignment = CORE_DEFAULT_ALIGNMENT) noexcept override {
        if (size == 0 || _begin == nullptr) {
            return nullptr;
        }
        
        u8* aligned = AlignPtrUp(_current, alignment);
        if (aligned < _current || aligned > _end) {
            return nullptr;
        }
        
        memory_size available = static_cast<memory_size>(_end - aligned);
        if (size > available) {
            return nullptr;
        }
        
        _current = aligned + size;
        return aligned;
    }
    
    void Reset() noexcept override {
        _current = _begin;
#if CORE_MEMORY_DEBUG
        ++_generation; // Invalidate all previous markers
#endif
    }
    
    ArenaMarker GetMarker() const noexcept override {
        ArenaMarker marker;
        marker.internal_state = _current;
#if CORE_MEMORY_DEBUG
        marker.debug_arena_id = _arenaId;
        marker.debug_sequence = _generation;
#endif
        return marker;
    }
    
    void RewindTo(ArenaMarker marker) noexcept override {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(marker.debug_arena_id == _arenaId && "Marker from different arena");
        CORE_MEM_ASSERT(marker.debug_sequence == _generation && "Stale marker (from before Reset)");
        CORE_MEM_ASSERT(static_cast<u8*>(marker.internal_state) >= _begin &&
                       static_cast<u8*>(marker.internal_state) <= _current &&
                       "Invalid marker position");
#endif
        _current = static_cast<u8*>(marker.internal_state);
    }
    
    memory_size Capacity() const noexcept override {
        if (_begin == nullptr) return 0;
        return static_cast<memory_size>(_end - _begin);
    }
    
    memory_size Used() const noexcept override {
        if (_begin == nullptr) return 0;
        return static_cast<memory_size>(_current - _begin);
    }
    
    memory_size Remaining() const noexcept override {
        if (_begin == nullptr) return 0;
        return static_cast<memory_size>(_end - _current);
    }
    
    const char* Name() const noexcept override {
        return _name;
    }
    
    bool Owns(const void* ptr) const noexcept override {
        if (ptr == nullptr || _begin == nullptr) return false;
        const u8* p = static_cast<const u8*>(ptr);
        return p >= _begin && p < _end;
    }

private:
    u8* _begin;
    u8* _current;
    u8* _end;
    const char* _name;
    
#if CORE_MEMORY_DEBUG
    u32 _arenaId;
    u32 _generation;
    
    static u32 GenerateArenaId() {
        static u32 nextId = 1;
        return nextId++;
    }
#endif
};

} // namespace core::test

