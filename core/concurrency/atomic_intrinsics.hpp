#pragma once

// =============================================================================
// atomic_intrinsics.hpp
// Platform-specific atomic intrinsics wrapper.
//
// Provides unified interface to compiler-specific atomic operations:
//   - MSVC: _Interlocked* family
//   - GCC/Clang: __atomic_* builtins
// =============================================================================

#include "../base/core_platform.hpp"
#include "../base/core_features.hpp"
#include "../base/core_types.hpp"
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

// -----------------------------------------------------------------------------
// 32-bit atomic operations
// -----------------------------------------------------------------------------

/// Atomically load a 32-bit unsigned value.
inline u32 atomic_load_u32(const volatile u32* ptr, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    u32 value = *ptr;
    switch (order) {
    case memory_order::relaxed:
        return value;
    case memory_order::acquire:
    case memory_order::seq_cst:
        _ReadWriteBarrier();
        return value;
    default:
        return value;
    }
#else
    return __atomic_load_n(ptr, to_gcc_memory_order(order));
#endif
}

/// Atomically store a 32-bit unsigned value.
inline void atomic_store_u32(volatile u32* ptr, u32 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    switch (order) {
    case memory_order::relaxed:
        *ptr = value;
        break;
    case memory_order::release:
    case memory_order::seq_cst:
        _ReadWriteBarrier();
        *ptr = value;
        break;
    default:
        *ptr = value;
        break;
    }
#else
    __atomic_store_n(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically exchange a 32-bit unsigned value and return the old value.
inline u32 atomic_exchange_u32(volatile u32* ptr, u32 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;  // MSVC InterlockedExchange is always seq_cst
    return static_cast<u32>(_InterlockedExchange(
        reinterpret_cast<volatile long*>(ptr),
        static_cast<long>(value)
    ));
#else
    return __atomic_exchange_n(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically compare and exchange a 32-bit unsigned value (strong version).
inline bool atomic_compare_exchange_strong_u32(
    volatile u32* ptr,
    u32* expected,
    u32 desired,
    memory_order success,
    memory_order failure) noexcept {
#if CORE_COMPILER_MSVC
    (void)success; (void)failure;
    u32 old = static_cast<u32>(_InterlockedCompareExchange(
        reinterpret_cast<volatile long*>(ptr),
        static_cast<long>(desired),
        static_cast<long>(*expected)
    ));
    if (old == *expected) {
        return true;
    }
    *expected = old;
    return false;
#else
    return __atomic_compare_exchange_n(
        ptr, expected, desired,
        false,  // strong
        to_gcc_memory_order(success),
        to_gcc_memory_order(failure)
    );
#endif
}

/// Atomically compare and exchange a 32-bit unsigned value (weak version).
inline bool atomic_compare_exchange_weak_u32(
    volatile u32* ptr,
    u32* expected,
    u32 desired,
    memory_order success,
    memory_order failure) noexcept {
#if CORE_COMPILER_MSVC
    // MSVC doesn't have weak CAS, use strong
    return atomic_compare_exchange_strong_u32(ptr, expected, desired, success, failure);
#else
    return __atomic_compare_exchange_n(
        ptr, expected, desired,
        true,  // weak
        to_gcc_memory_order(success),
        to_gcc_memory_order(failure)
    );
#endif
}

} // namespace detail
} // namespace core

