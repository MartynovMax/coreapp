#pragma once

// =============================================================================
// memory_order.hpp
// Memory ordering semantics for atomic operations.
//
// Provides explicit control over memory synchronization and ordering
// guarantees for atomic operations.
// =============================================================================

#include "../base/core_types.hpp"

namespace core {

/// Memory ordering for atomic operations.
enum class memory_order : u8 {
    relaxed = 0,  // No synchronization, only atomicity.
    acquire = 1,  // Prevents subsequent ops from moving before this load.
    release = 2,  // Prevents prior ops from moving after this store.
    acq_rel = 3,  // Both acquire and release (for read-modify-write).
    seq_cst = 4,  // Sequential consistency (strongest, default).
};

} // namespace core

