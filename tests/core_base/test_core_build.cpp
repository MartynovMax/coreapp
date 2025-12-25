#include <gtest/gtest.h>

#include "core/base/core_build.hpp"

TEST(CoreBuild, ExactlyOneBuildTypeIsSelected) {
    const int sum =
        CORE_BUILD_DEBUG + CORE_BUILD_RELEASE +
        CORE_BUILD_DEVELOPMENT + CORE_BUILD_SHIPPING;
    EXPECT_EQ(sum, 1);
}

TEST(CoreBuild, BuildTypeMatchesCoreDebugDefaultMapping) {
    if (CORE_BUILD_DEBUG) {
        EXPECT_EQ(CORE_DEBUG, 1);
    }
    if (CORE_BUILD_RELEASE) {
        EXPECT_EQ(CORE_DEBUG, 0);
    }
}

TEST(CoreBuild, FeatureFlagsAreNormalized) {
    EXPECT_TRUE(CORE_BUILD_WITH_ASSERTS == 0 || CORE_BUILD_WITH_ASSERTS == 1);
    EXPECT_TRUE(CORE_BUILD_WITH_LOGS == 0 || CORE_BUILD_WITH_LOGS == 1);
    EXPECT_TRUE(CORE_BUILD_WITH_OPTIMIZATIONS == 0 || CORE_BUILD_WITH_OPTIMIZATIONS == 1);
}

TEST(CoreBuild, IdentificationStringsAreNonEmpty) {
    EXPECT_NE(CORE_BUILD_ID[0], '\0');
    EXPECT_NE(CORE_BUILD_BRANCH[0], '\0');
    EXPECT_NE(CORE_BUILD_COMMIT[0], '\0');
}

TEST(CoreBuild, TimestampStringsAreNonEmpty) {
    EXPECT_NE(CORE_BUILD_DATE[0], '\0');
    EXPECT_NE(CORE_BUILD_TIME[0], '\0');
    EXPECT_NE(CORE_BUILD_DATETIME[0], '\0');
}

TEST(CoreBuild, HelperStringsAreNonEmpty) {
    EXPECT_NE(CORE_BUILD_CONFIG_NAME[0], '\0');
    EXPECT_NE(CORE_BUILD_FULL_NAME[0], '\0');
}
