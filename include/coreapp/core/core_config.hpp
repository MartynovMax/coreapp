#pragma once


// ----------------------------------------------------------------------------
// Compiler detection
// ----------------------------------------------------------------------------

#define CORE_COMPILER_MSVC   0
#define CORE_COMPILER_CLANG  0
#define CORE_COMPILER_GCC    0

#if defined(_MSC_VER)
#undef  CORE_COMPILER_MSVC
#define CORE_COMPILER_MSVC 1
#elif defined(__clang__)
#undef  CORE_COMPILER_CLANG
#define CORE_COMPILER_CLANG 1
#elif defined(__GNUC__)
#undef  CORE_COMPILER_GCC
#define CORE_COMPILER_GCC 1
#else
#error "Unsupported compiler: CoreApp currently supports MSVC, Clang and GCC only."
#endif

// ----------------------------------------------------------------------------
// Platform detection
// ----------------------------------------------------------------------------

#define CORE_PLATFORM_WINDOWS 0
#define CORE_PLATFORM_LINUX   0
#define CORE_PLATFORM_MACOS   0
#define CORE_PLATFORM_UNKNOWN 0

#if defined(_WIN32) || defined(_WIN64)
#undef  CORE_PLATFORM_WINDOWS
#define CORE_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
#undef  CORE_PLATFORM_MACOS
#define CORE_PLATFORM_MACOS 1
#elif defined(__linux__)
#undef  CORE_PLATFORM_LINUX
#define CORE_PLATFORM_LINUX 1
#else
#undef  CORE_PLATFORM_UNKNOWN
#define CORE_PLATFORM_UNKNOWN 1
#endif