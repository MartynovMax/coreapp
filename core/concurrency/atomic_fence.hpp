#pragma once

// =============================================================================
// atomic_fence.hpp
// Memory fence (barrier) operations.
//
// Provides thread_fence and signal_fence for explicit memory synchronization.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_platform.hpp"
#include "memory_order.hpp"

#if CORE_COMPILER_MSVC
#include <intrin.h>
#endif

namespace core {

// -----------------------------------------------------------------------------
// thread_fence: Full memory barrier between threads
//
// Synchronizes memory operations between threads by preventing the compiler
// and CPU from reordering memory accesses across the fence.
//
// - acquire: Prevents subsequent reads/writes from moving before the fence
// - release: Prevents prior reads/writes from moving after the fence
// - acq_rel: Combines both acquire and release semantics
// - seq_cst: Sequential consistency with full ordering guarantees
// - relaxed: No fence, no-op
//
// Use when synchronizing non-atomic variables with atomic flags.
// -----------------------------------------------------------------------------

#if CORE_COMPILER_MSVC

/// Establishes memory synchronization ordering between threads.
inline void thread_fence(memory_order order) noexcept {
    switch (order) {
    case memory_order::relaxed:
        break;
    case memory_order::acquire:
    case memory_order::release:
    case memory_order::acq_rel:
    case memory_order::seq_cst:
        _ReadWriteBarrier();
        {
            volatile long dummy = 0;
            _InterlockedOr(&dummy, 0);
        }
        break;
    }
}

#endif  // CORE_COMPILER_MSVC

#if CORE_COMPILER_GCC || CORE_COMPILER_CLANG

/// Establishes memory synchronization ordering between threads.
inline void thread_fence(memory_order order) noexcept {
    __atomic_thread_fence(to_gcc_memory_order(order));
}

#endif  // GCC/Clang

// -----------------------------------------------------------------------------
// signal_fence: Compiler-only barrier (no CPU fence)
//
// Prevents compiler reordering but does not emit hardware fence instructions.
// Used for signal handlers and single-threaded scenarios with external memory.
// -----------------------------------------------------------------------------

#if CORE_COMPILER_MSVC

/// Compiler-only memory ordering barrier.
inline void signal_fence(memory_order order) noexcept {
    switch (order) {
    case memory_order::relaxed:
        break;
    case memory_order::acquire:
    case memory_order::release:
    case memory_order::acq_rel:
    case memory_order::seq_cst:
        _ReadWriteBarrier();
        break;
    }
}

#endif  // CORE_COMPILER_MSVC

#if CORE_COMPILER_GCC || CORE_COMPILER_CLANG

/// Compiler-only memory ordering barrier.
inline void signal_fence(memory_order order) noexcept {
    __atomic_signal_fence(to_gcc_memory_order(order));
}

#endif  // GCC/Clang

} // namespace core

