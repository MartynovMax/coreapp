#include "core/memory/pool_allocator.hpp"
#include "core/memory/system_allocator.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/memory/core_memory.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

namespace {

// =============================================================================
// Constructor tests
// =============================================================================

TEST(PoolAllocator, NonOwningConstructor_ValidBuffer) {
    constexpr core::memory_size kBufferSize = 1024;
    constexpr core::memory_size kBlockSize = 64;
    alignas(16) core::u8 buffer[kBufferSize];
    
    core::PoolAllocator pool(buffer, kBufferSize, kBlockSize);
    
    EXPECT_GT(pool.BlockCount(), 0);
    EXPECT_GE(pool.BlockSize(), kBlockSize);
    EXPECT_EQ(pool.FreeBlocks(), pool.BlockCount());
    EXPECT_EQ(pool.UsedBlocks(), 0);
    EXPECT_EQ(pool.CapacityBytes(), pool.BlockSize() * pool.BlockCount());
}

TEST(PoolAllocator, NonOwningConstructor_UnalignedBuffer) {
    constexpr core::memory_size kBufferSize = 1024;
    constexpr core::memory_size kBlockSize = 64;
    core::u8 buffer[kBufferSize + 16];
    
    core::u8* unaligned = buffer + 7;
    core::PoolAllocator pool(unaligned, kBufferSize, kBlockSize);
    
    EXPECT_GT(pool.BlockCount(), 0);
    EXPECT_EQ(pool.FreeBlocks(), pool.BlockCount());
    
    void* ptr = core::AllocateBytes(pool, 8);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, CORE_DEFAULT_ALIGNMENT));
}

TEST(PoolAllocator, NonOwningConstructor_NullBuffer) {
    core::PoolAllocator pool(nullptr, 1024, 64);
    
    EXPECT_EQ(pool.BlockCount(), 0);
    EXPECT_EQ(pool.BlockSize(), 0);
    EXPECT_EQ(pool.FreeBlocks(), 0);
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

TEST(PoolAllocator, NonOwningConstructor_ZeroSize) {
    core::u8 buffer[128];
    core::PoolAllocator pool(buffer, 0, 64);
    
    EXPECT_EQ(pool.BlockCount(), 0);
    EXPECT_EQ(pool.FreeBlocks(), 0);
}

TEST(PoolAllocator, NonOwningConstructor_ZeroBlockSize) {
    core::u8 buffer[128];
    core::PoolAllocator pool(buffer, 128, 0);
    
    EXPECT_EQ(pool.BlockCount(), 0);
}

TEST(PoolAllocator, NonOwningConstructor_TooSmallBuffer) {
    core::u8 buffer[32];
    core::PoolAllocator pool(buffer, 32, 1024);
    
    EXPECT_EQ(pool.BlockCount(), 0);
}

TEST(PoolAllocator, OwningConstructor_ValidParameters) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 100;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    EXPECT_EQ(pool.BlockCount(), kBlockCount);
    EXPECT_GE(pool.BlockSize(), kBlockSize);
    EXPECT_EQ(pool.FreeBlocks(), kBlockCount);
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

TEST(PoolAllocator, OwningConstructor_ZeroBlockSize) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(0, 100, sys);
    
    EXPECT_EQ(pool.BlockCount(), 0);
}

TEST(PoolAllocator, OwningConstructor_ZeroBlockCount) {
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(64, 0, sys);
    
    EXPECT_EQ(pool.BlockCount(), 0);
}

// =============================================================================
// Basic allocation/deallocation tests
// =============================================================================

TEST(PoolAllocator, SingleAllocation) {
    constexpr core::memory_size kBufferSize = 1024;
    constexpr core::memory_size kBlockSize = 64;
    alignas(16) core::u8 buffer[kBufferSize];
    core::PoolAllocator pool(buffer, kBufferSize, kBlockSize);
    
    const core::memory_size initial_free = pool.FreeBlocks();
    
    core::AllocationRequest req{};
    req.size = 32;
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    
    void* ptr = pool.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, CORE_DEFAULT_ALIGNMENT));
    EXPECT_EQ(pool.FreeBlocks(), initial_free - 1);
    EXPECT_EQ(pool.UsedBlocks(), 1);
    EXPECT_TRUE(pool.Owns(ptr));
}

TEST(PoolAllocator, AllocateDeallocateSingle) {
    constexpr core::memory_size kBufferSize = 1024;
    constexpr core::memory_size kBlockSize = 64;
    alignas(16) core::u8 buffer[kBufferSize];
    core::PoolAllocator pool(buffer, kBufferSize, kBlockSize);
    
    const core::memory_size initial_free = pool.FreeBlocks();
    
    void* ptr = core::AllocateBytes(pool, 32, CORE_DEFAULT_ALIGNMENT);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(pool.FreeBlocks(), initial_free - 1);
    
    core::DeallocateBytes(pool, ptr, 32, CORE_DEFAULT_ALIGNMENT);
    EXPECT_EQ(pool.FreeBlocks(), initial_free);
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

TEST(PoolAllocator, MultipleAllocations) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    std::vector<void*> pointers;
    
    for (core::memory_size i = 0; i < 5; ++i) {
        void* ptr = core::AllocateBytes(pool, 32);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }
    
    EXPECT_EQ(pool.UsedBlocks(), 5);
    EXPECT_EQ(pool.FreeBlocks(), 5);
    
    for (void* ptr : pointers) {
        EXPECT_TRUE(pool.Owns(ptr));
        EXPECT_NE(ptr, nullptr);
    }
    
    for (size_t i = 0; i < pointers.size(); ++i) {
        for (size_t j = i + 1; j < pointers.size(); ++j) {
            EXPECT_NE(pointers[i], pointers[j]);
        }
    }
}

TEST(PoolAllocator, AllocateDeallocateMultiple) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    std::vector<void*> pointers;
    
    for (core::memory_size i = 0; i < 8; ++i) {
        void* ptr = core::AllocateBytes(pool, 40);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }
    
    EXPECT_EQ(pool.UsedBlocks(), 8);
    
    for (void* ptr : pointers) {
        core::DeallocateBytes(pool, ptr, 40);
    }
    
    EXPECT_EQ(pool.UsedBlocks(), 0);
    EXPECT_EQ(pool.FreeBlocks(), kBlockCount);
}

TEST(PoolAllocator, NonLIFODeallocation) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    void* ptr1 = core::AllocateBytes(pool, 32);
    void* ptr2 = core::AllocateBytes(pool, 32);
    void* ptr3 = core::AllocateBytes(pool, 32);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    
    core::DeallocateBytes(pool, ptr2, 32);
    EXPECT_EQ(pool.UsedBlocks(), 2);
    
    core::DeallocateBytes(pool, ptr1, 32);
    EXPECT_EQ(pool.UsedBlocks(), 1);
    
    core::DeallocateBytes(pool, ptr3, 32);
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

// =============================================================================
// Alignment tests
// =============================================================================

TEST(PoolAllocator, AlignmentDefault) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    core::AllocationRequest req{};
    req.size = 32;
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    
    void* ptr = pool.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, CORE_DEFAULT_ALIGNMENT));
}

TEST(PoolAllocator, AlignmentExceedsDefault_Fails) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    core::AllocationRequest req{};
    req.size = 32;
    req.alignment = CORE_DEFAULT_ALIGNMENT * 2;
    
    void* ptr = pool.Allocate(req);
    
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

// =============================================================================
// Size constraint tests
// =============================================================================

TEST(PoolAllocator, AllocateZeroSize) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    core::AllocationRequest req{};
    req.size = 0;
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    
    void* ptr = pool.Allocate(req);
    
    EXPECT_EQ(ptr, nullptr);
}

TEST(PoolAllocator, AllocateExactBlockSize) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    core::AllocationRequest req{};
    req.size = pool.BlockSize();
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    
    void* ptr = pool.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(pool.UsedBlocks(), 1);
}

TEST(PoolAllocator, AllocateTooLarge_Fails) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    core::AllocationRequest req{};
    req.size = pool.BlockSize() + 1;
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    
    void* ptr = pool.Allocate(req);
    
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

// =============================================================================
// Pool exhaustion tests
// =============================================================================

TEST(PoolAllocator, ExhaustPool) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 5;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    std::vector<void*> pointers;
    
    for (core::memory_size i = 0; i < kBlockCount; ++i) {
        void* ptr = core::AllocateBytes(pool, 32);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }
    
    EXPECT_EQ(pool.FreeBlocks(), 0);
    EXPECT_EQ(pool.UsedBlocks(), kBlockCount);
    
    void* overflow_ptr = core::AllocateBytes(pool, 32);
    EXPECT_EQ(overflow_ptr, nullptr);
}

TEST(PoolAllocator, ExhaustAndRefill) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 5;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    std::vector<void*> pointers;
    
    for (core::memory_size i = 0; i < kBlockCount; ++i) {
        void* ptr = core::AllocateBytes(pool, 32);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
    }
    
    EXPECT_EQ(pool.FreeBlocks(), 0);
    
    for (void* ptr : pointers) {
        core::DeallocateBytes(pool, ptr, 32);
    }
    
    EXPECT_EQ(pool.FreeBlocks(), kBlockCount);
    
    for (core::memory_size i = 0; i < kBlockCount; ++i) {
        void* ptr = core::AllocateBytes(pool, 32);
        EXPECT_NE(ptr, nullptr);
    }
}

// =============================================================================
// AllocationFlags tests
// =============================================================================

TEST(PoolAllocator, ZeroInitialize) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    core::AllocationRequest req{};
    req.size = 32;
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    req.flags = core::AllocationFlags::ZeroInitialize;
    
    void* ptr = pool.Allocate(req);
    ASSERT_NE(ptr, nullptr);
    
    core::u8* bytes = static_cast<core::u8*>(ptr);
    for (core::memory_size i = 0; i < req.size; ++i) {
        EXPECT_EQ(bytes[i], 0);
    }
}

TEST(PoolAllocator, ZeroInitialize_OnlyRequestedSize) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 1;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    const core::memory_size small_size = 16;
    constexpr core::u8 kPattern = 0xAB;
    
    void* ptr1 = core::AllocateBytes(pool, pool.BlockSize(), CORE_DEFAULT_ALIGNMENT);
    ASSERT_NE(ptr1, nullptr);
    
    core::u8* bytes = static_cast<core::u8*>(ptr1);
    for (core::memory_size i = 0; i < pool.BlockSize(); ++i) {
        bytes[i] = kPattern;
    }
    
    core::DeallocateBytes(pool, ptr1, pool.BlockSize(), CORE_DEFAULT_ALIGNMENT);
    
    void* ptr2 = core::AllocateBytes(pool, small_size, CORE_DEFAULT_ALIGNMENT, 0, 
                                      core::AllocationFlags::ZeroInitialize);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(ptr1, ptr2);
    
    bytes = static_cast<core::u8*>(ptr2);
    for (core::memory_size i = 0; i < small_size; ++i) {
        EXPECT_EQ(bytes[i], 0) << "Byte at index " << i << " should be zeroed";
    }
    
    for (core::memory_size i = small_size; i < pool.BlockSize(); ++i) {
        EXPECT_EQ(bytes[i], kPattern) << "Byte at index " << i << " should not be touched";
    }
}

TEST(PoolAllocator, ZeroInitialize_ExactBlockSize) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    core::AllocationRequest req{};
    req.size = pool.BlockSize();
    req.alignment = CORE_DEFAULT_ALIGNMENT;
    req.flags = core::AllocationFlags::ZeroInitialize;
    
    void* ptr = pool.Allocate(req);
    ASSERT_NE(ptr, nullptr);
    
    core::u8* bytes = static_cast<core::u8*>(ptr);
    for (core::memory_size i = 0; i < pool.BlockSize(); ++i) {
        EXPECT_EQ(bytes[i], 0) << "Byte at index " << i << " should be zeroed";
    }
}

// =============================================================================
// Owns() tests
// =============================================================================

TEST(PoolAllocator, Owns_ValidPointer) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    void* ptr = core::AllocateBytes(pool, 32);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(pool.Owns(ptr));
}

TEST(PoolAllocator, Owns_Nullptr) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    EXPECT_FALSE(pool.Owns(nullptr));
}

TEST(PoolAllocator, Owns_ExternalPointer) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    void* external_ptr = core::AllocateBytes(sys, 128);
    ASSERT_NE(external_ptr, nullptr);
    
    EXPECT_FALSE(pool.Owns(external_ptr));
    
    core::DeallocateBytes(sys, external_ptr, 128);
}

TEST(PoolAllocator, Owns_AfterDeallocate) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    void* ptr = core::AllocateBytes(pool, 32);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(pool.Owns(ptr));
    
    core::DeallocateBytes(pool, ptr, 32);
    
    EXPECT_TRUE(pool.Owns(ptr));
}

// =============================================================================
// Deallocate edge cases
// =============================================================================

TEST(PoolAllocator, DeallocateNullptr) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    const core::memory_size initial_free = pool.FreeBlocks();
    
    core::AllocationInfo info{};
    info.ptr = nullptr;
    info.size = 32;
    
    pool.Deallocate(info);
    
    EXPECT_EQ(pool.FreeBlocks(), initial_free);
}

// =============================================================================
// Stress tests
// =============================================================================

TEST(PoolAllocator, StressTest_AllocateDeallocatePattern) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 100;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    for (int iteration = 0; iteration < 10; ++iteration) {
        std::vector<void*> pointers;
        
        for (core::memory_size i = 0; i < kBlockCount / 2; ++i) {
            void* ptr = core::AllocateBytes(pool, 32);
            ASSERT_NE(ptr, nullptr);
            pointers.push_back(ptr);
        }
        
        EXPECT_EQ(pool.UsedBlocks(), kBlockCount / 2);
        
        for (void* ptr : pointers) {
            core::DeallocateBytes(pool, ptr, 32);
        }
        
        EXPECT_EQ(pool.UsedBlocks(), 0);
    }
}

TEST(PoolAllocator, StressTest_InterleavedAllocDealloc) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 50;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    std::vector<void*> pointers;
    
    for (int i = 0; i < 100; ++i) {
        if (i % 3 == 0 && !pointers.empty()) {
            size_t idx = pointers.size() / 2;
            core::DeallocateBytes(pool, pointers[idx], 32);
            pointers.erase(pointers.begin() + idx);
        } else if (pointers.size() < kBlockCount) {
            void* ptr = core::AllocateBytes(pool, 32);
            if (ptr != nullptr) {
                for (void* existing : pointers) {
                    ASSERT_NE(ptr, existing) << "Duplicate pointer - free-list corruption!";
                }
                pointers.push_back(ptr);
            }
        }
    }
    
    EXPECT_EQ(pool.UsedBlocks() + pool.FreeBlocks(), kBlockCount);
    
    for (void* ptr : pointers) {
        core::DeallocateBytes(pool, ptr, 32);
    }
    
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

// =============================================================================
// Introspection consistency tests
// =============================================================================

TEST(PoolAllocator, IntrospectionConsistency) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    EXPECT_EQ(pool.BlockCount(), kBlockCount);
    EXPECT_EQ(pool.FreeBlocks() + pool.UsedBlocks(), pool.BlockCount());
    EXPECT_EQ(pool.CapacityBytes(), pool.BlockSize() * pool.BlockCount());
    
    void* ptr1 = core::AllocateBytes(pool, 32);
    void* ptr2 = core::AllocateBytes(pool, 32);
    void* ptr3 = core::AllocateBytes(pool, 32);
    
    EXPECT_EQ(pool.FreeBlocks() + pool.UsedBlocks(), pool.BlockCount());
    EXPECT_EQ(pool.UsedBlocks(), 3);
    
    core::DeallocateBytes(pool, ptr2, 32);
    
    EXPECT_EQ(pool.FreeBlocks() + pool.UsedBlocks(), pool.BlockCount());
    EXPECT_EQ(pool.UsedBlocks(), 2);
    
    core::DeallocateBytes(pool, ptr1, 32);
    core::DeallocateBytes(pool, ptr3, 32);
    
    EXPECT_EQ(pool.FreeBlocks(), pool.BlockCount());
    EXPECT_EQ(pool.UsedBlocks(), 0);
}

TEST(PoolAllocator, BlockSizeMinimum) {
    constexpr core::memory_size kTinyBlockSize = 1;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kTinyBlockSize, kBlockCount, sys);
    
    EXPECT_GE(pool.BlockSize(), sizeof(void*));
}

// =============================================================================
// Memory reuse tests
// =============================================================================

TEST(PoolAllocator, MemoryReuse_SamePointerAfterFree) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 1;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    void* ptr1 = core::AllocateBytes(pool, 32);
    ASSERT_NE(ptr1, nullptr);
    
    core::DeallocateBytes(pool, ptr1, 32);
    
    void* ptr2 = core::AllocateBytes(pool, 32);
    ASSERT_NE(ptr2, nullptr);
    
    EXPECT_EQ(ptr1, ptr2);
}

// =============================================================================
// Block boundary alignment tests
// =============================================================================

TEST(PoolAllocator, BlockBoundaryAlignment_AllPairs) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 10;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    std::vector<void*> pointers;
    std::vector<core::usize> addrs;
    
    for (int i = 0; i < 5; ++i) {
        void* ptr = core::AllocateBytes(pool, 32);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
        addrs.push_back(reinterpret_cast<core::usize>(ptr));
    }
    
    for (size_t i = 0; i < addrs.size(); ++i) {
        for (size_t j = i + 1; j < addrs.size(); ++j) {
            core::memory_size diff = static_cast<core::memory_size>(
                addrs[i] > addrs[j] ? addrs[i] - addrs[j] : addrs[j] - addrs[i]);
            EXPECT_EQ(diff % pool.BlockSize(), 0u) 
                << "Distance between blocks must be multiple of BlockSize";
        }
    }
    
    for (void* ptr : pointers) {
        core::DeallocateBytes(pool, ptr, 32);
    }
}

TEST(PoolAllocator, BlockBoundaryAlignment_UnalignedBuffer) {
    constexpr core::memory_size kBufferSize = 2048;
    constexpr core::memory_size kBlockSize = 64;
    core::u8 buffer[kBufferSize + 32];
    
    core::u8* unaligned = buffer + 11;
    core::PoolAllocator pool(unaligned, kBufferSize, kBlockSize);
    
    ASSERT_GT(pool.BlockCount(), 0);
    
    std::vector<void*> pointers;
    std::vector<core::usize> addrs;
    
    for (int i = 0; i < 5; ++i) {
        void* ptr = core::AllocateBytes(pool, 32);
        ASSERT_NE(ptr, nullptr);
        pointers.push_back(ptr);
        addrs.push_back(reinterpret_cast<core::usize>(ptr));
    }
    
    for (size_t i = 0; i < addrs.size(); ++i) {
        for (size_t j = i + 1; j < addrs.size(); ++j) {
            core::memory_size diff = static_cast<core::memory_size>(
                addrs[i] > addrs[j] ? addrs[i] - addrs[j] : addrs[j] - addrs[i]);
            EXPECT_EQ(diff % pool.BlockSize(), 0u)
                << "Distance between blocks must be multiple of BlockSize (unaligned buffer)";
        }
    }
    
    for (void* ptr : pointers) {
        core::DeallocateBytes(pool, ptr, 32);
    }
}

// =============================================================================
// NoFail death test
// =============================================================================

#if GTEST_HAS_DEATH_TEST
TEST(PoolAllocator, NoFail_ExhaustedPool_Fatal) {
    constexpr core::memory_size kBlockSize = 64;
    constexpr core::memory_size kBlockCount = 2;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    core::PoolAllocator pool(kBlockSize, kBlockCount, sys);
    
    void* ptr1 = core::AllocateBytes(pool, 32);
    void* ptr2 = core::AllocateBytes(pool, 32);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    EXPECT_EQ(pool.FreeBlocks(), 0);
    
    EXPECT_DEATH({
        core::AllocationRequest req{};
        req.size = 32;
        req.alignment = CORE_DEFAULT_ALIGNMENT;
        req.flags = core::AllocationFlags::NoFail;
        pool.Allocate(req);
    }, ".*PoolAllocator.*out of memory.*");
}
#endif

} // anonymous namespace

