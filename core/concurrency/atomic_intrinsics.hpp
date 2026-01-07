#pragma once

// =============================================================================
// atomic_intrinsics.hpp
// Platform-specific atomic intrinsics wrapper.
//
// Provides unified interface to compiler-specific atomic operations:
//   - MSVC: _Interlocked* family (all operations are effectively seq_cst)
//   - GCC/Clang: __atomic_* builtins (supports all memory orders)
//
// Note: On MSVC, the memory_order parameter is accepted for API compatibility
// but all atomic operations behave as seq_cst for correctness and portability.
// This ensures proper synchronization on all architectures including ARM.
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
// Storage types for atomic operations
// -----------------------------------------------------------------------------

// On MSVC, we use native interlocked types to avoid aliasing issues.
// On GCC/Clang, we use our standard unsigned types.
#if CORE_COMPILER_MSVC
using atomic_u32_storage_t = long;
using atomic_u64_storage_t = long long;
#else
using atomic_u32_storage_t = u32;
using atomic_u64_storage_t = u64;
#endif

static_assert(sizeof(atomic_u32_storage_t) == 4, "atomic_u32_storage_t must be 4 bytes");
static_assert(sizeof(atomic_u64_storage_t) == 8, "atomic_u64_storage_t must be 8 bytes");

// -----------------------------------------------------------------------------
// 32-bit atomic operations
// -----------------------------------------------------------------------------

/// Atomically load a 32-bit unsigned value.
inline u32 atomic_load_u32(const volatile atomic_u32_storage_t* ptr, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u32>(_InterlockedOr(const_cast<volatile long*>(ptr), 0));
#else
    return __atomic_load_n(ptr, to_gcc_memory_order(order));
#endif
}

/// Atomically store a 32-bit unsigned value.
inline void atomic_store_u32(volatile atomic_u32_storage_t* ptr, u32 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    _InterlockedExchange(ptr, static_cast<long>(value));
#else
    __atomic_store_n(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically exchange a 32-bit unsigned value and return the old value.
inline u32 atomic_exchange_u32(volatile atomic_u32_storage_t* ptr, u32 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u32>(_InterlockedExchange(ptr, static_cast<long>(value)));
#else
    return __atomic_exchange_n(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically compare and exchange a 32-bit unsigned value (strong version).
inline bool atomic_compare_exchange_strong_u32(
    volatile atomic_u32_storage_t* ptr,
    u32* expected,
    u32 desired,
    memory_order success,
    memory_order failure) noexcept {
#if CORE_COMPILER_MSVC
    (void)success; (void)failure;
    u32 old = static_cast<u32>(_InterlockedCompareExchange(
        ptr,
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
        false,
        to_gcc_memory_order(success),
        to_gcc_memory_order(failure)
    );
#endif
}

/// Atomically compare and exchange a 32-bit unsigned value (weak version).
inline bool atomic_compare_exchange_weak_u32(
    volatile atomic_u32_storage_t* ptr,
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

/// Atomically add a value to a 32-bit unsigned integer and return the old value.
inline u32 atomic_fetch_add_u32(volatile atomic_u32_storage_t* ptr, u32 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u32>(_InterlockedExchangeAdd(ptr, static_cast<long>(value)));
#else
    return __atomic_fetch_add(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically subtract a value from a 32-bit unsigned integer and return the old value.
inline u32 atomic_fetch_sub_u32(volatile atomic_u32_storage_t* ptr, u32 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u32>(_InterlockedExchangeAdd(ptr, -static_cast<long>(value)));
#else
    return __atomic_fetch_sub(ptr, value, to_gcc_memory_order(order));
#endif
}

// -----------------------------------------------------------------------------
// 64-bit atomic operations
// -----------------------------------------------------------------------------

#if CORE_HAS_64BIT_ATOMICS

/// Atomically load a 64-bit unsigned value.
inline u64 atomic_load_u64(const volatile atomic_u64_storage_t* ptr, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u64>(_InterlockedOr64(const_cast<volatile long long*>(ptr), 0));
#else
    return __atomic_load_n(ptr, to_gcc_memory_order(order));
#endif
}

/// Atomically store a 64-bit unsigned value.
inline void atomic_store_u64(volatile atomic_u64_storage_t* ptr, u64 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    _InterlockedExchange64(ptr, static_cast<long long>(value));
#else
    __atomic_store_n(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically exchange a 64-bit unsigned value and return the old value.
inline u64 atomic_exchange_u64(volatile atomic_u64_storage_t* ptr, u64 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u64>(_InterlockedExchange64(ptr, static_cast<long long>(value)));
#else
    return __atomic_exchange_n(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically compare and exchange a 64-bit unsigned value (strong version).
inline bool atomic_compare_exchange_strong_u64(
    volatile atomic_u64_storage_t* ptr,
    u64* expected,
    u64 desired,
    memory_order success,
    memory_order failure) noexcept {
#if CORE_COMPILER_MSVC
    (void)success; (void)failure;
    u64 old = static_cast<u64>(_InterlockedCompareExchange64(
        ptr,
        static_cast<long long>(desired),
        static_cast<long long>(*expected)
    ));
    if (old == *expected) {
        return true;
    }
    *expected = old;
    return false;
#else
    return __atomic_compare_exchange_n(
        ptr, expected, desired,
        false,
        to_gcc_memory_order(success),
        to_gcc_memory_order(failure)
    );
#endif
}

/// Atomically compare and exchange a 64-bit unsigned value (weak version).
inline bool atomic_compare_exchange_weak_u64(
    volatile atomic_u64_storage_t* ptr,
    u64* expected,
    u64 desired,
    memory_order success,
    memory_order failure) noexcept {
#if CORE_COMPILER_MSVC
    return atomic_compare_exchange_strong_u64(ptr, expected, desired, success, failure);
#else
    return __atomic_compare_exchange_n(
        ptr, expected, desired,
        true,
        to_gcc_memory_order(success),
        to_gcc_memory_order(failure)
    );
#endif
}

/// Atomically add a value to a 64-bit unsigned integer and return the old value.
inline u64 atomic_fetch_add_u64(volatile atomic_u64_storage_t* ptr, u64 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u64>(_InterlockedExchangeAdd64(ptr, static_cast<long long>(value)));
#else
    return __atomic_fetch_add(ptr, value, to_gcc_memory_order(order));
#endif
}

/// Atomically subtract a value from a 64-bit unsigned integer and return the old value.
inline u64 atomic_fetch_sub_u64(volatile atomic_u64_storage_t* ptr, u64 value, memory_order order) noexcept {
#if CORE_COMPILER_MSVC
    (void)order;
    return static_cast<u64>(_InterlockedExchangeAdd64(ptr, -static_cast<long long>(value)));
#else
    return __atomic_fetch_sub(ptr, value, to_gcc_memory_order(order));
#endif
}

#endif  // CORE_HAS_64BIT_ATOMICS

} // namespace detail
} // namespace core

