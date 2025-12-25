#pragma once

// ============================================================================
// core_config.hpp
// Unified compile-time configuration for Core module.
// - Compiler detection
// - Platform detection
// - CPU architecture
// - Endianness
// - Global config flags
// - Utility macros
// ============================================================================

#include "core_platform.hpp"

// ----------------------------------------------------------------------------
// C++ standard check (C++20 required)
// ----------------------------------------------------------------------------

#if defined(_MSVC_LANG) && !defined(__clang__)
#if _MSVC_LANG < 202002L
#error "CoreApp requires at least C++20 (/std:c++20)."
#endif
#else
#if __cplusplus < 202002L
#error "CoreApp requires at least C++20 (-std=c++20)."
#endif
#endif

// ----------------------------------------------------------------------------
// Global configuration flags
// ----------------------------------------------------------------------------

// Allow manual override from build system
#ifndef CORE_DEBUG
#if defined(_DEBUG) || !defined(NDEBUG)
#define CORE_DEBUG 1
#else
#define CORE_DEBUG 0
#endif
#endif

#ifndef CORE_ASSERTIONS_ENABLED
#define CORE_ASSERTIONS_ENABLED CORE_DEBUG
#endif

#ifndef CORE_ENABLE_LOGS
#define CORE_ENABLE_LOGS CORE_DEBUG
#endif

// ----------------------------------------------------------------------------
// Utility macros
// ----------------------------------------------------------------------------

#if CORE_COMPILER_MSVC

#define CORE_FORCE_INLINE __forceinline
#define CORE_NO_INLINE __declspec(noinline)

#define CORE_ALIGN(N) __declspec(align(N))

#elif CORE_COMPILER_GCC || CORE_COMPILER_CLANG

#define CORE_FORCE_INLINE inline __attribute__((always_inline))
#define CORE_NO_INLINE __attribute__((noinline))

#define CORE_ALIGN(N) __attribute__((aligned(N)))

#else

#define CORE_FORCE_INLINE inline
#define CORE_NO_INLINE
#define CORE_ALIGN(N) alignas(N)

#endif

#define CORE_UNUSED(x) ((void)(x))

// ----------------------------------------------------------------------------
// Branch prediction helpers
// ----------------------------------------------------------------------------

#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define CORE_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define CORE_LIKELY(expr) (expr)
#define CORE_UNLIKELY(expr) (expr)
#endif

// ----------------------------------------------------------------------------
// Function / result attributes
// ----------------------------------------------------------------------------

#if defined(__cplusplus)
#define CORE_NODISCARD [[nodiscard]]
#define CORE_NORETURN [[noreturn]]
#else
#if CORE_COMPILER_MSVC
#define CORE_NODISCARD
#define CORE_NORETURN __declspec(noreturn)
#elif CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_NODISCARD __attribute__((warn_unused_result))
#define CORE_NORETURN __attribute__((noreturn))
#else
#define CORE_NODISCARD
#define CORE_NORETURN
#endif
#endif

// ----------------------------------------------------------------------------
// Fallthrough annotation
// ----------------------------------------------------------------------------

#if defined(__cplusplus)
#define CORE_FALLTHROUGH [[fallthrough]]
#else
#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_FALLTHROUGH __attribute__((fallthrough))
#else
#define CORE_FALLTHROUGH
#endif
#endif

// ----------------------------------------------------------------------------
// Deprecation helpers
// ----------------------------------------------------------------------------

#if defined(__cplusplus)
#define CORE_DEPRECATED [[deprecated]]
#define CORE_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#else
#if CORE_COMPILER_MSVC
#define CORE_DEPRECATED __declspec(deprecated)
#define CORE_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#elif CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_DEPRECATED __attribute__((deprecated))
#define CORE_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#else
#define CORE_DEPRECATED
#define CORE_DEPRECATED_MSG(msg)
#endif
#endif

// ----------------------------------------------------------------------------
// Assumption helper
// ----------------------------------------------------------------------------

#if CORE_COMPILER_MSVC
#define CORE_ASSUME(expr) __assume(expr)

#elif CORE_COMPILER_CLANG
#if defined(__has_builtin) && __has_builtin(__builtin_assume)
#define CORE_ASSUME(expr) __builtin_assume(expr)
#else
#define CORE_ASSUME(expr)                                                      \
  do {                                                                         \
    if (!(expr))                                                               \
      __builtin_unreachable();                                                 \
  } while (0)
#endif

#elif CORE_COMPILER_GCC
#define CORE_ASSUME(expr)                                                      \
  do {                                                                         \
    if (!(expr))                                                               \
      __builtin_unreachable();                                                 \
  } while (0)

#else
#define CORE_ASSUME(expr) ((void)0)
#endif