#include <gtest/gtest.h>

#include "core/base/core_version.hpp"

// Compile-time invariants (minimal, but protects accidental changes).
static_assert(CORE_VERSION_MAJOR >= 0,
              "CORE_VERSION_MAJOR must be non-negative");
static_assert(CORE_VERSION_MINOR >= 0,
              "CORE_VERSION_MINOR must be non-negative");
static_assert(CORE_VERSION_PATCH >= 0,
              "CORE_VERSION_PATCH must be non-negative");

static_assert(
    CORE_VERSION_NUMBER == CORE_VERSION_MAKE(CORE_VERSION_MAJOR,
                                             CORE_VERSION_MINOR,
                                             CORE_VERSION_PATCH),
    "CORE_VERSION_NUMBER must match CORE_VERSION_MAKE(MAJOR, MINOR, PATCH)");

static_assert(CORE_VERSION_AT_LEAST(0, 0, 0),
              "Core version should be at least 0.0.0");

TEST(CoreVersion, MakeMatchesNumber) {
  constexpr int n = CORE_VERSION_MAKE(CORE_VERSION_MAJOR, CORE_VERSION_MINOR,
                                      CORE_VERSION_PATCH);
  EXPECT_EQ(n, CORE_VERSION_NUMBER);
}

TEST(CoreVersion, AtLeastWorks) {
  EXPECT_TRUE(CORE_VERSION_AT_LEAST(CORE_VERSION_MAJOR, CORE_VERSION_MINOR,
                                    CORE_VERSION_PATCH));

  // If PATCH>0 then version must be at least same major/minor with patch-1.
  // If PATCH==0 we fall back to a trivially true check.
#if (CORE_VERSION_PATCH > 0)
  EXPECT_TRUE(CORE_VERSION_AT_LEAST(CORE_VERSION_MAJOR, CORE_VERSION_MINOR,
                                    CORE_VERSION_PATCH - 1));
#else
  EXPECT_TRUE(CORE_VERSION_AT_LEAST(0, 0, 0));
#endif
}

TEST(CoreVersion, OptionalFieldsDefaultToStrings) {
  // Must be valid string literals (never null).
  EXPECT_NE(CORE_VERSION_PRERELEASE, nullptr);
  EXPECT_NE(CORE_VERSION_METADATA, nullptr);

  // Defaults are empty by spec.
  EXPECT_STREQ(CORE_VERSION_PRERELEASE, "");
  EXPECT_STREQ(CORE_VERSION_METADATA, "");
}
