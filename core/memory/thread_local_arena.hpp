#pragma once

// =============================================================================
// thread_local_arena.hpp
// Per-thread arena facility for fast scratch allocations.
//
// Provides each thread with its own arena instance, eliminating contention
// and synchronization overhead. Integrates with IArena and ScopedArena.
// =============================================================================

#include "arena.hpp"

namespace core {

// Get current thread's arena (lazy initialization)
CORE_API IArena& GetThreadLocalArena() noexcept;

// Check if current thread has initialized its arena
CORE_API bool HasThreadLocalArena() noexcept;

// Reset current thread's arena (no-op if not initialized)
CORE_API void ResetThreadLocalArena() noexcept;

} // namespace core

