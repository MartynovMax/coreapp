#pragma once

// =============================================================================
// arena.hpp
// Unified arena interface for linear/stack-style memory allocation.
// =============================================================================

#include "core_memory.hpp"
#include "memory_traits.hpp"

namespace core {

// ----------------------------------------------------------------------------
// ArenaMarker - Checkpoint for arena state
// ----------------------------------------------------------------------------

struct ArenaMarker {
    void* internal_state = nullptr;
    
#if CORE_MEMORY_DEBUG
    u32 debug_arena_id = 0;
    u32 debug_sequence = 0;
#endif
    
    bool operator==(const ArenaMarker& other) const noexcept {
        return internal_state == other.internal_state;
    }
    
    bool operator!=(const ArenaMarker& other) const noexcept {
        return internal_state != other.internal_state;
    }
};

static_assert(sizeof(ArenaMarker) <= 16, "ArenaMarker should be small");
static_assert(is_trivially_copyable_v<ArenaMarker>(), "ArenaMarker must be trivially copyable");

} // namespace core

