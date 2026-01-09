#include "core/memory/memory_ops.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace core;

// =============================================================================
// Test helpers
// =============================================================================

namespace {

// Sampled validation for large buffers (first 256, last 256, 1024 pseudorandom)
void CheckSampledEquality(const u8* dst, const u8* src, memory_size size) {
    if (size == 0) return;
    
    for (memory_size i = 0; i < 256 && i < size; ++i) {
        EXPECT_EQ(dst[i], src[i]) << "Mismatch at index " << i;
    }
    
    const memory_size tail_start = (size < 256) ? 0 : (size - 256);
    for (memory_size i = tail_start; i < size; ++i) {
        EXPECT_EQ(dst[i], src[i]) << "Mismatch at index " << i;
    }
    
    // Pseudorandom positions, skipping already checked ranges
    for (memory_size j = 0; j < 1024 && j < size; ++j) {
        memory_size i = (j * memory_size(1009) + memory_size(17)) % size;
        if (i < 256 || i >= tail_start) continue;
        EXPECT_EQ(dst[i], src[i]) << "Mismatch at index " << i;
    }
}

void CheckSampledValue(const u8* buffer, memory_size size, u8 expected) {
    if (size == 0) return;
    
    for (memory_size i = 0; i < 256 && i < size; ++i) {
        EXPECT_EQ(buffer[i], expected) << "Mismatch at index " << i;
    }
    
    const memory_size tail_start = (size < 256) ? 0 : (size - 256);
    for (memory_size i = tail_start; i < size; ++i) {
        EXPECT_EQ(buffer[i], expected) << "Mismatch at index " << i;
    }
    
    // Pseudorandom positions, skipping already checked ranges
    for (memory_size j = 0; j < 1024 && j < size; ++j) {
        memory_size i = (j * memory_size(1009) + memory_size(17)) % size;
        if (i < 256 || i >= tail_start) continue;
        EXPECT_EQ(buffer[i], expected) << "Mismatch at index " << i;
    }
}

} // anonymous namespace

// =============================================================================
// memory_copy tests
// =============================================================================

TEST(MemoryCopy, BasicSmallCopy) {
    u8 src[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    u8 dst[16] = {0};

    void* result = memory_copy(dst, src, 16);

    EXPECT_EQ(result, dst);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(dst[i], src[i]) << "Mismatch at index " << i;
    }
}

TEST(MemoryCopy, BasicLargeCopy) {
    constexpr memory_size size = 1024 * 1024;
    std::vector<u8> src(size);
    std::vector<u8> dst(size, 0);

    for (memory_size i = 0; i < size; ++i) {
        src[i] = static_cast<u8>((i * 131u + 17u) & 0xFF);
    }

    void* result = memory_copy(dst.data(), src.data(), size);
    EXPECT_EQ(result, dst.data());
    
    CheckSampledEquality(dst.data(), src.data(), size);
}

TEST(MemoryCopy, SizeZero) {
    u8 src[4] = {1, 2, 3, 4};
    u8 dst[4] = {0, 0, 0, 0};

    void* result = memory_copy(dst, src, 0);

    EXPECT_EQ(result, dst);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(dst[i], 0);
    }
}

TEST(MemoryCopy, SamePointer) {
    u8 buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    u8 original[8];
    void* result1 = memory_copy(original, buffer, 8);
    EXPECT_EQ(result1, original);

    void* result = memory_copy(buffer, buffer, 8);
    EXPECT_EQ(result, buffer);
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[i], original[i]);
    }
}

TEST(MemoryCopy, DifferentSizes) {
    u8 src[32];
    u8 dst[32];

    static constexpr memory_size sizes[] = {1, 2, 4, 8, 16, 32};
    for (memory_size size : sizes) {
        for (memory_size i = 0; i < size; ++i) {
            src[i] = static_cast<u8>(i * 7);
            dst[i] = 0;
        }

        memory_copy(dst, src, size);

        for (memory_size i = 0; i < size; ++i) {
            EXPECT_EQ(dst[i], src[i]) << "Size " << size << ", index " << i;
        }
    }
}

#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST
#if !CORE_PLATFORM_WINDOWS
// Death test can be flaky on Windows due to different assert mechanisms
TEST(MemoryCopyDeathTest, OverlapDetection) {
    u8 buffer[16];
    for (int i = 0; i < 16; ++i) {
        buffer[i] = static_cast<u8>(i);
    }

    EXPECT_DEATH({
        memory_copy(buffer + 4, buffer, 8);
    }, ".*");
}
#endif // !CORE_PLATFORM_WINDOWS
#endif // CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST

// =============================================================================
// memory_move tests
// =============================================================================

TEST(MemoryMove, BasicSmallMove) {
    u8 src[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    u8 dst[16] = {0};

    void* result = memory_move(dst, src, 16);

    EXPECT_EQ(result, dst);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(dst[i], src[i]) << "Mismatch at index " << i;
    }
}

TEST(MemoryMove, BasicLargeMove) {
    constexpr memory_size size = 1024 * 1024;
    std::vector<u8> src(size);
    std::vector<u8> dst(size, 0);

    for (memory_size i = 0; i < size; ++i) {
        src[i] = static_cast<u8>((i * 131u + 17u) & 0xFF);
    }

    void* result = memory_move(dst.data(), src.data(), size);
    EXPECT_EQ(result, dst.data());
    
    CheckSampledEquality(dst.data(), src.data(), size);
}

TEST(MemoryMove, SizeZero) {
    u8 src[4] = {1, 2, 3, 4};
    u8 dst[4] = {0, 0, 0, 0};

    void* result = memory_move(dst, src, 0);

    EXPECT_EQ(result, dst);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(dst[i], 0);
    }
}

TEST(MemoryMove, SamePointer) {
    u8 buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    u8 original[8];
    void* result1 = memory_copy(original, buffer, 8);
    EXPECT_EQ(result1, original);

    void* result = memory_move(buffer, buffer, 8);
    EXPECT_EQ(result, buffer);
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[i], original[i]);
    }
}

TEST(MemoryMove, OverlappingForward) {
    u8 buffer[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    void* result = memory_move(buffer, buffer + 4, 8);
    EXPECT_EQ(result, buffer);

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[i], 4 + i) << "Mismatch at index " << i;
    }
    for (int i = 8; i < 16; ++i) {
        EXPECT_EQ(buffer[i], i) << "Mismatch at index " << i;
    }
}

TEST(MemoryMove, OverlappingBackward) {
    u8 buffer[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    void* result = memory_move(buffer + 4, buffer, 8);
    EXPECT_EQ(result, buffer + 4);

    EXPECT_EQ(buffer[0], 0);
    EXPECT_EQ(buffer[1], 1);
    EXPECT_EQ(buffer[2], 2);
    EXPECT_EQ(buffer[3], 3);
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[4 + i], i) << "Mismatch at index " << (4 + i);
    }
    
    EXPECT_EQ(buffer[12], 12);
    EXPECT_EQ(buffer[13], 13);
    EXPECT_EQ(buffer[14], 14);
    EXPECT_EQ(buffer[15], 15);
}

TEST(MemoryMove, OverlappingPartial) {
    u8 buffer[32];
    for (int i = 0; i < 32; ++i) {
        buffer[i] = static_cast<u8>(i);
    }

    void* result = memory_move(buffer + 12, buffer + 8, 16);
    EXPECT_EQ(result, buffer + 12);

    for (int i = 0; i < 12; ++i) {
        EXPECT_EQ(buffer[i], i) << "Mismatch at index " << i;
    }
    
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(buffer[12 + i], 8 + i) << "Mismatch at index " << (12 + i);
    }
    
    for (int i = 28; i < 32; ++i) {
        EXPECT_EQ(buffer[i], i) << "Mismatch at index " << i;
    }
}

TEST(MemoryMove, NonOverlappingDstAfterSrc) {
    u8 buffer[32];
    for (int i = 0; i < 32; ++i) {
        buffer[i] = static_cast<u8>(i);
    }

    void* result = memory_move(buffer + 20, buffer, 8);
    EXPECT_EQ(result, buffer + 20);

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[i], i) << "Source mismatch at index " << i;
    }
    
    for (int i = 8; i < 20; ++i) {
        EXPECT_EQ(buffer[i], i) << "Middle mismatch at index " << i;
    }
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[20 + i], i) << "Destination mismatch at index " << (20 + i);
    }
    
    for (int i = 28; i < 32; ++i) {
        EXPECT_EQ(buffer[i], i) << "End mismatch at index " << i;
    }
}

TEST(MemoryMove, OverlappingOddSize) {
    u8 buffer[32];
    for (int i = 0; i < 32; ++i) {
        buffer[i] = static_cast<u8>(i);
    }

    void* result = memory_move(buffer + 10, buffer + 5, 15);
    EXPECT_EQ(result, buffer + 10);

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(buffer[i], i) << "Beginning mismatch at index " << i;
    }
    
    for (int i = 0; i < 15; ++i) {
        EXPECT_EQ(buffer[10 + i], 5 + i) << "Destination mismatch at index " << (10 + i);
    }
    
    for (int i = 25; i < 32; ++i) {
        EXPECT_EQ(buffer[i], i) << "End mismatch at index " << i;
    }
}

// =============================================================================
// memory_set tests
// =============================================================================

TEST(MemorySet, BasicSmallSet) {
    u8 buffer[16] = {0};

    void* result = memory_set(buffer, 0xAA, 16);

    EXPECT_EQ(result, buffer);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(buffer[i], 0xAA) << "Mismatch at index " << i;
    }
}

TEST(MemorySet, BasicLargeSet) {
    constexpr memory_size size = 1024 * 1024;
    std::vector<u8> buffer(size, 0);

    void* result = memory_set(buffer.data(), 0x55, size);
    EXPECT_EQ(result, buffer.data());
    
    CheckSampledValue(buffer.data(), size, 0x55);
}

TEST(MemorySet, SizeZero) {
    u8 buffer[4] = {1, 2, 3, 4};

    void* result = memory_set(buffer, 0, 0);

    EXPECT_EQ(result, buffer);
    // Buffer should remain unchanged
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 2);
    EXPECT_EQ(buffer[2], 3);
    EXPECT_EQ(buffer[3], 4);
}

TEST(MemorySet, ZeroValue) {
    u8 buffer[32];
    for (int i = 0; i < 32; ++i) {
        buffer[i] = 0xFF;
    }

    memory_set(buffer, 0, 32);

    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(buffer[i], 0) << "Mismatch at index " << i;
    }
}

TEST(MemorySet, DifferentValues) {
    u8 buffer[16];

    for (int value : {0x00, 0x11, 0x55, 0xAA, 0xFF}) {
        memory_set(buffer, value, 16);

        for (int i = 0; i < 16; ++i) {
            EXPECT_EQ(buffer[i], static_cast<u8>(value)) 
                << "Value " << value << ", index " << i;
        }
    }
}

TEST(MemorySet, ValueCastToUnsignedChar) {
    u8 buffer[8];

    memory_set(buffer, 0x1234, 8);

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[i], 0x34) << "Mismatch at index " << i;
    }
}

TEST(MemorySet, NegativeValue) {
    u8 buffer[16];

    memory_set(buffer, -1, 16);

    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(buffer[i], 0xFF) << "Mismatch at index " << i;
    }
}

TEST(MemorySet, DifferentSizes) {
    u8 buffer[64];

    static constexpr memory_size sizes[] = {1, 2, 4, 8, 16, 32, 64};
    for (memory_size size : sizes) {
        // Guard value to detect write-overruns
        for (memory_size i = 0; i < 64; ++i) {
            buffer[i] = 0xCC;
        }
        
        memory_set(buffer, 0x77, size);

        for (memory_size i = 0; i < size; ++i) {
            EXPECT_EQ(buffer[i], 0x77) << "Size " << size << ", index " << i;
        }
        
        for (memory_size i = size; i < 64; ++i) {
            EXPECT_EQ(buffer[i], 0xCC) << "Overrun detected at size " << size << ", index " << i;
        }
    }
}

// =============================================================================
// memory_zero tests
// =============================================================================

TEST(MemoryZero, BasicSmallZero) {
    u8 buffer[16];
    for (int i = 0; i < 16; ++i) {
        buffer[i] = 0xFF;
    }

    void* result = memory_zero(buffer, 16);

    EXPECT_EQ(result, buffer);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(buffer[i], 0) << "Mismatch at index " << i;
    }
}

TEST(MemoryZero, BasicLargeZero) {
    constexpr memory_size size = 1024 * 1024;
    std::vector<u8> buffer(size, 0xFF);

    void* result = memory_zero(buffer.data(), size);
    EXPECT_EQ(result, buffer.data());
    
    CheckSampledValue(buffer.data(), size, 0);
}

TEST(MemoryZero, SizeZero) {
    u8 buffer[4] = {1, 2, 3, 4};

    void* result = memory_zero(buffer, 0);

    EXPECT_EQ(result, buffer);
    // Buffer should remain unchanged
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[1], 2);
    EXPECT_EQ(buffer[2], 3);
    EXPECT_EQ(buffer[3], 4);
}

// =============================================================================
// Cross-function integration tests
// =============================================================================

TEST(MemoryOps, CopyThenSet) {
    u8 src[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    u8 dst[16];

    memory_copy(dst, src, 16);
    memory_set(dst, 0, 8); // Zero first half

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(dst[i], 0);
    }
    for (int i = 8; i < 16; ++i) {
        EXPECT_EQ(dst[i], src[i]);
    }
}

TEST(MemoryOps, MoveThenZero) {
    u8 buffer[32];
    for (int i = 0; i < 32; ++i) {
        buffer[i] = static_cast<u8>(i);
    }

    memory_move(buffer + 8, buffer, 16);
    memory_zero(buffer, 8);

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(buffer[i], 0);
    }
    for (int i = 8; i < 24; ++i) {
        EXPECT_EQ(buffer[i], i - 8);
    }
}

