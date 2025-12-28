#include <gtest/gtest.h>

#include "core/base/core_features.hpp"

namespace {

    static_assert(CORE_HAS_EXCEPTIONS == 0 || CORE_HAS_EXCEPTIONS == 1,
        "CORE_HAS_EXCEPTIONS must be 0/1.");
    static_assert(CORE_HAS_RTTI == 0 || CORE_HAS_RTTI == 1,
        "CORE_HAS_RTTI must be 0/1.");
    static_assert(CORE_HAS_THREAD_LOCAL == 0 || CORE_HAS_THREAD_LOCAL == 1,
        "CORE_HAS_THREAD_LOCAL must be 0/1.");
    static_assert(CORE_HAS_CONSTEXPR_20 == 0 || CORE_HAS_CONSTEXPR_20 == 1,
        "CORE_HAS_CONSTEXPR_20 must be 0/1.");
    static_assert(CORE_HAS_INLINE_VARIABLES == 0 || CORE_HAS_INLINE_VARIABLES == 1,
        "CORE_HAS_INLINE_VARIABLES must be 0/1.");

    static_assert(CORE_HAS_THREADS == 0 || CORE_HAS_THREADS == 1,
        "CORE_HAS_THREADS must be 0/1.");
    static_assert(CORE_HAS_ATOMICS == 0 || CORE_HAS_ATOMICS == 1,
        "CORE_HAS_ATOMICS must be 0/1.");
    static_assert(CORE_HAS_64BIT_ATOMICS == 0 || CORE_HAS_64BIT_ATOMICS == 1,
        "CORE_HAS_64BIT_ATOMICS must be 0/1.");
    static_assert(CORE_HAS_MEMORY_MODEL == 0 || CORE_HAS_MEMORY_MODEL == 1,
        "CORE_HAS_MEMORY_MODEL must be 0/1.");
    static_assert(CORE_HAS_THREAD_FENCE == 0 || CORE_HAS_THREAD_FENCE == 1,
        "CORE_HAS_THREAD_FENCE must be 0/1.");

    static_assert(CORE_HAS_SIMD == 0 || CORE_HAS_SIMD == 1,
        "CORE_HAS_SIMD must be 0/1.");
    static_assert(CORE_HAS_SSE2 == 0 || CORE_HAS_SSE2 == 1,
        "CORE_HAS_SSE2 must be 0/1.");
    static_assert(CORE_HAS_SSE4_1 == 0 || CORE_HAS_SSE4_1 == 1,
        "CORE_HAS_SSE4_1 must be 0/1.");
    static_assert(CORE_HAS_AVX2 == 0 || CORE_HAS_AVX2 == 1,
        "CORE_HAS_AVX2 must be 0/1.");
    static_assert(CORE_HAS_NEON == 0 || CORE_HAS_NEON == 1,
        "CORE_HAS_NEON must be 0/1.");

    static_assert(CORE_HAS_ALIGNED_NEW == 0 || CORE_HAS_ALIGNED_NEW == 1,
        "CORE_HAS_ALIGNED_NEW must be 0/1.");
    static_assert(CORE_HAS_MAX_ALIGN_T_COMPAT == 0 || CORE_HAS_MAX_ALIGN_T_COMPAT == 1,
        "CORE_HAS_MAX_ALIGN_T_COMPAT must be 0/1.");

    static_assert(CORE_HAS_OS_FILESYSTEM == 0 || CORE_HAS_OS_FILESYSTEM == 1,
        "CORE_HAS_OS_FILESYSTEM must be 0/1.");
    static_assert(CORE_HAS_OS_TIMERS == 0 || CORE_HAS_OS_TIMERS == 1,
        "CORE_HAS_OS_TIMERS must be 0/1.");

    static_assert(CORE_HAS_SIMD ==
        (CORE_HAS_SSE2 || CORE_HAS_SSE4_1 || CORE_HAS_AVX2 ||
            CORE_HAS_NEON),
        "CORE_HAS_SIMD must be the OR of individual SIMD flags.");

#if CORE_HAS_ATOMICS && (CORE_CPU_X64 || CORE_CPU_ARM64)
    static_assert(CORE_HAS_64BIT_ATOMICS,
        "64-bit targets with atomics are expected to have 64-bit atomics.");
#endif

#if CORE_CPU_X64
    static_assert(CORE_HAS_SSE2, "x64 requires SSE2.");
#endif

} // namespace

TEST(CoreFeatures, Smoke) {
    // Runtime smoke only; validation is compile-time above.
    SUCCEED();
}
