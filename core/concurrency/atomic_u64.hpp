#pragma once

// =============================================================================
// atomic_u64.hpp
// 64-bit unsigned atomic integer type.
//
// Only available on platforms with 64-bit atomic support
// (CORE_HAS_64BIT_ATOMICS == 1).
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_features.hpp"
#include "../base/core_types.hpp"
#include "atomic_intrinsics.hpp"
#include "memory_order.hpp"

#if CORE_HAS_64BIT_ATOMICS

namespace core {

} // namespace core

#endif  // CORE_HAS_64BIT_ATOMICS

