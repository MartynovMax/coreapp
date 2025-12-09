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

// ----------------------------------------------------------------------------
// CPU architecture
// ----------------------------------------------------------------------------

#define CORE_CPU_X86   0
#define CORE_CPU_X64   0
#define CORE_CPU_ARM   0
#define CORE_CPU_ARM64 0

// x86 32-bit
#if defined(_M_IX86) || defined(__i386__)
#undef  CORE_CPU_X86
#define CORE_CPU_X86 1

// x86 64-bit
#elif defined(_M_X64) || defined(__x86_64__)
#undef  CORE_CPU_X64
#define CORE_CPU_X64 1

// ARM 32-bit
#elif defined(_M_ARM) || defined(__arm__)
#undef  CORE_CPU_ARM
#define CORE_CPU_ARM 1

// ARM 64-bit
#elif defined(_M_ARM64) || defined(__aarch64__)
#undef  CORE_CPU_ARM64
#define CORE_CPU_ARM64 1

#else
#error "Unsupported CPU architecture."
#endif