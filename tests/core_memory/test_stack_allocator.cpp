#include "core/memory/stack_allocator.hpp"
#include "core/memory/system_allocator.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/memory/core_memory.hpp"
#include <gtest/gtest.h>

namespace {

// =============================================================================
// Constructor Tests
// =============================================================================

TEST(StackAllocator, NonOwningConstructor_ValidBuffer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    
    core::StackAllocator allocator(buffer, kBufferSize);
    
    EXPECT_EQ(allocator.Capacity(), kBufferSize);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kBufferSize);
}

TEST(StackAllocator, NonOwningConstructor_NullBuffer) {
    core::StackAllocator allocator(nullptr, 1024);
    
    EXPECT_EQ(allocator.Capacity(), 0);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), 0);
}

TEST(StackAllocator, NonOwningConstructor_ZeroSize) {
    core::u8 buffer[128];
    core::StackAllocator allocator(buffer, 0);
    
    EXPECT_EQ(allocator.Capacity(), 0);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), 0);
}

TEST(StackAllocator, OwningConstructor_ValidCapacity) {
    constexpr core::memory_size kCapacity = 2048;
    core::SystemAllocator& sys = core::SystemAllocator::Instance();
    
    core::StackAllocator allocator(kCapacity, sys);
    
    EXPECT_EQ(allocator.Capacity(), kCapacity);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kCapacity);
}

// =============================================================================
// Basic LIFO Allocation/Deallocation Tests
// =============================================================================

TEST(StackAllocator, SimpleAllocation) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 16;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    
    const core::memory_alignment header_align = 
        static_cast<core::memory_alignment>(alignof(core::detail::StackAllocationHeader));
    const core::memory_alignment effective_align = 
        req.alignment > header_align ? req.alignment : header_align;
    EXPECT_TRUE(core::IsAlignedPtr(ptr, effective_align));
    EXPECT_TRUE(allocator.Owns(ptr));
    EXPECT_GT(allocator.Used(), 0);
    EXPECT_LT(allocator.Remaining(), kBufferSize);
}

TEST(StackAllocator, SimpleAllocationAndDeallocation_LIFO) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr, nullptr);
    
    core::memory_size used_after_alloc = allocator.Used();
    EXPECT_GT(used_after_alloc, 0);
    
    core::DeallocateBytes(allocator, ptr, 100, 8);
    
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kBufferSize);
}

TEST(StackAllocator, MultipleLIFO_Allocations) {
    constexpr core::memory_size kBufferSize = 2048;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr1, nullptr);
    core::memory_size used1 = allocator.Used();
    
    void* ptr2 = core::AllocateBytes(allocator, 200, 8);
    ASSERT_NE(ptr2, nullptr);
    core::memory_size used2 = allocator.Used();
    EXPECT_GT(used2, used1);
    
    void* ptr3 = core::AllocateBytes(allocator, 50, 8);
    ASSERT_NE(ptr3, nullptr);
    core::memory_size used3 = allocator.Used();
    EXPECT_GT(used3, used2);
    
    core::DeallocateBytes(allocator, ptr3, 50, 8);
    EXPECT_EQ(allocator.Used(), used2);
    
    core::DeallocateBytes(allocator, ptr2, 200, 8);
    EXPECT_EQ(allocator.Used(), used1);
    
    core::DeallocateBytes(allocator, ptr1, 100, 8);
    EXPECT_EQ(allocator.Used(), 0);
}

// =============================================================================
// Alignment Tests
// =============================================================================

TEST(StackAllocator, AlignmentCorrectness_Align1) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 17;
    req.alignment = 1;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    const core::memory_alignment header_align = 
        static_cast<core::memory_alignment>(alignof(core::detail::StackAllocationHeader));
    const core::memory_alignment effective_align = 
        req.alignment > header_align ? req.alignment : header_align;
    EXPECT_TRUE(core::IsAlignedPtr(ptr, effective_align));
}

TEST(StackAllocator, AlignmentCorrectness_Align4) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 17;
    req.alignment = 4;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    const core::memory_alignment header_align = 
        static_cast<core::memory_alignment>(alignof(core::detail::StackAllocationHeader));
    const core::memory_alignment effective_align = 
        req.alignment > header_align ? req.alignment : header_align;
    EXPECT_TRUE(core::IsAlignedPtr(ptr, effective_align));
}

TEST(StackAllocator, AlignmentCorrectness_Align8) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 17;
    req.alignment = 8;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    const core::memory_alignment header_align = 
        static_cast<core::memory_alignment>(alignof(core::detail::StackAllocationHeader));
    const core::memory_alignment effective_align = 
        req.alignment > header_align ? req.alignment : header_align;
    EXPECT_TRUE(core::IsAlignedPtr(ptr, effective_align));
}

TEST(StackAllocator, AlignmentCorrectness_Align16) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(32) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 17;
    req.alignment = 16;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 16));
}

TEST(StackAllocator, AlignmentCorrectness_Align32) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(32) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 17;
    req.alignment = 32;
    
    void* ptr = allocator.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 32));
}

TEST(StackAllocator, MultipleAllocations_MixedAlignment) {
    constexpr core::memory_size kBufferSize = 2048;
    alignas(32) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    const core::memory_alignment header_align = 
        static_cast<core::memory_alignment>(alignof(core::detail::StackAllocationHeader));
    
    void* ptr1 = core::AllocateBytes(allocator, 10, 1);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr1, header_align));
    
    void* ptr2 = core::AllocateBytes(allocator, 20, 16);
    ASSERT_NE(ptr2, nullptr);
    const core::memory_alignment effective_align2 = 16 > header_align ? 16 : header_align;
    EXPECT_TRUE(core::IsAlignedPtr(ptr2, effective_align2));
    
    void* ptr3 = core::AllocateBytes(allocator, 15, 4);
    ASSERT_NE(ptr3, nullptr);
    const core::memory_alignment effective_align3 = 4 > header_align ? 4 : header_align;
    EXPECT_TRUE(core::IsAlignedPtr(ptr3, effective_align3));
    core::DeallocateBytes(allocator, ptr3, 15, 4);
    core::DeallocateBytes(allocator, ptr2, 20, 16);
    core::DeallocateBytes(allocator, ptr1, 10, 1);
    
    EXPECT_EQ(allocator.Used(), 0);
}

// =============================================================================
// Marker/Scope Tests
// =============================================================================

TEST(StackAllocator, GetMarker_InitialState) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    auto marker = allocator.GetMarker();
    EXPECT_NE(marker.position, nullptr);
    EXPECT_EQ(marker.position, buffer);
}

TEST(StackAllocator, MarkerRewind_SingleScope) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    auto marker = allocator.GetMarker();
    EXPECT_EQ(allocator.Used(), 0);
    
    void* ptr1 = core::AllocateBytes(allocator, 100, 8);
    void* ptr2 = core::AllocateBytes(allocator, 200, 8);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    core::memory_size used = allocator.Used();
    EXPECT_GT(used, 0);
    
    allocator.RewindToMarker(marker);
    EXPECT_EQ(allocator.Used(), 0);
}

TEST(StackAllocator, MarkerRewind_NestedScopes) {
    constexpr core::memory_size kBufferSize = 2048;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    // Outer scope
    auto marker1 = allocator.GetMarker();
    void* ptr1 = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr1, nullptr);
    core::memory_size used1 = allocator.Used();
    
    // Inner scope 1
    {
        auto marker2 = allocator.GetMarker();
        void* ptr2 = core::AllocateBytes(allocator, 200, 8);
        ASSERT_NE(ptr2, nullptr);
        core::memory_size used2 = allocator.Used();
        EXPECT_GT(used2, used1);
        
        // Inner scope 2
        {
            auto marker3 = allocator.GetMarker();
            void* ptr3 = core::AllocateBytes(allocator, 50, 8);
            ASSERT_NE(ptr3, nullptr);
            core::memory_size used3 = allocator.Used();
            EXPECT_GT(used3, used2);
            
            // Exit inner scope 2
            allocator.RewindToMarker(marker3);
            EXPECT_EQ(allocator.Used(), used2);
        }
        
        // Exit inner scope 1
        allocator.RewindToMarker(marker2);
        EXPECT_EQ(allocator.Used(), used1);
    }
    
    // Exit outer scope
    allocator.RewindToMarker(marker1);
    EXPECT_EQ(allocator.Used(), 0);
}

TEST(StackAllocator, MarkerComparison) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    auto marker1 = allocator.GetMarker();
    core::AllocateBytes(allocator, 100, 8);
    auto marker2 = allocator.GetMarker();
    
    EXPECT_NE(marker1, marker2);
    EXPECT_EQ(marker1, marker1);
    EXPECT_EQ(marker2, marker2);
}

// =============================================================================
// Reset Tests
// =============================================================================

TEST(StackAllocator, Reset_FreesAllMemory) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocateBytes(allocator, 100, 8);
    core::AllocateBytes(allocator, 200, 8);
    core::AllocateBytes(allocator, 50, 8);
    
    EXPECT_GT(allocator.Used(), 0);
    
    allocator.Reset();
    
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), kBufferSize);
}

TEST(StackAllocator, Reset_UninitializedAllocator) {
    core::StackAllocator allocator(nullptr, 0);
    
    allocator.Reset();  // Should not crash
    
    EXPECT_EQ(allocator.Used(), 0);
}

// =============================================================================
// Capacity Exhaustion Tests
// =============================================================================

TEST(StackAllocator, Allocation_CapacityExhausted) {
    constexpr core::memory_size kBufferSize = 256;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 200, 8);
    ASSERT_NE(ptr1, nullptr);
    
    // This should fail - not enough space
    void* ptr2 = core::AllocateBytes(allocator, 200, 8);
    EXPECT_EQ(ptr2, nullptr);
}

TEST(StackAllocator, Allocation_ExactCapacity) {
    constexpr core::memory_size kBufferSize = 256;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    // Calculate worst-case overhead: header + alignment padding
    const core::memory_size header_size = sizeof(core::detail::StackAllocationHeader);
    const core::memory_alignment effective_align = 
        static_cast<core::memory_alignment>(alignof(core::detail::StackAllocationHeader));
    const core::memory_size worst_overhead = header_size + (effective_align - 1);
    
    const core::memory_size usable_size = kBufferSize - worst_overhead;
    
    void* ptr = core::AllocateBytes(allocator, usable_size, effective_align);
    ASSERT_NE(ptr, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 1, effective_align);
    EXPECT_EQ(ptr2, nullptr);
}

TEST(StackAllocator, Allocation_AfterRewindHasSpace) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 400, 8);
    ASSERT_NE(ptr1, nullptr);
    
    // Not enough space
    void* ptr2 = core::AllocateBytes(allocator, 200, 8);
    EXPECT_EQ(ptr2, nullptr);
    
    // Rewind and try again
    core::DeallocateBytes(allocator, ptr1, 400, 8);
    
    void* ptr3 = core::AllocateBytes(allocator, 200, 8);
    EXPECT_NE(ptr3, nullptr);
}

// =============================================================================
// Introspection Tests (used, capacity, remaining)
// =============================================================================

TEST(StackAllocator, Introspection_InitialState) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Capacity(), kBufferSize);
    EXPECT_EQ(allocator.Remaining(), kBufferSize);
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
}

TEST(StackAllocator, Introspection_AfterAllocations) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocateBytes(allocator, 100, 8);
    
    EXPECT_GT(allocator.Used(), 0);
    EXPECT_EQ(allocator.Capacity(), kBufferSize);
    EXPECT_LT(allocator.Remaining(), kBufferSize);
    EXPECT_EQ(allocator.Used() + allocator.Remaining(), allocator.Capacity());
}

TEST(StackAllocator, Introspection_AfterDeallocations) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr, nullptr);
    
    core::memory_size used_before = allocator.Used();
    
    core::DeallocateBytes(allocator, ptr, 100, 8);
    
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_LT(allocator.Used(), used_before);
}

// =============================================================================
// Owns() Tests
// =============================================================================

TEST(StackAllocator, Owns_ValidPointer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(allocator.Owns(ptr));
}

TEST(StackAllocator, Owns_NullPointer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    EXPECT_FALSE(allocator.Owns(nullptr));
}

TEST(StackAllocator, Owns_ExternalPointer) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    int external = 42;
    EXPECT_FALSE(allocator.Owns(&external));
}

// =============================================================================
// AllocationFlags Tests
// =============================================================================

TEST(StackAllocator, ZeroInitialize_Flag) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    constexpr core::memory_size kAllocSize = 64;
    void* ptr = core::AllocateBytes(allocator, kAllocSize, 8, 0, 
                                     core::AllocationFlags::ZeroInitialize);
    
    ASSERT_NE(ptr, nullptr);
    
    // Check that memory is zeroed
    const core::u8* bytes = static_cast<const core::u8*>(ptr);
    for (core::memory_size i = 0; i < kAllocSize; ++i) {
        EXPECT_EQ(bytes[i], 0) << "Byte at index " << i << " is not zero";
    }
}

TEST(StackAllocator, ZeroSize_Allocation) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 0;
    req.alignment = 8;
    
    void* ptr = allocator.Allocate(req);
    
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(allocator.Used(), 0);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(StackAllocator, Deallocate_Nullptr) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationInfo info{};
    info.ptr = nullptr;
    info.size = 0;
    
    allocator.Deallocate(info);  // Should not crash
    
    EXPECT_EQ(allocator.Used(), 0);
}

TEST(StackAllocator, UninitializedAllocator_Operations) {
    core::StackAllocator allocator(nullptr, 0);
    
    EXPECT_EQ(allocator.Capacity(), 0);
    EXPECT_EQ(allocator.Used(), 0);
    EXPECT_EQ(allocator.Remaining(), 0);
    
    void* ptr = core::AllocateBytes(allocator, 100, 8);
    EXPECT_EQ(ptr, nullptr);
    
    EXPECT_FALSE(allocator.Owns(nullptr));
}

// =============================================================================
// Debug Assertion Tests (LIFO violations)
// =============================================================================

#if defined(CORE_MEMORY_DEBUG) && CORE_MEMORY_DEBUG && defined(GTEST_HAS_DEATH_TEST) && GTEST_HAS_DEATH_TEST

TEST(StackAllocator, Debug_LIFO_Violation_OutOfOrder) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr1 = core::AllocateBytes(allocator, 100, 8);
    void* ptr2 = core::AllocateBytes(allocator, 200, 8);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    EXPECT_DEATH({
        core::DeallocateBytes(allocator, ptr1, 100, 8);
    }, ".*");
}

TEST(StackAllocator, Debug_InvalidPointer_NotOwned) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    int external = 42;
    
    EXPECT_DEATH({
        core::AllocationInfo info{};
        info.ptr = &external;
        info.size = sizeof(int);
        allocator.Deallocate(info);
    }, ".*");
}

TEST(StackAllocator, Debug_InvalidAlignment_NotPowerOfTwo) {
    constexpr core::memory_size kBufferSize = 512;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 3;
    
    EXPECT_DEATH({
        allocator.Allocate(req);
    }, ".*");
}

TEST(StackAllocator, Debug_InvalidMarker_OutOfBounds) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    core::StackAllocator::Marker invalid_marker;
    invalid_marker.position = buffer + kBufferSize + 1;
    
    EXPECT_DEATH({
        allocator.RewindToMarker(invalid_marker);
    }, ".*");
}

TEST(StackAllocator, Debug_SizeMismatch_InfoVsHeader) {
    constexpr core::memory_size kBufferSize = 1024;
    alignas(16) core::u8 buffer[kBufferSize];
    core::StackAllocator allocator(buffer, kBufferSize);
    
    void* ptr = core::AllocateBytes(allocator, 100, 8);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_DEATH({
        core::DeallocateBytes(allocator, ptr, 200, 8);
    }, ".*");
}

#endif // CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST

} // namespace

