#include <gtest/gtest.h>

#include "core/base/core_macros.hpp"

#define CORE_TEST_VALUE 123

TEST(CoreMacros, Stringify) {
  EXPECT_STREQ(CORE_STRINGIFY(ABC), "ABC");
  EXPECT_STREQ(CORE_STRINGIFY(CORE_TEST_VALUE), "123");
  EXPECT_STREQ(CORE_STRINGIFY_RAW(CORE_TEST_VALUE), "CORE_TEST_VALUE");
}

TEST(CoreMacros, Concat) {
  int CORE_CONCAT(foo, bar) = 7;
  EXPECT_EQ(foobar, 7);
}

TEST(CoreMacros, ArraySize) {
  int a[5] = {};
  EXPECT_EQ(CORE_ARRAY_SIZE(a), 5u);
}

TEST(CoreMacros, BitAndFlags) {
  EXPECT_EQ(CORE_BIT(0), 1ull);
  EXPECT_EQ(CORE_BIT(3), 8ull);

  // Test 64-bit support (n >= 32)
  EXPECT_EQ(CORE_BIT(32), 1ull << 32);
  EXPECT_EQ(CORE_BIT(63), 1ull << 63);

  // Test explicit width variants
  EXPECT_EQ(CORE_BIT32(0), 1u);
  EXPECT_EQ(CORE_BIT64(0), 1ull);

  unsigned long long v = 0;
  constexpr unsigned long long kA = CORE_BIT(1);
  constexpr unsigned long long kB = CORE_BIT(4);

  CORE_SET_FLAG(v, kA);
  EXPECT_TRUE(CORE_HAS_FLAG(v, kA));
  EXPECT_FALSE(CORE_HAS_FLAG(v, kB));

  CORE_SET_FLAG(v, kB);
  EXPECT_TRUE(CORE_HAS_FLAG(v, kA));
  EXPECT_TRUE(CORE_HAS_FLAG(v, kB));

  CORE_CLEAR_FLAG(v, kA);
  EXPECT_FALSE(CORE_HAS_FLAG(v, kA));
  EXPECT_TRUE(CORE_HAS_FLAG(v, kB));
}

TEST(CoreMacros, StaticAssertHelpers) {
  CORE_STATIC_ASSERT(1 + 1 == 2, "Math is broken");
  CORE_STATIC_ASSERT_SIZE(unsigned, sizeof(unsigned));
}