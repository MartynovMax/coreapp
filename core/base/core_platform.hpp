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

// -----------------------------------------------------------------------------
// CPU architecture detection
// -----------------------------------------------------------------------------

#define CORE_CPU_X86 0
#define CORE_CPU_X64 0
#define CORE_CPU_ARM 0
#define CORE_CPU_ARM64 0

// x86 32-bit
#if defined(_M_IX86) || defined(__i386__)
#undef CORE_CPU_X86
#define CORE_CPU_X86 1

// x86 64-bit
#elif defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
#undef CORE_CPU_X64
#define CORE_CPU_X64 1

// ARM 32-bit
#elif defined(_M_ARM) || defined(__arm__) || defined(__thumb__)
#undef CORE_CPU_ARM
#define CORE_CPU_ARM 1

// ARM 64-bit
#elif defined(_M_ARM64) || defined(__aarch64__)
#undef CORE_CPU_ARM64
#define CORE_CPU_ARM64 1

#else
#error "Unsupported CPU architecture: Core supports x86/x64/ARM/ARM64."
#endif

#if (CORE_CPU_X86 + CORE_CPU_X64 + CORE_CPU_ARM + CORE_CPU_ARM64) != 1
#error "CPU detection error: expected exactly one CORE_CPU_* == 1."
#endif

// -----------------------------------------------------------------------------
// Pointer size helpers
// -----------------------------------------------------------------------------

#define CORE_32BIT 0
#define CORE_64BIT 0

#if CORE_CPU_X64 || CORE_CPU_ARM64
#undef CORE_64BIT
#define CORE_64BIT 1
#elif CORE_CPU_X86 || CORE_CPU_ARM
#undef CORE_32BIT
#define CORE_32BIT 1
#endif

#if (CORE_32BIT + CORE_64BIT) != 1
#error "Pointer-size detection error: expected exactly one CORE_32BIT/CORE_64BIT == 1."
#endif

// -----------------------------------------------------------------------------
// Endianness detection
// -----------------------------------------------------------------------------

#define CORE_LITTLE_ENDIAN 0
#define CORE_BIG_ENDIAN 0

// 1) Standard GCC/Clang byte-order macros.
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    defined(__ORDER_BIG_ENDIAN__)

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#undef CORE_LITTLE_ENDIAN
#define CORE_LITTLE_ENDIAN 1
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#undef CORE_BIG_ENDIAN
#define CORE_BIG_ENDIAN 1
#else
#error "Unknown byte order (__BYTE_ORDER__)."
#endif

// 2) Explicit ARM endianness macros (rare, but handle if provided).
#elif defined(__ARMEB__) || defined(__AARCH64EB__)
#undef CORE_BIG_ENDIAN
#define CORE_BIG_ENDIAN 1

#elif defined(__ARMEL__) || defined(__AARCH64EL__)
#undef CORE_LITTLE_ENDIAN
#define CORE_LITTLE_ENDIAN 1

// 3) Windows: all supported targets are little-endian.
#elif CORE_PLATFORM_WINDOWS
#undef CORE_LITTLE_ENDIAN
#define CORE_LITTLE_ENDIAN 1

// 4) Conservative fallback: assume little-endian for x86/x64.
#elif CORE_CPU_X86 || CORE_CPU_X64
#undef CORE_LITTLE_ENDIAN
#define CORE_LITTLE_ENDIAN 1

#else
#error "Cannot determine endianness for this platform/architecture."
#endif

#if (CORE_LITTLE_ENDIAN + CORE_BIG_ENDIAN) != 1
#error "Endianness detection error: expected exactly one CORE_LITTLE_ENDIAN/CORE_BIG_ENDIAN == 1."
#endif
