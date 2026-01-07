#pragma once

// =============================================================================
// memory_order.hpp
// Memory ordering semantics for atomic operations.
//
// Provides explicit control over memory synchronization and ordering
// guarantees for atomic operations.
// =============================================================================

#include "../base/core_types.hpp"
#include "../base/core_platform.hpp"

namespace core {

/// Memory ordering for atomic operations.
enum class memory_order : u8 {
    relaxed = 0,  // No synchronization, only atomicity.
    acquire = 1,  // Prevents subsequent ops from moving before this load.
    release = 2,  // Prevents prior ops from moving after this store.
    acq_rel = 3,  // Both acquire and release (for read-modify-write).
    seq_cst = 4,  // Sequential consistency (strongest, default).
};

// -----------------------------------------------------------------------------
// GCC/Clang memory order conversion
// -----------------------------------------------------------------------------

#if CORE_COMPILER_GCC || CORE_COMPILER_CLANG

/// Convert core::memory_order to GCC/Clang __ATOMIC_* constants.
constexpr int to_gcc_memory_order(memory_order order) noexcept {
    switch (order) {
    case memory_order::relaxed: return __ATOMIC_RELAXED;
    case memory_order::acquire: return __ATOMIC_ACQUIRE;
    case memory_order::release: return __ATOMIC_RELEASE;
    case memory_order::acq_rel: return __ATOMIC_ACQ_REL;
    case memory_order::seq_cst: return __ATOMIC_SEQ_CST;
    }
    return __ATOMIC_SEQ_CST;  // Fallback to strongest ordering.
}

#endif  // GCC/Clang

} // namespace core

