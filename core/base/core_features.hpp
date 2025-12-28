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