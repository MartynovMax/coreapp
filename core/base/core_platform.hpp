#pragma once

// -----------------------------------------------------------------------------
// Compiler detection
// -----------------------------------------------------------------------------

#define CORE_COMPILER_MSVC 0
#define CORE_COMPILER_GCC 0
#define CORE_COMPILER_CLANG 0

// Numeric representation: MAJOR*10000 + MINOR*100 + PATCH
#define CORE_COMPILER_VERSION 0

// Clang often defines _MSC_VER in clang-cl mode; treat it as Clang first.
#if defined(__clang__)
#undef CORE_COMPILER_CLANG
#define CORE_COMPILER_CLANG 1
#undef CORE_COMPILER_VERSION
#define CORE_COMPILER_VERSION \
    (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)

#elif defined(_MSC_VER)
#undef CORE_COMPILER_MSVC
#define CORE_COMPILER_MSVC 1
#undef CORE_COMPILER_VERSION
#define CORE_COMPILER_VERSION (_MSC_VER)

#elif defined(__GNUC__)
#undef CORE_COMPILER_GCC
#define CORE_COMPILER_GCC 1
#undef CORE_COMPILER_VERSION
#if defined(__GNUC_PATCHLEVEL__)
#define CORE_COMPILER_VERSION \
      (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#define CORE_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#endif

#else
#error "Unsupported compiler: Core requires MSVC, GCC, or Clang."
#endif

#if (CORE_COMPILER_MSVC + CORE_COMPILER_GCC + CORE_COMPILER_CLANG) != 1
#error "Compiler detection error: expected exactly one CORE_COMPILER_* == 1."
#endif

// -----------------------------------------------------------------------------
// Platform detection
// -----------------------------------------------------------------------------

#define CORE_PLATFORM_WINDOWS 0
#define CORE_PLATFORM_LINUX 0
#define CORE_PLATFORM_MACOS 0
#define CORE_PLATFORM_UNKNOWN 0

#if defined(_WIN32) || defined(_WIN64)
#undef CORE_PLATFORM_WINDOWS
#define CORE_PLATFORM_WINDOWS 1

#elif defined(__APPLE__) && defined(__MACH__)
#undef CORE_PLATFORM_MACOS
#define CORE_PLATFORM_MACOS 1

#elif defined(__linux__)
#undef CORE_PLATFORM_LINUX
#define CORE_PLATFORM_LINUX 1

#else
#undef CORE_PLATFORM_UNKNOWN
#define CORE_PLATFORM_UNKNOWN 1
#endif

#if (CORE_PLATFORM_WINDOWS + CORE_PLATFORM_LINUX + CORE_PLATFORM_MACOS + \
     CORE_PLATFORM_UNKNOWN) != 1
#error "Platform detection error: expected exactly one CORE_PLATFORM_* == 1."
#endif

// Optional grouping helper for current targets.
#define CORE_PLATFORM_DESKTOP \
  (CORE_PLATFORM_WINDOWS || CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS)
