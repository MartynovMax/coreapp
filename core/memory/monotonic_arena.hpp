#pragma once

// =============================================================================
// monotonic_arena.hpp
// Alias for BumpArena emphasizing monotonic growth semantics.
//
// MonotonicArena and BumpArena refer to the same class:
//   - "Bump" emphasizes implementation (bump pointer forward)
//   - "Monotonic" emphasizes semantics (memory usage only grows)
//
// Both terms are used interchangeably in systems programming. Use whichever
// better communicates intent in your context.
// =============================================================================

#include "bump_arena.hpp"

namespace core {
    
using MonotonicArena = BumpArena;

} // namespace core

