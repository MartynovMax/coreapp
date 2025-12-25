#include <gtest/gtest.h>
#include <cstdint>

#include "core/base/core_platform.hpp"

TEST(CorePlatform, ExactlyOnePlatformIsSelected) {
    const int sum = CORE_PLATFORM_WINDOWS + CORE_PLATFORM_LINUX +
        CORE_PLATFORM_MACOS + CORE_PLATFORM_UNKNOWN;
    EXPECT_EQ(sum, 1);
}

TEST(CorePlatform, ExactlyOneCompilerIsSelected) {
    const int sum = CORE_COMPILER_MSVC + CORE_COMPILER_GCC + CORE_COMPILER_CLANG;
    EXPECT_EQ(sum, 1);
    EXPECT_NE(CORE_COMPILER_VERSION, 0);
}

TEST(CorePlatform, ExactlyOneCpuIsSelected) {
    const int sum = CORE_CPU_X86 + CORE_CPU_X64 + CORE_CPU_ARM + CORE_CPU_ARM64;
    EXPECT_EQ(sum, 1);
}

TEST(CorePlatform, PointerSizeMatches32Or64BitMacros) {
    const int sum = CORE_32BIT + CORE_64BIT;
    EXPECT_EQ(sum, 1);

    if (CORE_32BIT) EXPECT_EQ(sizeof(void*), 4u);
    if (CORE_64BIT) EXPECT_EQ(sizeof(void*), 8u);

    EXPECT_EQ(sizeof(std::uintptr_t), sizeof(void*));
}

TEST(CorePlatform, EndiannessIsSelected) {
    const int sum = CORE_LITTLE_ENDIAN + CORE_BIG_ENDIAN;
    EXPECT_EQ(sum, 1);
}

TEST(CorePlatform, HelperNamesAreNonEmpty) {
    EXPECT_TRUE(CORE_PLATFORM_NAME[0] != '\0');
    EXPECT_TRUE(CORE_COMPILER_NAME[0] != '\0');
    EXPECT_TRUE(CORE_CPU_NAME[0] != '\0');
}