#pragma once

// ============================================================================
// core_config.hpp
// Unified compile-time configuration for Core module.
// - Compiler detection
// - Platform detection
// - CPU architecture
// - Endianness
// - Global config flags
// - Utility macros
// ============================================================================

// ----------------------------------------------------------------------------
// C++ standard check (C++20 required)
// ----------------------------------------------------------------------------

#if defined(_MSVC_LANG) && !defined(__clang__)
#if _MSVC_LANG < 202002L
#error "CoreApp requires at least C++20 (/std:c++20)."
#endif
#else
#if __cplusplus < 202002L
#error "CoreApp requires at least C++20 (-std=c++20)."
#endif
#endif

// ----------------------------------------------------------------------------
// Compiler detection
// ----------------------------------------------------------------------------

#define CORE_COMPILER_MSVC 0
#define CORE_COMPILER_CLANG 0
#define CORE_COMPILER_GCC 0

#if defined(_MSC_VER)
#undef CORE_COMPILER_MSVC
#define CORE_COMPILER_MSVC 1
#elif defined(__clang__)
#undef CORE_COMPILER_CLANG
#define CORE_COMPILER_CLANG 1
#elif defined(__GNUC__)
#undef CORE_COMPILER_GCC
#define CORE_COMPILER_GCC 1
#else
#error                                                                         \
    "Unsupported compiler: CoreApp currently supports MSVC, Clang and GCC only."
#endif

// ----------------------------------------------------------------------------
// Platform detection
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// CPU architecture
// ----------------------------------------------------------------------------

#define CORE_CPU_X86 0
#define CORE_CPU_X64 0
#define CORE_CPU_ARM 0
#define CORE_CPU_ARM64 0

// x86 32-bit
#if defined(_M_IX86) || defined(__i386__)
#undef CORE_CPU_X86
#define CORE_CPU_X86 1

// x86 64-bit
#elif defined(_M_X64) || defined(__x86_64__)
#undef CORE_CPU_X64
#define CORE_CPU_X64 1

// ARM 32-bit
#elif defined(_M_ARM) || defined(__arm__)
#undef CORE_CPU_ARM
#define CORE_CPU_ARM 1

// ARM 64-bit
#elif defined(_M_ARM64) || defined(__aarch64__)
#undef CORE_CPU_ARM64
#define CORE_CPU_ARM64 1

#else
#error "Unsupported CPU architecture."
#endif

// ----------------------------------------------------------------------------
// Endianness detection
// ----------------------------------------------------------------------------

#define CORE_LITTLE_ENDIAN 0
#define CORE_BIG_ENDIAN 0

// 1) Known compiler/platform macros
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&             \
    defined(__ORDER_BIG_ENDIAN__)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#undef CORE_LITTLE_ENDIAN
#define CORE_LITTLE_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#undef CORE_BIG_ENDIAN
#define CORE_BIG_ENDIAN 1
#else
#error "Unknown byte order."
#endif

// 2) Windows: all supported targets are little-endian
#elif CORE_PLATFORM_WINDOWS
#undef CORE_LITTLE_ENDIAN
#define CORE_LITTLE_ENDIAN 1

// 3) Fallback: assume little-endian for x86/x64
#elif CORE_CPU_X86 || CORE_CPU_X64
#undef CORE_LITTLE_ENDIAN
#define CORE_LITTLE_ENDIAN 1

#else
#error "Cannot determine endianness for this platform."
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

#define CORE_FORCE_INLINE __forceinline
#define CORE_NO_INLINE __declspec(noinline)

#define CORE_ALIGN(N) __declspec(align(N))

#elif CORE_COMPILER_GCC || CORE_COMPILER_CLANG

#define CORE_FORCE_INLINE inline __attribute__((always_inline))
#define CORE_NO_INLINE __attribute__((noinline))

#define CORE_ALIGN(N) __attribute__((aligned(N)))

#else

#define CORE_FORCE_INLINE inline
#define CORE_NO_INLINE
#define CORE_ALIGN(N) alignas(N)

#endif

#define CORE_UNUSED(x) ((void)(x))

// ----------------------------------------------------------------------------
// Branch prediction helpers
// ----------------------------------------------------------------------------

#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define CORE_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define CORE_LIKELY(expr) (expr)
#define CORE_UNLIKELY(expr) (expr)
#endif

// ----------------------------------------------------------------------------
// Function / result attributes
// ----------------------------------------------------------------------------

#if defined(__cplusplus)
#define CORE_NODISCARD [[nodiscard]]
#define CORE_NORETURN [[noreturn]]
#else
#if CORE_COMPILER_MSVC
#define CORE_NODISCARD
#define CORE_NORETURN __declspec(noreturn)
#elif CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_NODISCARD __attribute__((warn_unused_result))
#define CORE_NORETURN __attribute__((noreturn))
#else
#define CORE_NODISCARD
#define CORE_NORETURN
#endif
#endif

// ----------------------------------------------------------------------------
// Fallthrough annotation
// ----------------------------------------------------------------------------

#if defined(__cplusplus)
#define CORE_FALLTHROUGH [[fallthrough]]
#else
#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
#define CORE_FALLTHROUGH __attribute__((fallthrough))
#else
#define CORE_FALLTHROUGH
#endif
#endif