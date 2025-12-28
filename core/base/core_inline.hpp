#pragma once

// =============================================================================
// core_inline.hpp
// Cross-compiler inlining control macros for Core.
//
// This header centralizes compiler-specific syntax for forcing or preventing
// inlining, so performance-sensitive code can remain portable and readable.
//
// These macros are *hints*. Inlining decisions are ultimately up to the
// compiler and can be affected by optimization level, LTO, debug info, etc.
// =============================================================================


#include "core_platform.hpp"

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

#if !defined(CORE_HAS_ATTRIBUTE)
#if defined(__has_attribute)
#define CORE_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#define CORE_HAS_ATTRIBUTE(x) 0
#endif
#endif

// -----------------------------------------------------------------------------
// Force inline
// -----------------------------------------------------------------------------

#if !defined(CORE_FORCE_INLINE)
#if CORE_COMPILER_MSVC
#define CORE_FORCE_INLINE __forceinline
#elif CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_FORCE_INLINE inline __attribute__((always_inline))
#else
#define CORE_FORCE_INLINE inline
#endif
#endif

// -----------------------------------------------------------------------------
// No inline
// -----------------------------------------------------------------------------

#if !defined(CORE_NO_INLINE)
#if CORE_COMPILER_MSVC
#define CORE_NO_INLINE __declspec(noinline)
#elif CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_NO_INLINE __attribute__((noinline))
#else
#define CORE_NO_INLINE
#endif
#endif

// -----------------------------------------------------------------------------
// Inlining hints
// -----------------------------------------------------------------------------

#if !defined(CORE_FLATTEN)
#if CORE_COMPILER_GCC
#define CORE_FLATTEN __attribute__((flatten))
#elif CORE_COMPILER_CLANG
#if CORE_HAS_ATTRIBUTE(flatten)
#define CORE_FLATTEN __attribute__((flatten))
#else
#define CORE_FLATTEN
#endif
#else
#define CORE_FLATTEN
#endif
#endif

#if !defined(CORE_NO_INLINE_DEBUG)
#if defined(CORE_DEBUG)
#if CORE_DEBUG
#define CORE_NO_INLINE_DEBUG CORE_NO_INLINE
#else
#define CORE_NO_INLINE_DEBUG
#endif
#else
#if defined(_DEBUG) || !defined(NDEBUG)
#define CORE_NO_INLINE_DEBUG CORE_NO_INLINE
#else
#define CORE_NO_INLINE_DEBUG
#endif
#endif
#endif