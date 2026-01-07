#pragma once

// =============================================================================
// atomic_uintptr.hpp
// Pointer-sized unsigned atomic integer type.
//
// Atomic integer with the same size as a pointer (usize).
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_types.hpp"
#include "../base/core_platform.hpp"
#include "atomic_u32.hpp"
#if CORE_64BIT
#include "atomic_u64.hpp"
#endif

namespace core {

// atomic_uintptr is a type alias to atomic_u32 or atomic_u64
// depending on pointer size.
#if CORE_64BIT
using atomic_uintptr = atomic_u64;
#else
using atomic_uintptr = atomic_u32;
#endif

} // namespace core

