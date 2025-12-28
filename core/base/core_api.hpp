#pragma once

#include "core_platform.hpp"

// -----------------------------------------------------------------------------
// Build configuration sanity
// -----------------------------------------------------------------------------

#if defined(CORE_BUILD_SHARED) && defined(CORE_BUILD_STATIC)
#error "Invalid build configuration: define only one of CORE_BUILD_SHARED or CORE_BUILD_STATIC."
#endif

// -----------------------------------------------------------------------------
// Low-level compiler/platform mappings
// -----------------------------------------------------------------------------

#if defined(CORE_API_DISABLE_VISIBILITY)

#define CORE_API_EXPORT
#define CORE_API_IMPORT
#define CORE_API_INTERNAL

#else

#if CORE_PLATFORM_WINDOWS

#define CORE_API_EXPORT __declspec(dllexport)
#define CORE_API_IMPORT __declspec(dllimport)

#define CORE_API_INTERNAL

#elif (CORE_COMPILER_GCC || CORE_COMPILER_CLANG)

#define CORE_API_EXPORT __attribute__((visibility("default")))
#define CORE_API_IMPORT

#define CORE_API_INTERNAL __attribute__((visibility("hidden")))

#else

#define CORE_API_EXPORT
#define CORE_API_IMPORT
#define CORE_API_INTERNAL

#endif

#endif  // CORE_API_DISABLE_VISIBILITY

// -----------------------------------------------------------------------------
// Top-level macro used by public Core headers
// -----------------------------------------------------------------------------

#if defined(CORE_BUILD_SHARED)

#if defined(CORE_BUILD_CORE_LIBRARY)
#define CORE_API CORE_API_EXPORT
#else
#define CORE_API CORE_API_IMPORT
#endif

#else

#define CORE_API

#endif
