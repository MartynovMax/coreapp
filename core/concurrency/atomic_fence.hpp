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
// -----------------------------------------------------------------------------

#if CORE_COMPILER_MSVC

/// Establishes memory synchronization ordering between threads.
inline void thread_fence(memory_order order) noexcept {
    switch (order) {
    case memory_order::relaxed:
        // No fence needed for relaxed
        break;
    case memory_order::acquire:
    case memory_order::release:
    case memory_order::acq_rel:
    case memory_order::seq_cst:
        // Compiler barrier to prevent reordering
        _ReadWriteBarrier();
        
        // Hardware memory barrier using a dummy interlocked operation
        // This ensures visibility across CPU cores
        {
            static volatile long dummy = 0;
            _InterlockedOr(&dummy, 0);
        }
        break;
    }
}

#endif  // CORE_COMPILER_MSVC

} // namespace core

