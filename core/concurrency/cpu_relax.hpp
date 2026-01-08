#pragma once

// =============================================================================
// cpu_relax.hpp
// CPU hint for spin-wait loops.
//
// Provides a platform-specific instruction to signal the CPU that the current
// thread is in a busy-wait loop, improving power efficiency and performance.
// =============================================================================

#include "../base/core_platform.hpp"
#if CORE_COMPILER_MSVC
    #if CORE_CPU_X64 || CORE_CPU_X86
        #include <immintrin.h>  // _mm_pause
    #elif CORE_CPU_ARM64 || CORE_CPU_ARM
        #include <intrin.h>     // __yield
    #endif
#endif

namespace core {

inline void cpu_relax() noexcept {
#if CORE_COMPILER_MSVC
    #if CORE_CPU_X64 || CORE_CPU_X86
        _mm_pause();
    #elif CORE_CPU_ARM64 || CORE_CPU_ARM
        __yield(); 
    #endif
#elif CORE_COMPILER_GCC || CORE_COMPILER_CLANG
    #if CORE_CPU_X64 || CORE_CPU_X86
        __builtin_ia32_pause();
    #elif CORE_CPU_ARM64 || CORE_CPU_ARM
        __asm__ __volatile__("yield" ::: "memory");
    #endif
#endif
}

} // namespace core

