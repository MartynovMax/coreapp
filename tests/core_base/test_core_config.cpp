#include "core/base/core_config.hpp"

// -----------------------------------------------------------------------------
// Helper constexpr functions
// -----------------------------------------------------------------------------

constexpr int CoreCompilerCount() {
  return (CORE_COMPILER_MSVC ? 1 : 0) + (CORE_COMPILER_CLANG ? 1 : 0) +
         (CORE_COMPILER_GCC ? 1 : 0);
}

constexpr int CorePlatformCount() {
  return (CORE_PLATFORM_WINDOWS ? 1 : 0) + (CORE_PLATFORM_LINUX ? 1 : 0) +
         (CORE_PLATFORM_MACOS ? 1 : 0) + (CORE_PLATFORM_UNKNOWN ? 1 : 0);
}

constexpr int CoreCpuCount() {
  return (CORE_CPU_X86 ? 1 : 0) + (CORE_CPU_X64 ? 1 : 0) +
         (CORE_CPU_ARM ? 1 : 0) + (CORE_CPU_ARM64 ? 1 : 0);
}

constexpr int CoreEndianCount() {
  return (CORE_LITTLE_ENDIAN ? 1 : 0) + (CORE_BIG_ENDIAN ? 1 : 0);
}

// -----------------------------------------------------------------------------
// Static asserts
// -----------------------------------------------------------------------------

// 1. Compiler
static_assert(CoreCompilerCount() == 1,
              "Exactly one CORE_COMPILER_* macro must be 1.");

// 2. Platform
static_assert(CorePlatformCount() == 1,
              "Exactly one CORE_PLATFORM_* macro must be 1.");

// 3. CPU
static_assert(CoreCpuCount() == 1, "Exactly one CORE_CPU_* macro must be 1.");

// 4. Endianness
static_assert(CoreEndianCount() == 1,
              "Exactly one CORE_*_ENDIAN macro must be 1.");

#if (CORE_CPU_X86 || CORE_CPU_X64)
static_assert(CORE_LITTLE_ENDIAN,
              "x86/x64 must be little-endian in Core config.");
#endif

#if defined(_MSC_VER)
static_assert(CORE_COMPILER_MSVC,
              "MSVC compiler must define CORE_COMPILER_MSVC.");
#elif defined(__clang__)
static_assert(CORE_COMPILER_CLANG,
              "Clang compiler must define CORE_COMPILER_CLANG.");
#elif defined(__GNUC__)
static_assert(CORE_COMPILER_GCC, "GCC compiler must define CORE_COMPILER_GCC.");
#endif

// 5. CORE_DEBUG must match NDEBUG / _DEBUG configuration
#if defined(NDEBUG)
static_assert(CORE_DEBUG == 0,
              "Release-like build (NDEBUG defined) must set CORE_DEBUG to 0.");
#else
static_assert(
    CORE_DEBUG == 1,
    "Debug-like build (NDEBUG not defined) must set CORE_DEBUG to 1.");
#endif

int main() {
  // No runtime tests for now, everything is compile-time.
  return 0;
}