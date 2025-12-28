#pragma once

#include "core/base/core_platform.hpp"

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