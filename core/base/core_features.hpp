#pragma once

#include "core/base/core_platform.hpp"
#include "core/base/core_preprocessor.hpp"

// -----------------------------------------------------------------------------
// Helper: hosted vs freestanding
// -----------------------------------------------------------------------------

#ifndef CORE_STDC_HOSTED
#if defined(__STDC_HOSTED__)
#define CORE_STDC_HOSTED (__STDC_HOSTED__)
#else
#define CORE_STDC_HOSTED 1
#endif
#endif

// =============================================================================
// Language / runtime features
// =============================================================================

#ifndef CORE_HAS_EXCEPTIONS
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define CORE_HAS_EXCEPTIONS 1
#else
#define CORE_HAS_EXCEPTIONS 0
#endif
#endif

// RTTI (may be disabled via compiler flags).
#ifndef CORE_HAS_RTTI
#if defined(__cpp_rtti) || defined(__GXX_RTTI) || defined(_CPPRTTI)
#define CORE_HAS_RTTI 1
#else
#define CORE_HAS_RTTI 0
#endif
#endif

// thread_local storage.
#ifndef CORE_HAS_THREAD_LOCAL
#if defined(__cpp_thread_local)
#define CORE_HAS_THREAD_LOCAL 1
#elif CORE_CPP11_OR_GREATER
// Conservative assumption for mainstream desktop toolchains in C++11+ mode.
#define CORE_HAS_THREAD_LOCAL 1
#else
#define CORE_HAS_THREAD_LOCAL 0
#endif
#endif

// C++20 constexpr capabilities.
// (Project requires C++20, but we still keep explicit feature detection.)
#ifndef CORE_HAS_CONSTEXPR_20
#if defined(__cpp_constexpr)
// 201907L corresponds to C++20 constexpr.
#if (__cpp_constexpr >= 201907L)
#define CORE_HAS_CONSTEXPR_20 1
#else
#define CORE_HAS_CONSTEXPR_20 0
#endif
#elif CORE_CPP20_OR_GREATER
// Conservative: assume C++20 toolchains support the required constexpr set.
#define CORE_HAS_CONSTEXPR_20 1
#else
#define CORE_HAS_CONSTEXPR_20 0
#endif
#endif

// Inline variables (C++17).
#ifndef CORE_HAS_INLINE_VARIABLES
#if defined(__cpp_inline_variables)
#if (__cpp_inline_variables >= 201606L)
#define CORE_HAS_INLINE_VARIABLES 1
#else
#define CORE_HAS_INLINE_VARIABLES 0
#endif
#elif CORE_CPP17_OR_GREATER
#define CORE_HAS_INLINE_VARIABLES 1
#else
#define CORE_HAS_INLINE_VARIABLES 0
#endif
#endif

// =============================================================================
// Concurrency / atomics
// =============================================================================

#ifndef CORE_HAS_THREADS
#if (CORE_STDC_HOSTED == 0)
#define CORE_HAS_THREADS 0
#elif CORE_PLATFORM_DESKTOP
#define CORE_HAS_THREADS 1
#else
#define CORE_HAS_THREADS 0
#endif
#endif

#ifndef CORE_HAS_ATOMICS
#if (CORE_STDC_HOSTED == 0)
#define CORE_HAS_ATOMICS 0
#elif (CORE_COMPILER_MSVC || CORE_COMPILER_GCC || CORE_COMPILER_CLANG)
#define CORE_HAS_ATOMICS 1
#else
#define CORE_HAS_ATOMICS 0
#endif
#endif

// 64-bit atomic operations are typically available on 64-bit architectures.
#ifndef CORE_HAS_64BIT_ATOMICS
#if !CORE_HAS_ATOMICS
#define CORE_HAS_64BIT_ATOMICS 0
#elif (CORE_CPU_X64 || CORE_CPU_ARM64)
#define CORE_HAS_64BIT_ATOMICS 1
#else
// Conservative: do not assume 64-bit atomics on 32-bit targets.
#define CORE_HAS_64BIT_ATOMICS 0
#endif
#endif

// Memory model support (C++11+).
#ifndef CORE_HAS_MEMORY_MODEL
#if CORE_CPP11_OR_GREATER
#define CORE_HAS_MEMORY_MODEL 1
#else
#define CORE_HAS_MEMORY_MODEL 0
#endif
#endif

// Thread fences are part of the C++11 memory model; tie to atomics + memory model.
#ifndef CORE_HAS_THREAD_FENCE
#if CORE_HAS_MEMORY_MODEL && CORE_HAS_ATOMICS
#define CORE_HAS_THREAD_FENCE 1
#else
#define CORE_HAS_THREAD_FENCE 0
#endif
#endif

// =============================================================================
// SIMD / CPU features (compile-time only)
// =============================================================================

#ifndef CORE_HAS_SSE2
#if (CORE_CPU_X64)
// x64 ABI mandates SSE2.
#define CORE_HAS_SSE2 1
#elif defined(__SSE2__)
#define CORE_HAS_SSE2 1
#elif CORE_COMPILER_MSVC && defined(_M_IX86_FP) && (_M_IX86_FP >= 2)
#define CORE_HAS_SSE2 1
#else
#define CORE_HAS_SSE2 0
#endif
#endif

#ifndef CORE_HAS_SSE4_1
#if defined(__SSE4_1__)
#define CORE_HAS_SSE4_1 1
#else
#define CORE_HAS_SSE4_1 0
#endif
#endif

#ifndef CORE_HAS_AVX2
#if defined(__AVX2__)
#define CORE_HAS_AVX2 1
#else
#define CORE_HAS_AVX2 0
#endif
#endif

#ifndef CORE_HAS_NEON
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define CORE_HAS_NEON 1
#else
#define CORE_HAS_NEON 0
#endif
#endif

#ifndef CORE_HAS_SIMD
#if (CORE_HAS_SSE2 || CORE_HAS_SSE4_1 || CORE_HAS_AVX2 || CORE_HAS_NEON)
#define CORE_HAS_SIMD 1
#else
#define CORE_HAS_SIMD 0
#endif
#endif