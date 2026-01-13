#include "core/memory/segregated_list_allocator.hpp"
#include "core/memory/system_allocator.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/memory/core_memory.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <random>

namespace {

// =============================================================================
// Constructor tests
// =============================================================================

TEST(SegregatedListAllocator, Constructor_ValidConfiguration) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {64, 10},
        {128, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 4, sys, sys);
    
    EXPECT_EQ(alloc.SizeClassCount(), 4);
    EXPECT_GE(alloc.MaxClassSize(), 128);
    EXPECT_GE(alloc.ClassBlockSize(0), 16);
    EXPECT_GE(alloc.ClassBlockSize(1), 32);
    EXPECT_GE(alloc.ClassBlockSize(2), 64);
    EXPECT_GE(alloc.ClassBlockSize(3), 128);
}

TEST(SegregatedListAllocator, Constructor_NullConfigs) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SegregatedListAllocator alloc(nullptr, 4, sys, sys);
    
    EXPECT_EQ(alloc.SizeClassCount(), 0);
    EXPECT_EQ(alloc.MaxClassSize(), 0);
}

TEST(SegregatedListAllocator, Constructor_ZeroCount) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{16, 10}};
    core::SegregatedListAllocator alloc(configs, 0, sys, sys);
    
    EXPECT_EQ(alloc.SizeClassCount(), 0);
}

TEST(SegregatedListAllocator, Constructor_ExceedingMaxClasses) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[40];
    for (core::u32 i = 0; i < 40; ++i) {
        configs[i].block_size = 16 * (i + 1);
        configs[i].block_count = 10;
    }
    
    core::SegregatedListAllocator alloc(configs, 40, sys, sys);
    
    EXPECT_EQ(alloc.SizeClassCount(), 0);
}

TEST(SegregatedListAllocator, Constructor_SkipsInvalidEntries) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {0, 10},    // Invalid: zero block_size
        {32, 0},    // Invalid: zero block_count
        {64, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 4, sys, sys);
    
    EXPECT_EQ(alloc.SizeClassCount(), 2);
}

// =============================================================================
// Allocation tests
// =============================================================================

TEST(SegregatedListAllocator, Allocate_SmallSize_FirstClass) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {64, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 3, sys, sys);
    
    core::memory_size used0 = alloc.ClassUsedBlocks(0);
    core::memory_size used1 = alloc.ClassUsedBlocks(1);
    core::memory_size used2 = alloc.ClassUsedBlocks(2);
    
    void* ptr = core::AllocateBytes(alloc, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(alloc.Owns(ptr));
    
    EXPECT_EQ(alloc.ClassUsedBlocks(0), used0 + 1);
    EXPECT_EQ(alloc.ClassUsedBlocks(1), used1);
    EXPECT_EQ(alloc.ClassUsedBlocks(2), used2);
    
    core::DeallocateBytes(alloc, ptr, 8);
}

TEST(SegregatedListAllocator, Allocate_MediumSize_MiddleClass) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {64, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 3, sys, sys);
    
    core::memory_size used0 = alloc.ClassUsedBlocks(0);
    core::memory_size used1 = alloc.ClassUsedBlocks(1);
    core::memory_size used2 = alloc.ClassUsedBlocks(2);
    
    void* ptr = core::AllocateBytes(alloc, 48);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(alloc.Owns(ptr));
    
    EXPECT_EQ(alloc.ClassUsedBlocks(0), used0);
    EXPECT_EQ(alloc.ClassUsedBlocks(1), used1);
    EXPECT_EQ(alloc.ClassUsedBlocks(2), used2 + 1);
    
    core::DeallocateBytes(alloc, ptr, 48);
}

TEST(SegregatedListAllocator, Allocate_BoundarySize_ExactMatch) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {64, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 3, sys, sys);
    
    void* ptr1 = core::AllocateBytes(alloc, 16);
    void* ptr2 = core::AllocateBytes(alloc, 32);
    void* ptr3 = core::AllocateBytes(alloc, 64);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    
    core::DeallocateBytes(alloc, ptr1, 16);
    core::DeallocateBytes(alloc, ptr2, 32);
    core::DeallocateBytes(alloc, ptr3, 64);
}

TEST(SegregatedListAllocator, Allocate_LargeSize_Fallback) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {64, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 3, sys, sys);
    
    void* ptr = core::AllocateBytes(alloc, 1024);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_EQ(alloc.ClassUsedBlocks(0), 0);
    EXPECT_EQ(alloc.ClassUsedBlocks(1), 0);
    EXPECT_EQ(alloc.ClassUsedBlocks(2), 0);
    
    core::DeallocateBytes(alloc, ptr, 1024);
}

TEST(SegregatedListAllocator, Allocate_StrictAlignment_Fallback) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 2, sys, sys);
    
    core::AllocationRequest req;
    req.size = 16;
    req.alignment = CORE_DEFAULT_ALIGNMENT * 2;
    
    core::memory_size used0 = alloc.ClassUsedBlocks(0);
    core::memory_size used1 = alloc.ClassUsedBlocks(1);
    
    void* ptr = alloc.Allocate(req);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_EQ(alloc.ClassUsedBlocks(0), used0);
    EXPECT_EQ(alloc.ClassUsedBlocks(1), used1);
    
    core::AllocationInfo info;
    info.ptr = ptr;
    info.size = 16;
    info.alignment = req.alignment;
    alloc.Deallocate(info);
}

TEST(SegregatedListAllocator, Allocate_PoolExhaustion_Fallback) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 2}
    };
    
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    void* ptr1 = core::AllocateBytes(alloc, 16);
    void* ptr2 = core::AllocateBytes(alloc, 16);
    void* ptr3 = core::AllocateBytes(alloc, 16);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    
    EXPECT_TRUE(alloc.Owns(ptr1));
    EXPECT_TRUE(alloc.Owns(ptr2));
    
    EXPECT_EQ(alloc.ClassUsedBlocks(0), 2);
    
    core::memory_size freeBefore = alloc.ClassFreeBlocks(0);
    core::DeallocateBytes(alloc, ptr3, 16);
    EXPECT_EQ(alloc.ClassFreeBlocks(0), freeBefore);
    
    core::DeallocateBytes(alloc, ptr1, 16);
    core::DeallocateBytes(alloc, ptr2, 16);
}

TEST(SegregatedListAllocator, Allocate_ZeroSize_ReturnsNull) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{16, 10}};
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    void* ptr = core::AllocateBytes(alloc, 0);
    EXPECT_EQ(ptr, nullptr);
}

TEST(SegregatedListAllocator, Allocate_NearBoundarySizes_CorrectRouting) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {64, 10},
        {128, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 4, sys, sys);
    
    core::memory_size used0 = alloc.ClassUsedBlocks(0);
    core::memory_size used1 = alloc.ClassUsedBlocks(1);
    core::memory_size used2 = alloc.ClassUsedBlocks(2);
    core::memory_size used3 = alloc.ClassUsedBlocks(3);
    
    void* ptr17 = core::AllocateBytes(alloc, 17);
    ASSERT_NE(ptr17, nullptr);
    EXPECT_EQ(alloc.ClassUsedBlocks(0), used0);
    EXPECT_EQ(alloc.ClassUsedBlocks(1), used1 + 1);
    EXPECT_EQ(alloc.ClassUsedBlocks(2), used2);
    EXPECT_EQ(alloc.ClassUsedBlocks(3), used3);
    
    void* ptr33 = core::AllocateBytes(alloc, 33);
    ASSERT_NE(ptr33, nullptr);
    EXPECT_EQ(alloc.ClassUsedBlocks(0), used0);
    EXPECT_EQ(alloc.ClassUsedBlocks(1), used1 + 1);
    EXPECT_EQ(alloc.ClassUsedBlocks(2), used2 + 1);
    EXPECT_EQ(alloc.ClassUsedBlocks(3), used3);
    
    void* ptr65 = core::AllocateBytes(alloc, 65);
    ASSERT_NE(ptr65, nullptr);
    EXPECT_EQ(alloc.ClassUsedBlocks(0), used0);
    EXPECT_EQ(alloc.ClassUsedBlocks(1), used1 + 1);
    EXPECT_EQ(alloc.ClassUsedBlocks(2), used2 + 1);
    EXPECT_EQ(alloc.ClassUsedBlocks(3), used3 + 1);
    
    core::DeallocateBytes(alloc, ptr17, 17);
    core::DeallocateBytes(alloc, ptr33, 33);
    core::DeallocateBytes(alloc, ptr65, 65);
}

// =============================================================================
// Deallocation tests
// =============================================================================

TEST(SegregatedListAllocator, Deallocate_ViaSizeHint) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 2, sys, sys);
    
    void* ptr = core::AllocateBytes(alloc, 16);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_EQ(alloc.ClassFreeBlocks(0), 9);  // 1 used
    
    core::DeallocateBytes(alloc, ptr, 16);  // Size hint provided
    
    EXPECT_EQ(alloc.ClassFreeBlocks(0), 10);  // Back to pool
}

TEST(SegregatedListAllocator, Deallocate_ViaOwnershipScan) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 2, sys, sys);
    
    void* ptr = core::AllocateBytes(alloc, 16);
    ASSERT_NE(ptr, nullptr);
    
    core::DeallocateBytes(alloc, ptr, 0);  // No size hint, uses Owns()
    
    EXPECT_EQ(alloc.ClassFreeBlocks(0), 10);
}

TEST(SegregatedListAllocator, Deallocate_FallbackPointer_CorrectRouting) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 2}
    };
    
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    void* ptr1 = core::AllocateBytes(alloc, 16);
    void* ptr2 = core::AllocateBytes(alloc, 16);
    void* ptr3 = core::AllocateBytes(alloc, 16);
    
    ASSERT_NE(ptr3, nullptr);
    
    EXPECT_EQ(alloc.ClassUsedBlocks(0), 2);
    
    core::memory_size freeBefore = alloc.ClassFreeBlocks(0);
    core::DeallocateBytes(alloc, ptr3, 16);
    core::memory_size freeAfter = alloc.ClassFreeBlocks(0);
    
    EXPECT_EQ(freeBefore, freeAfter);
    
    core::DeallocateBytes(alloc, ptr1, 16);
    core::DeallocateBytes(alloc, ptr2, 16);
}

TEST(SegregatedListAllocator, Deallocate_Nullptr_NoOp) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{16, 10}};
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    core::DeallocateBytes(alloc, nullptr, 16);  // Should not crash
}

// =============================================================================
// Owns() tests
// =============================================================================

TEST(SegregatedListAllocator, Owns_PoolPointer_ReturnsTrue) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 2, sys, sys);
    
    void* ptr = core::AllocateBytes(alloc, 16);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(alloc.Owns(ptr));
    
    core::DeallocateBytes(alloc, ptr, 16);
}

TEST(SegregatedListAllocator, Owns_Nullptr_ReturnsFalse) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{16, 10}};
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    EXPECT_FALSE(alloc.Owns(nullptr));
}

TEST(SegregatedListAllocator, Owns_ExternalPointer_ReturnsFalse) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{16, 10}};
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    int stack_var = 42;
    EXPECT_FALSE(alloc.Owns(&stack_var));
}

// =============================================================================
// Introspection tests
// =============================================================================

TEST(SegregatedListAllocator, Introspection_SizeClassCount) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {64, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 3, sys, sys);
    
    EXPECT_EQ(alloc.SizeClassCount(), 3);
}

TEST(SegregatedListAllocator, Introspection_MaxClassSize) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10},
        {128, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 3, sys, sys);
    
    EXPECT_GE(alloc.MaxClassSize(), 128);
}

TEST(SegregatedListAllocator, Introspection_ClassBlockSize) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 2, sys, sys);
    
    EXPECT_GE(alloc.ClassBlockSize(0), 16);
    EXPECT_GE(alloc.ClassBlockSize(1), 32);
    EXPECT_EQ(alloc.ClassBlockSize(10), 0);  // Out of range
}

TEST(SegregatedListAllocator, Introspection_ClassBlockCount) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 15},
        {32, 20}
    };
    
    core::SegregatedListAllocator alloc(configs, 2, sys, sys);
    
    EXPECT_EQ(alloc.ClassBlockCount(0), 15);
    EXPECT_EQ(alloc.ClassBlockCount(1), 20);
}

TEST(SegregatedListAllocator, Introspection_FreeAndUsedBlocks) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{16, 10}};
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    EXPECT_EQ(alloc.ClassFreeBlocks(0), 10);
    EXPECT_EQ(alloc.ClassUsedBlocks(0), 0);
    
    void* ptr1 = core::AllocateBytes(alloc, 16);
    void* ptr2 = core::AllocateBytes(alloc, 16);
    
    EXPECT_EQ(alloc.ClassFreeBlocks(0), 8);
    EXPECT_EQ(alloc.ClassUsedBlocks(0), 2);
    
    core::DeallocateBytes(alloc, ptr1, 16);
    
    EXPECT_EQ(alloc.ClassFreeBlocks(0), 9);
    EXPECT_EQ(alloc.ClassUsedBlocks(0), 1);
    
    core::DeallocateBytes(alloc, ptr2, 16);
}

TEST(SegregatedListAllocator, Introspection_ClassCapacityBytes) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{64, 10}};
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    core::memory_size capacity = alloc.ClassCapacityBytes(0);
    EXPECT_GE(capacity, 64 * 10);
}

// =============================================================================
// Stress and integration tests
// =============================================================================

TEST(SegregatedListAllocator, Stress_MultipleClassesRandomSizes) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 50},
        {32, 50},
        {64, 50},
        {128, 50},
        {256, 50}
    };
    
    core::SegregatedListAllocator alloc(configs, 5, sys, sys);
    
    std::vector<std::pair<void*, core::memory_size>> pointers;
    core::memory_size sizes[] = {8, 12, 16, 24, 32, 48, 64, 96, 128, 200, 256};
    
    for (int i = 0; i < 100; ++i) {
        core::memory_size size = sizes[i % 11];
        void* ptr = core::AllocateBytes(alloc, size);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back({ptr, size});
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(pointers.begin(), pointers.end(), gen);
    
    for (auto& pair : pointers) {
        EXPECT_TRUE(alloc.Owns(pair.first));
        core::DeallocateBytes(alloc, pair.first, pair.second);
    }
}

TEST(SegregatedListAllocator, Stress_MixedPoolAndFallback) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {
        {16, 10},
        {32, 10}
    };
    
    core::SegregatedListAllocator alloc(configs, 2, sys, sys);
    
    std::vector<std::pair<void*, core::memory_size>> allocations;
    
    // Mix of pool and fallback allocations
    core::memory_size sizes[] = {8, 16, 32, 64, 128, 512, 1024};
    
    for (core::memory_size size : sizes) {
        void* ptr = core::AllocateBytes(alloc, size);
        ASSERT_NE(ptr, nullptr);
        allocations.push_back({ptr, size});
    }
    
    // Deallocate all
    for (auto& pair : allocations) {
        core::DeallocateBytes(alloc, pair.first, pair.second);
    }
}

TEST(SegregatedListAllocator, Integration_ZeroInitialize) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::SizeClassConfig configs[] = {{64, 10}};
    core::SegregatedListAllocator alloc(configs, 1, sys, sys);
    
    void* ptr = core::AllocateBytes(alloc, 32, CORE_DEFAULT_ALIGNMENT, 0, 
                                     core::AllocationFlags::ZeroInitialize);
    ASSERT_NE(ptr, nullptr);
    
    // Verify zero-initialized
    core::u8* bytes = static_cast<core::u8*>(ptr);
    for (core::memory_size i = 0; i < 32; ++i) {
        EXPECT_EQ(bytes[i], 0);
    }
    
    core::DeallocateBytes(alloc, ptr, 32);
}

} // anonymous namespace

