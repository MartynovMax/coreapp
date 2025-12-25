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