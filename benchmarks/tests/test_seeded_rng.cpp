#include "../common/seeded_rng.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;

// Test RNG determinism (same seed → same sequence)
TEST(SeededRNGTest, Determinism) {
    SeededRNG rng1(42);
    SeededRNG rng2(42);

    // Generate sequence from both RNGs
    for (int i = 0; i < 100; ++i) {
        u64 val1 = rng1.NextU64();
        u64 val2 = rng2.NextU64();
        EXPECT_EQ(val1, val2);
    }

    // Test u32 generation
    SeededRNG rng3(123);
    SeededRNG rng4(123);

    for (int i = 0; i < 100; ++i) {
        u32 val1 = rng3.NextU32();
        u32 val2 = rng4.NextU32();
        EXPECT_EQ(val1, val2);
    }
}

// Test different seeds produce different sequences
TEST(SeededRNGTest, DifferentSeeds) {
    SeededRNG rng1(42);
    SeededRNG rng2(123);

    u64 val1 = rng1.NextU64();
    u64 val2 = rng2.NextU64();

    EXPECT_NE(val1, val2);
}

// Test RNG range (values in bounds)
TEST(SeededRNGTest, RangeValues) {
    SeededRNG rng(42);

    // Test range [0, 10]
    for (int i = 0; i < 1000; ++i) {
        u32 val = rng.NextRange(0, 10);
        EXPECT_GE(val, 0u);
        EXPECT_LE(val, 10u);
    }

    // Test range [100, 200]
    SeededRNG rng2(123);
    for (int i = 0; i < 1000; ++i) {
        u32 val = rng2.NextRange(100, 200);
        EXPECT_GE(val, 100u);
        EXPECT_LE(val, 200u);
    }

    // Test single value range
    SeededRNG rng3(456);
    for (int i = 0; i < 10; ++i) {
        u32 val = rng3.NextRange(5, 5);
        EXPECT_EQ(val, 5u);
    }
}

// Test zero seed handling
TEST(SeededRNGTest, ZeroSeed) {
    SeededRNG rng(0);

    // Should still generate non-zero values (state initialized to 1)
    u64 val = rng.NextU64();
    EXPECT_NE(val, 0ull);
}

// Test range edge case (max == UINT32_MAX)
TEST(SeededRNGTest, RangeMaxValue) {
    SeededRNG rng(42);

    // Test full u32 range [0, UINT32_MAX]
    for (int i = 0; i < 100; ++i) {
        u32 val = rng.NextRange(0, 0xFFFFFFFFu);
        // Just check it doesn't crash (no UB from overflow)
        (void)val;
    }

    // Test large range near max
    for (int i = 0; i < 100; ++i) {
        u32 val = rng.NextRange(0xFFFFFFF0u, 0xFFFFFFFFu);
        EXPECT_GE(val, 0xFFFFFFF0u);
        EXPECT_LE(val, 0xFFFFFFFFu);
    }
}
