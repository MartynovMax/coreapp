#include "../common/seeded_rng.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>
#include <cmath>

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

// Test rejection sampling avoids modulo bias
TEST(SeededRNGTest, UnbiasedDistribution) {
    SeededRNG rng(12345);
    
    // Test with a range that doesn't evenly divide 2^32 (100 is not a power of 2)
    // With modulo bias, some values would appear more frequently
    constexpr u32 kRange = 100;
    constexpr u32 kIterations = 100000;
    u32 counts[kRange] = {0};
    
    for (u32 i = 0; i < kIterations; ++i) {
        u32 val = rng.NextRange(0, kRange - 1);
        ASSERT_LT(val, kRange);
        counts[val]++;
    }
    
    // Expected count per bucket (with uniform distribution)
    const f64 expected = static_cast<f64>(kIterations) / static_cast<f64>(kRange);
    
    // Check that all buckets are reasonably close to expected
    // Allow 20% deviation (this is a statistical test, some variance is normal)
    const f64 tolerance = expected * 0.20;
    
    for (u32 i = 0; i < kRange; ++i) {
        f64 deviation = static_cast<f64>(counts[i]) - expected;
        EXPECT_LT(std::abs(deviation), tolerance)
            << "Bucket " << i << " has " << counts[i] << " values, expected ~" << expected;
    }
}

// Test NextRange determinism with rejection sampling
TEST(SeededRNGTest, RangeDeterminism) {
    SeededRNG rng1(789);
    SeededRNG rng2(789);
    
    // Test various ranges
    for (int i = 0; i < 100; ++i) {
        u32 val1 = rng1.NextRange(0, 99);
        u32 val2 = rng2.NextRange(0, 99);
        EXPECT_EQ(val1, val2) << "Iteration " << i;
    }
    
    // Test with different range
    SeededRNG rng3(789);
    SeededRNG rng4(789);
    
    for (int i = 0; i < 100; ++i) {
        u32 val1 = rng3.NextRange(50, 150);
        u32 val2 = rng4.NextRange(50, 150);
        EXPECT_EQ(val1, val2) << "Iteration " << i;
    }
}
