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