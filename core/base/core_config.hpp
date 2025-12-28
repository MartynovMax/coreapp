#pragma once

// ============================================================================
// core_config.hpp
// Unified compile-time configuration for Core module.
//
// This header is a *policy/config* layer:
//   - build-mode flags (CORE_DEBUG, CORE_ASSERTIONS_ENABLED, ...)
//   - portable low-level annotations used across Core
//
// ============================================================================

#include "core_inline.hpp"
#include "core_platform.hpp"
#include "core_preprocessor.hpp"

// ----------------------------------------------------------------------------
// C++ standard check (C++20 required)
// ----------------------------------------------------------------------------

#if !CORE_CPP20_OR_GREATER
#error "CoreApp requires at least C++20 (/std:c++20 or -std=c++20)."
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
#define CORE_ALIGN(N) __declspec(align(N))
#elif CORE_COMPILER_GCC || CORE_COMPILER_CLANG
#define CORE_ALIGN(N) __attribute__((aligned(N)))
#else
#define CORE_ALIGN(N) alignas(N)
#endif

#define CORE_UNUSED(x) ((void)(x))

// ----------------------------------------------------------------------------
// Branch prediction helpers
// ----------------------------------------------------------------------------

#if (CORE_COMPILER_CLANG || CORE_COMPILER_GCC) &&                              \
    CORE_HAS_BUILTIN(__builtin_expect)
#define CORE_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define CORE_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define CORE_LIKELY(expr) (expr)
#define CORE_UNLIKELY(expr) (expr)
#endif

// ----------------------------------------------------------------------------
// Function / result attributes
// ----------------------------------------------------------------------------

// NODISCARD
#if CORE_HAS_NODISCARD
#define CORE_NODISCARD [[nodiscard]]
#elif (CORE_COMPILER_CLANG || CORE_COMPILER_GCC)
#define CORE_NODISCARD __attribute__((warn_unused_result))
#else
#define CORE_NODISCARD
#endif

// NORETURN
#if CORE_HAS_NORETURN
#define CORE_NORETURN [[noreturn]]
#elif CORE_COMPILER_MSVC
#define CORE_NORETURN __declspec(noreturn)
#elif (CORE_COMPILER_CLANG || CORE_COMPILER_GCC)
#define CORE_NORETURN __attribute__((noreturn))
#else
#define CORE_NORETURN
#endif

// ----------------------------------------------------------------------------
// Fallthrough annotation
// ----------------------------------------------------------------------------

#if CORE_HAS_FALLTHROUGH
#define CORE_FALLTHROUGH [[fallthrough]]
#elif (CORE_COMPILER_CLANG || CORE_COMPILER_GCC)
#define CORE_FALLTHROUGH __attribute__((fallthrough))
#else
#define CORE_FALLTHROUGH
#endif

// ----------------------------------------------------------------------------
// Deprecation helpers
// ----------------------------------------------------------------------------

#if CORE_HAS_DEPRECATED
#define CORE_DEPRECATED [[deprecated]]
#else
#if CORE_COMPILER_MSVC
#define CORE_DEPRECATED __declspec(deprecated)
#elif (CORE_COMPILER_CLANG || CORE_COMPILER_GCC)
#define CORE_DEPRECATED __attribute__((deprecated))
#else
#define CORE_DEPRECATED
#endif
#endif

#if (CORE_HAS_CPP_ATTRIBUTE(deprecated) != 0) && CORE_CPP14_OR_GREATER
#define CORE_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#else
#if CORE_COMPILER_MSVC
#define CORE_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#elif (CORE_COMPILER_CLANG || CORE_COMPILER_GCC)
#define CORE_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#else
#define CORE_DEPRECATED_MSG(msg)
#endif
#endif

// ----------------------------------------------------------------------------
// Assumption helper
// ----------------------------------------------------------------------------

#if CORE_COMPILER_MSVC
#define CORE_ASSUME(expr) __assume(expr)

#elif CORE_COMPILER_CLANG
#if CORE_HAS_BUILTIN(__builtin_assume)
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
