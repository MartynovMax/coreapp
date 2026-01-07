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

// -----------------------------------------------------------------------------
// Compile-time configuration checks
// -----------------------------------------------------------------------------

// Verify atomics are available
static_assert(CORE_HAS_ATOMICS, "CORE_HAS_ATOMICS must be 1");

// Verify memory model support
static_assert(CORE_HAS_MEMORY_MODEL, "CORE_HAS_MEMORY_MODEL must be 1");

// Verify supported compiler
static_assert(CORE_COMPILER_MSVC || CORE_COMPILER_GCC || CORE_COMPILER_CLANG,
              "Atomic intrinsics require MSVC, GCC, or Clang");

// Verify u32/u64 sizes match expectations
static_assert(sizeof(u32) == 4, "u32 must be 4 bytes for atomic operations");
static_assert(sizeof(u64) == 8, "u64 must be 8 bytes for atomic operations");
static_assert(sizeof(void*) == sizeof(usize), "pointer size must match usize");

// Verify 64-bit atomics on 64-bit platforms
#if CORE_64BIT
static_assert(CORE_HAS_64BIT_ATOMICS, 
              "64-bit atomics must be available on 64-bit platforms");
#endif

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

} // namespace detail
} // namespace core

