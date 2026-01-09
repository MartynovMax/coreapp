#include "core/memory/bump_allocator.hpp"
#include "core/memory/system_allocator.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/memory/core_memory.hpp"
#include <gtest/gtest.h>

namespace {

TEST(BumpAllocator, NonOwningConstructor_ValidBuffer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    EXPECT_EQ(allocator.Capacity(), kBufferSize);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kBufferSize);
}

TEST(BumpAllocator, NonOwningConstructor_NullBuffer) {
    core::BumpAllocator allocator(nullptr, 1024);
    
    EXPECT_EQ(allocator.Capacity(), 0);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), 0);
}

TEST(BumpAllocator, NonOwningConstructor_ZeroSize) {
    core::u8 buffer[128];
    core::BumpAllocator allocator(buffer, 0);
    
    EXPECT_EQ(allocator.Capacity(), 0);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), 0);
}

TEST(BumpAllocator, OwningConstructor_ValidCapacity) {
    constexpr core::memory_size kCapacity = 2048;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::BumpAllocator allocator(kCapacity, sys);
    
    EXPECT_EQ(allocator.Capacity(), kCapacity);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kCapacity);
}

TEST(BumpAllocator, SimpleAllocation) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 16;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, req.alignment));
    EXPECT_EQ(allocator.Used(), 64);
    EXPECT_EQ(allocator.Remaining(), kBufferSize - 64);
}

TEST(BumpAllocator, MultipleAllocations) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::memory_size used_before = allocator.Used();
    
    void* ptr1 = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_GT(allocator.Used(), used_before);
    core::memory_size after_first = allocator.Used();
    
    void* ptr2 = core::AllocateBytes(allocator, 200, 8);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_GT(allocator.Used(), after_first);
    core::memory_size after_second = allocator.Used();
    
    void* ptr3 = core::AllocateBytes(allocator, 50, 8);
    ASSERT_NE(ptr3, nullptr);
    EXPECT_GT(allocator.Used(), after_second);
    
    EXPECT_NE(ptr1, ptr2);
    EXPECT_NE(ptr2, ptr3);
    EXPECT_NE(ptr1, ptr3);
    
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
}

TEST(BumpAllocator, AlignmentCorrectness_Align4) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 17;
    req.alignment = 4;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 4));
}

TEST(BumpAllocator, AlignmentCorrectness_Align8) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 5, 1);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 10, 8);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr2, 8));
    EXPECT_NE(ptr1, ptr2);
}

TEST(BumpAllocator, AlignmentCorrectness_Align16) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(32) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 7, 1);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 32, 16);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr2, 16));
    EXPECT_NE(ptr1, ptr2);
}

TEST(BumpAllocator, AlignmentCorrectness_Align32) {
    constexpr core::memory_size kBufferSize = 2048;
    alignas(64) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 13, 1);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 64, 32);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr2, 32));
    EXPECT_NE(ptr1, ptr2);
}

TEST(BumpAllocator, AlignmentCorrectness_Align64) {
    constexpr core::memory_size kBufferSize = 2048;
    alignas(64) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 128, 64);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 64));
}

TEST(BumpAllocator, CapacityExhaustion_ExactFit) {
    constexpr core::memory_size kBufferSize = 256;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, kBufferSize, 16);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 16));
    EXPECT_EQ(allocator.Remaining(), 0);
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
    
    void* ptr2 = core::AllocateBytes(allocator, 1, 1);
    EXPECT_EQ(ptr2, nullptr);
}

TEST(BumpAllocator, CapacityExhaustion_ReturnsNullptr) {
    constexpr core::memory_size kBufferSize = 128;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 100, 1);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 50, 1);
    EXPECT_EQ(ptr2, nullptr);
}

TEST(BumpAllocator, CapacityExhaustion_WithAlignment) {
    constexpr core::memory_size kBufferSize = 100;
    alignas(32) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 80, 1);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 16, 32);
    EXPECT_EQ(ptr2, nullptr);
}

TEST(BumpAllocator, Reset_ClearsUsedMemory) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(allocator.Used(), 100);
    
    allocator.Reset();
    
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kBufferSize);
}

TEST(BumpAllocator, Reset_AllowsReuse) {
    constexpr core::memory_size kBufferSize = 256;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 200, 8);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 100, 8);
    EXPECT_EQ(ptr2, nullptr);
    
    allocator.Reset();
    
    void* ptr3 = core::AllocateBytes(allocator, 200, 8);
    ASSERT_NE(ptr3, nullptr);
    EXPECT_EQ(ptr1, ptr3);
}

TEST(BumpAllocator, Reset_MultipleRounds) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    for (int round = 0; round < 5; ++round) {
        core::memory_size start_used = allocator.Used();
        EXPECT_EQ(start_used, 0);
        
        void* ptr1 = core::AllocateBytes(allocator, 100, 8);
        ASSERT_NE(ptr1, nullptr);
        EXPECT_GT(allocator.Used(), start_used);
        
        void* ptr2 = core::AllocateBytes(allocator, 100, 8);
        ASSERT_NE(ptr2, nullptr);
        
        void* ptr3 = core::AllocateBytes(allocator, 100, 8);
        ASSERT_NE(ptr3, nullptr);
        
        EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
        
        allocator.Reset();
        EXPECT_EQ(allocator.Used(), 0);
        EXPECT_EQ(allocator.Remaining(), allocator.Capacity());
    }
}

TEST(BumpAllocator, UsedCapacityRemaining_InitialState) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    EXPECT_EQ(allocator.Capacity(), kBufferSize);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kBufferSize);
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
}

TEST(BumpAllocator, UsedCapacityRemaining_AfterAllocation) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::AllocateBytes(allocator, 256, 8);
    
    EXPECT_EQ(allocator.Capacity(), kBufferSize);
    EXPECT_EQ(allocator.Used(), 256);
    EXPECT_EQ(allocator.Remaining(), kBufferSize - 256);
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
}

TEST(BumpAllocator, UsedCapacityRemaining_MultipleAllocations) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::memory_size used1 = allocator.Used();
    core::AllocateBytes(allocator, 100, 8);
    EXPECT_GT(allocator.Used(), used1);
    
    core::memory_size used2 = allocator.Used();
    core::AllocateBytes(allocator, 200, 8);
    EXPECT_GT(allocator.Used(), used2);
    
    core::memory_size used3 = allocator.Used();
    core::AllocateBytes(allocator, 150, 8);
    EXPECT_GT(allocator.Used(), used3);
    
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
    EXPECT_LT(allocator.Remaining(), kBufferSize);
}

TEST(BumpAllocator, Owns_ValidPointer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 64, 8);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(allocator.Owns(ptr));
}

TEST(BumpAllocator, Owns_InvalidPointer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    int external_var = 42;
    EXPECT_FALSE(allocator.Owns(&external_var));
}

TEST(BumpAllocator, Owns_NullPointer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    EXPECT_FALSE(allocator.Owns(nullptr));
}

TEST(BumpAllocator, Owns_UninitializedAllocator) {
    core::BumpAllocator allocator(nullptr, 0);
    
    int some_var = 42;
    EXPECT_FALSE(allocator.Owns(&some_var));
    EXPECT_FALSE(allocator.Owns(nullptr));
}

TEST(BumpAllocator, Deallocate_IsNoOp) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr, nullptr);
    
    core::memory_size used_before = allocator.Used();
    
    core::DeallocateBytes(allocator, ptr, 100, 8);
    
    EXPECT_EQ(allocator.Used(), used_before);
}

TEST(BumpAllocator, ZeroInitialize_Flag) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    
    for (core::memory_size i = 0; i < kBufferSize; ++i) {
        buffer[i] = 0xFF;
    }
    
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 128;
    req.alignment = 8;
    req.flags = core::AllocationFlags::ZeroInitialize;
    
    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);
    
    const core::u8* bytes = static_cast<const core::u8*>(ptr);
    for (core::memory_size i = 0; i < req.size; ++i) {
        EXPECT_EQ(bytes[i], 0) << "Byte at index " << i << " is not zero";
    }
}

TEST(BumpAllocator, ZeroSizeAllocation) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 0;
    req.alignment = 8;
    
    core::memory_size used_before = allocator.Used();
    void* ptr = allocator.Allocate(req);
    
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(allocator.Used(), used_before);
    
    void* ptr2 = core::AllocateBytes(allocator, 64, 8);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_GT(allocator.Used(), used_before);
    
    void* ptr3 = allocator.Allocate(req);
    EXPECT_EQ(ptr3, nullptr);
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
}

TEST(BumpAllocator, UninitializedAllocator_Allocate) {
    core::BumpAllocator allocator(nullptr, 0);
    
    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 8;
    
    void* ptr = allocator.Allocate(req);
    EXPECT_EQ(ptr, nullptr);
}

TEST(BumpAllocator, OwningMode_MemoryReturnedToUpstream) {
    class TestUpstreamAllocator final : public core::IAllocator {
    public:
        void* Allocate(const core::AllocationRequest& request) noexcept override {
            alloc_count_++;
            core::SystemAllocator& sys = core::SystemAllocator::Instance();
            return sys.Allocate(request);
        }
        
        void Deallocate(const core::AllocationInfo& info) noexcept override {
            dealloc_count_++;
            core::SystemAllocator& sys = core::SystemAllocator::Instance();
            sys.Deallocate(info);
        }
        
        core::memory_size alloc_count() const noexcept { return alloc_count_; }
        core::memory_size dealloc_count() const noexcept { return dealloc_count_; }
        
    private:
        core::memory_size alloc_count_ = 0;
        core::memory_size dealloc_count_ = 0;
    };
    
    TestUpstreamAllocator upstream;
    
    {
        core::BumpAllocator allocator(2048, upstream);
        EXPECT_EQ(upstream.alloc_count(), 1);
        EXPECT_EQ(upstream.dealloc_count(), 0);
        
        void* ptr = core::AllocateBytes(allocator, 256, 8);
        EXPECT_NE(ptr, nullptr);
    }
    
    EXPECT_EQ(upstream.alloc_count(), 1);
    EXPECT_EQ(upstream.dealloc_count(), 1);
}

TEST(BumpAllocator, OwningMode_FailedAllocation) {
    class FailingAllocator final : public core::IAllocator {
    public:
        void* Allocate(const core::AllocationRequest&) noexcept override {
            return nullptr;
        }
        
        void Deallocate(const core::AllocationInfo&) noexcept override {
        }
    };
    
    FailingAllocator failing;
    core::BumpAllocator allocator(1024, failing);
    
    EXPECT_EQ(allocator.Capacity(), 0);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), 0);
    
    void* ptr = core::AllocateBytes(allocator, 64, 8);
    EXPECT_EQ(ptr, nullptr);
}

TEST(BumpAllocator, ZeroAlignment_Normalized) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 0;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    // NormalizeAlignment(0) -> CORE_DEFAULT_ALIGNMENT per contract
    EXPECT_TRUE(core::IsAlignedPtr(ptr, CORE_DEFAULT_ALIGNMENT));
}

TEST(BumpAllocator, ZeroInitialize_ViaHelper) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    
    for (core::memory_size i = 0; i < kBufferSize; ++i) {
        buffer[i] = 0xFF;
    }
    
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 128, 8, 0, core::AllocationFlags::ZeroInitialize);
    ASSERT_NE(ptr, nullptr);
    
    const core::u8* bytes = static_cast<const core::u8*>(ptr);
    for (core::memory_size i = 0; i < 128; ++i) {
        EXPECT_EQ(bytes[i], 0) << "Byte at index " << i << " is not zero";
    }
}

#if defined(CORE_MEMORY_DEBUG) && CORE_MEMORY_DEBUG && defined(GTEST_HAS_DEATH_TEST) && GTEST_HAS_DEATH_TEST
TEST(BumpAllocator, InvalidAlignment_NotPowerOfTwo) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::BumpAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 3;
    
    ASSERT_DEATH({
        allocator.Allocate(req);
    }, ".*");
}
#endif

} // anonymous namespace

