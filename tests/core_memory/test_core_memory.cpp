#include "core/memory/core_memory.hpp"
#include <gtest/gtest.h>

TEST(CoreMemory, FundamentalTypesExist) {
    // Verify that fundamental memory types are defined
    core::memory_size s = 0;
    core::memory_alignment a = 0;
    core::memory_tag t = 0;
    
    EXPECT_EQ(s, 0u);
    EXPECT_EQ(a, 0u);
    EXPECT_EQ(t, 0u);
}

TEST(CoreMemory, TypesAreCorrectSize) {
    // memory_size should be pointer-sized
    EXPECT_EQ(sizeof(core::memory_size), sizeof(void*));
    
    // memory_alignment should be u32
    EXPECT_EQ(sizeof(core::memory_alignment), sizeof(core::u32));
    
    // memory_tag should be u32
    EXPECT_EQ(sizeof(core::memory_tag), sizeof(core::u32));
}

TEST(CoreMemory, ConfigMacrosAreDefined) {
    // Verify that configuration macros are defined
    EXPECT_GT(CORE_DEFAULT_ALIGNMENT, 0u);
    EXPECT_GT(CORE_CACHE_LINE_SIZE, 0u);
    
    // CORE_MEMORY_DEBUG should be defined
#ifdef CORE_MEMORY_DEBUG
    EXPECT_TRUE(true);
#else
    FAIL() << "CORE_MEMORY_DEBUG should be defined";
#endif

    // CORE_MEMORY_TRACKING should be defined
#ifdef CORE_MEMORY_TRACKING
    EXPECT_TRUE(true);
#else
    FAIL() << "CORE_MEMORY_TRACKING should be defined";
#endif
}

TEST(CoreMemory, DefaultAlignmentIsPowerOfTwo) {
    EXPECT_GT(CORE_DEFAULT_ALIGNMENT, 0u);
    // Check if power of 2: (n & (n-1)) == 0
    EXPECT_EQ(CORE_DEFAULT_ALIGNMENT & (CORE_DEFAULT_ALIGNMENT - 1), 0u);
}

TEST(CoreMemory, CacheLineSizeIsPowerOfTwo) {
    EXPECT_GT(CORE_CACHE_LINE_SIZE, 0u);
    // Check if power of 2
    EXPECT_EQ(CORE_CACHE_LINE_SIZE & (CORE_CACHE_LINE_SIZE - 1), 0u);
}
