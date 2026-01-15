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

} // namespace core

