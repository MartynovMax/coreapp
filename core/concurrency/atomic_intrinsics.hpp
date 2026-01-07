#pragma once

// =============================================================================
// atomic_intrinsics.hpp
// Platform-specific atomic intrinsics wrapper.
//
// Provides unified interface to compiler-specific atomic operations:
//   - MSVC: _Interlocked* family
//   - GCC/Clang: __atomic_* builtins
// =============================================================================

#include "core/base/core_platform.hpp"
#include "core/base/core_features.hpp"
#include "core/base/core_types.hpp"
#include "memory_order.hpp"

#if !CORE_HAS_ATOMICS
#error "Atomic operations are not available on this platform (CORE_HAS_ATOMICS == 0)"
#endif

// Platform-specific headers
#if CORE_COMPILER_MSVC
#include <intrin.h>
#endif

namespace core {
namespace detail {


} // namespace detail
} // namespace core

