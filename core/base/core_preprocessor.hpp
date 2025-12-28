#pragma once

// =============================================================================
// core_preprocessor.hpp
// Advanced preprocessor utilities for feature detection and conditional compilation.
//
// Focus:
//   - C++ standard version helpers
//   - attribute/feature detection wrappers (__has_cpp_attribute, etc.)
//   - header availability checks (__has_include)
//   - general CORE_HAS_* capability macros (exceptions, RTTI, thread_local, ...)
//   - builtin detection wrapper (__has_builtin)
// =============================================================================


#include "core/base/core_platform.hpp"

// -----------------------------------------------------------------------------
// C++ standard version helpers
// -----------------------------------------------------------------------------

#if !defined(CORE_CPP_VERSION)
#if CORE_COMPILER_MSVC && !defined(__clang__)
#define CORE_CPP_VERSION _MSVC_LANG
#else
#define CORE_CPP_VERSION __cplusplus
#endif
#endif

#define CORE_CPP11_OR_GREATER (CORE_CPP_VERSION >= 201103L)
#define CORE_CPP14_OR_GREATER (CORE_CPP_VERSION >= 201402L)
#define CORE_CPP17_OR_GREATER (CORE_CPP_VERSION >= 201703L)
#define CORE_CPP20_OR_GREATER (CORE_CPP_VERSION >= 202002L)

// -----------------------------------------------------------------------------
// Attribute detection
// -----------------------------------------------------------------------------

#if !defined(CORE_HAS_CPP_ATTRIBUTE)
#if defined(__has_cpp_attribute)
#define CORE_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define CORE_HAS_CPP_ATTRIBUTE(x) 0
#endif
#endif

#if !defined(CORE_HAS_NODISCARD)
#if CORE_CPP17_OR_GREATER || (CORE_HAS_CPP_ATTRIBUTE(nodiscard) != 0)
#define CORE_HAS_NODISCARD 1
#else
#define CORE_HAS_NODISCARD 0
#endif
#endif

#if !defined(CORE_HAS_NORETURN)
#if CORE_CPP11_OR_GREATER || (CORE_HAS_CPP_ATTRIBUTE(noreturn) != 0)
#define CORE_HAS_NORETURN 1
#else
#define CORE_HAS_NORETURN 0
#endif
#endif

#if !defined(CORE_HAS_FALLTHROUGH)
#if CORE_CPP17_OR_GREATER || (CORE_HAS_CPP_ATTRIBUTE(fallthrough) != 0)
#define CORE_HAS_FALLTHROUGH 1
#else
#define CORE_HAS_FALLTHROUGH 0
#endif
#endif

#if !defined(CORE_HAS_DEPRECATED)
#if CORE_CPP14_OR_GREATER || (CORE_HAS_CPP_ATTRIBUTE(deprecated) != 0)
#define CORE_HAS_DEPRECATED 1
#else
#define CORE_HAS_DEPRECATED 0
#endif
#endif

// -----------------------------------------------------------------------------
// Header availability
// -----------------------------------------------------------------------------

#if !defined(CORE_HAS_INCLUDE)
#if defined(__has_include)
#define CORE_HAS_INCLUDE(path) __has_include(path)
#else
#define CORE_HAS_INCLUDE(path) 0
#endif
#endif

#if !defined(CORE_HAS_STD_HEADER)
#define CORE_HAS_STD_HEADER(header) CORE_HAS_INCLUDE(header)
#endif

// -----------------------------------------------------------------------------
// General feature flags
// -----------------------------------------------------------------------------

// Exceptions
#if !defined(CORE_HAS_EXCEPTIONS)
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define CORE_HAS_EXCEPTIONS 1
#else
#define CORE_HAS_EXCEPTIONS 0
#endif
#endif

// RTTI
#if !defined(CORE_HAS_RTTI)
#if defined(__cpp_rtti) || defined(__GXX_RTTI) || defined(_CPPRTTI)
#define CORE_HAS_RTTI 1
#else
#define CORE_HAS_RTTI 0
#endif
#endif

// thread_local storage
#if !defined(CORE_HAS_THREAD_LOCAL)
#if defined(__cpp_thread_local)
#define CORE_HAS_THREAD_LOCAL 1
#elif CORE_CPP11_OR_GREATER
// Conservative assumption: on modern MSVC/GCC/Clang in C++11+ mode, thread_local exists.
#define CORE_HAS_THREAD_LOCAL 1
#else
#define CORE_HAS_THREAD_LOCAL 0
#endif
#endif

// -----------------------------------------------------------------------------
// Builtin detection
// -----------------------------------------------------------------------------

#if !defined(CORE_HAS_BUILTIN)
#if defined(__has_builtin)
#define CORE_HAS_BUILTIN(x) __has_builtin(x)
#else
#define CORE_HAS_BUILTIN(x) 0
#endif
#endif