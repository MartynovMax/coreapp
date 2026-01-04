#include "core/memory/core_allocator.hpp"
#include <gtest/gtest.h>
#include <type_traits>

namespace {

class TestAllocator final : public core::IAllocator {
public:
    void* Allocate(const core::AllocationRequest& request) noexcept override {
        // Simple linear allocator for testing
        buffer_used_ = core::AlignUp(buffer_used_, request.alignment);
        if (buffer_used_ + request.size > kBufferSize) {
            return nullptr;
        }
        void* p = core::AddBytes(buffer_, buffer_used_);
        buffer_used_ += request.size;
        last_tag_ = request.tag;
        alloc_count_++;
        return p;
    }

    void Deallocate(const core::AllocationInfo& /*info*/) noexcept override {
        // Linear allocator doesn't support individual frees
        free_count_++;
    }

    bool TryGetStats(core::AllocatorStats& out) const noexcept override {
        out = {};
        out.bytes_allocated_current = buffer_used_;
        out.bytes_allocated_peak = buffer_used_;
        out.bytes_allocated_total = buffer_used_;
        out.alloc_count_total = alloc_count_;
        out.free_count_total = free_count_;
        return true;
    }

    core::memory_tag last_tag() const noexcept { return last_tag_; }
    core::memory_size alloc_count() const noexcept { return alloc_count_; }
    core::memory_size free_count() const noexcept { return free_count_; }

private:
    static constexpr core::memory_size kBufferSize = 4096;
    alignas(64) core::byte buffer_[kBufferSize]{};
    core::memory_size buffer_used_ = 0;
    core::memory_tag last_tag_ = 0;
    core::memory_size alloc_count_ = 0;
    core::memory_size free_count_ = 0;
};

} // namespace

// ----------------------------------------------------------------------------
// IAllocator interface tests
// ----------------------------------------------------------------------------

TEST(CoreAllocator, IAllocatorBasicInterface) {
    TestAllocator a;

    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 16;
    req.tag = 42;
    req.flags = core::AllocationFlags::None;

    void* p = a.Allocate(req);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p, req.alignment));

    core::AllocatorStats stats{};
    EXPECT_TRUE(a.TryGetStats(stats));
    EXPECT_GE(stats.bytes_allocated_current, req.size);
    EXPECT_EQ(stats.alloc_count_total, 1u);

    core::AllocationInfo info{};
    info.ptr = p;
    info.size = req.size;
    info.alignment = req.alignment;
    info.tag = req.tag;
    a.Deallocate(info);

    EXPECT_EQ(a.last_tag(), 42u);
    EXPECT_EQ(a.free_count(), 1u);
}

TEST(CoreAllocator, AllocationFlags) {
    using core::AllocationFlags;
    
    // Test flag operations
    auto flags = AllocationFlags::Transient | AllocationFlags::ZeroInitialize;
    EXPECT_TRUE(core::Any(flags & AllocationFlags::Transient));
    EXPECT_TRUE(core::Any(flags & AllocationFlags::ZeroInitialize));
    EXPECT_FALSE(core::Any(flags & AllocationFlags::Persistent));
    EXPECT_FALSE(core::Any(flags & AllocationFlags::NoFail));
}

TEST(CoreAllocator, AllocationRequestDefaultValues) {
    core::AllocationRequest req{};
    
    EXPECT_EQ(req.size, 0u);
    EXPECT_EQ(req.alignment, CORE_DEFAULT_ALIGNMENT);
    EXPECT_EQ(req.tag, 0u);
    EXPECT_EQ(req.flags, core::AllocationFlags::None);
}

TEST(CoreAllocator, AllocatorStatsDefaultValues) {
    core::AllocatorStats stats{};
    
    EXPECT_EQ(stats.bytes_allocated_current, 0u);
    EXPECT_EQ(stats.bytes_allocated_peak, 0u);
    EXPECT_EQ(stats.bytes_allocated_total, 0u);
    EXPECT_EQ(stats.alloc_count_total, 0u);
    EXPECT_EQ(stats.free_count_total, 0u);
}

// ----------------------------------------------------------------------------
// Typed allocation helpers tests
// ----------------------------------------------------------------------------

TEST(CoreAllocator, AllocateObjectAndDeallocate) {
    TestAllocator a;

    int* ip = core::AllocateObject<int>(a);
    ASSERT_NE(ip, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ip, static_cast<core::memory_alignment>(alignof(int))));
    
    // Use the memory
    *ip = 42;
    EXPECT_EQ(*ip, 42);
    
    core::DeallocateObject<int>(a, ip);
    EXPECT_EQ(a.alloc_count(), 1u);
    EXPECT_EQ(a.free_count(), 1u);
}

TEST(CoreAllocator, AllocateArrayAndDeallocate) {
    TestAllocator a;

    constexpr core::memory_size count = 10;
    double* dp = core::AllocateArray<double>(a, count);
    ASSERT_NE(dp, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(dp, static_cast<core::memory_alignment>(alignof(double))));
    
    // Use the array
    for (core::memory_size i = 0; i < count; ++i) {
        dp[i] = static_cast<double>(i) * 3.14;
    }
    EXPECT_DOUBLE_EQ(dp[0], 0.0);
    EXPECT_DOUBLE_EQ(dp[5], 5.0 * 3.14);
    
    core::DeallocateArray<double>(a, dp, count);
    EXPECT_EQ(a.alloc_count(), 1u);
    EXPECT_EQ(a.free_count(), 1u);
}

TEST(CoreAllocator, DeallocateNullptrIsNoOp) {
    TestAllocator a;
    
    // Should not crash
    core::DeallocateObject<int>(a, nullptr);
    core::DeallocateArray<double>(a, nullptr, 10);
    
    EXPECT_EQ(a.alloc_count(), 0u);
    EXPECT_EQ(a.free_count(), 0u);
}

TEST(CoreAllocator, AllocationWithTag) {
    TestAllocator a;

    constexpr core::memory_tag test_tag = 123;
    int* ip = core::AllocateObject<int>(a, test_tag);
    ASSERT_NE(ip, nullptr);
    
    EXPECT_EQ(a.last_tag(), test_tag);
    
    core::DeallocateObject<int>(a, ip, test_tag);
}

TEST(CoreAllocator, AllocationWithFlags) {
    TestAllocator a;

    auto flags = core::AllocationFlags::Transient | core::AllocationFlags::ZeroInitialize;
    int* ip = core::AllocateObject<int>(a, 0, flags);
    ASSERT_NE(ip, nullptr);
    
    core::DeallocateObject<int>(a, ip);
}

TEST(CoreAllocator, MultipleAllocations) {
    TestAllocator a;

    int* i1 = core::AllocateObject<int>(a);
    int* i2 = core::AllocateObject<int>(a);
    double* d1 = core::AllocateObject<double>(a);
    
    ASSERT_NE(i1, nullptr);
    ASSERT_NE(i2, nullptr);
    ASSERT_NE(d1, nullptr);
    
    // All should be distinct
    EXPECT_NE(i1, i2);
    EXPECT_NE(static_cast<void*>(i1), static_cast<void*>(d1));
    EXPECT_NE(static_cast<void*>(i2), static_cast<void*>(d1));
    
    EXPECT_EQ(a.alloc_count(), 3u);
    
    core::DeallocateObject(a, i1);
    core::DeallocateObject(a, i2);
    core::DeallocateObject(a, d1);
    
    EXPECT_EQ(a.free_count(), 3u);
}

// ----------------------------------------------------------------------------
// Allocator adapters tests
// ----------------------------------------------------------------------------

TEST(CoreAllocator, AllocatorRefBasic) {
    TestAllocator a;
    
    core::AllocatorRef ref{a};
    EXPECT_TRUE(ref);
    EXPECT_EQ(ref.Ptr(), &a);
    EXPECT_EQ(&ref.Get(), &a);
    
    // Use through reference
    core::AllocationRequest req{};
    req.size = 32;
    req.alignment = 8;
    
    void* p = ref->Allocate(req);
    ASSERT_NE(p, nullptr);
    
    core::AllocationInfo info{};
    info.ptr = p;
    info.size = req.size;
    info.alignment = req.alignment;
    
    ref->Deallocate(info);
    EXPECT_EQ(a.free_count(), 1u);
}

TEST(CoreAllocator, AllocatorRefDefaultConstructor) {
    core::AllocatorRef ref;
    EXPECT_FALSE(ref);
    EXPECT_EQ(ref.Ptr(), nullptr);
}

TEST(CoreAllocator, AllocatorRefFromPointer) {
    TestAllocator a;
    core::AllocatorRef ref{&a};
    
    EXPECT_TRUE(ref);
    EXPECT_EQ(ref.Ptr(), &a);
}

TEST(CoreAllocator, TypedAllocatorAdapter) {
    TestAllocator a;
    
    core::TypedAllocator<int> ta{a};
    EXPECT_EQ(ta.Resource(), &a);
    
    // Allocate array of 5 ints
    int* arr = ta.Allocate(5);
    ASSERT_NE(arr, nullptr);
    
    // Use the array
    for (int i = 0; i < 5; ++i) {
        arr[i] = i * 10;
    }
    EXPECT_EQ(arr[0], 0);
    EXPECT_EQ(arr[4], 40);
    
    ta.Deallocate(arr, 5);
    EXPECT_EQ(a.alloc_count(), 1u);
    EXPECT_EQ(a.free_count(), 1u);
}

TEST(CoreAllocator, TypedAllocatorFromAllocatorRef) {
    TestAllocator a;
    core::AllocatorRef ref{a};
    
    core::TypedAllocator<double> ta{ref};
    EXPECT_EQ(ta.Resource(), &a);
    
    double* p = ta.Allocate(1);
    ASSERT_NE(p, nullptr);
    *p = 3.14159;
    EXPECT_DOUBLE_EQ(*p, 3.14159);
    
    ta.Deallocate(p, 1);
}

TEST(CoreAllocator, TypedAllocatorDefaultConstructor) {
    core::TypedAllocator<int> ta;
    EXPECT_EQ(ta.Resource(), nullptr);
}

TEST(CoreAllocator, TypedAllocatorValueType) {
    using IntAllocator = core::TypedAllocator<int>;
    using DoubleAllocator = core::TypedAllocator<double>;
    
    // Check that value_type is correctly defined
    static_assert(std::is_same<IntAllocator::value_type, int>::value, "value_type should be int");
    static_assert(std::is_same<DoubleAllocator::value_type, double>::value, "value_type should be double");
}

TEST(CoreAllocator, OwnsMethodDefaultReturnsFalse) {
    TestAllocator a;
    
    // Default implementation should return false
    void* some_ptr = reinterpret_cast<void*>(0x1000);
    EXPECT_FALSE(a.Owns(some_ptr));
    EXPECT_FALSE(a.Owns(nullptr));
}

TEST(CoreAllocator, AllocateBytesBasic) {
    TestAllocator a;
    
    void* p = core::AllocateBytes(a, 128, 16);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p, 16u));
    
    // Use the memory
    char* cp = static_cast<char*>(p);
    for (int i = 0; i < 128; ++i) {
        cp[i] = static_cast<char>(i);
    }
    EXPECT_EQ(cp[0], 0);
    EXPECT_EQ(cp[127], 127);
    
    core::DeallocateBytes(a, p, 128, 16);
    EXPECT_EQ(a.alloc_count(), 1u);
    EXPECT_EQ(a.free_count(), 1u);
}

TEST(CoreAllocator, AllocateBytesWithTag) {
    TestAllocator a;
    
    constexpr core::memory_tag test_tag = 999;
    void* p = core::AllocateBytes(a, 64, 8, test_tag);
    ASSERT_NE(p, nullptr);
    
    EXPECT_EQ(a.last_tag(), test_tag);
    
    core::DeallocateBytes(a, p, 64, 8, test_tag);
}

TEST(CoreAllocator, AllocateBytesWithFlags) {
    TestAllocator a;
    
    auto flags = core::AllocationFlags::Persistent | core::AllocationFlags::ZeroInitialize;
    void* p = core::AllocateBytes(a, 32, 4, 0, flags);
    ASSERT_NE(p, nullptr);
    
    core::DeallocateBytes(a, p, 32, 4);
}

TEST(CoreAllocator, DeallocateBytesNullptrIsNoOp) {
    TestAllocator a;
    
    // Should not crash
    core::DeallocateBytes(a, nullptr, 100, 8);
    
    EXPECT_EQ(a.alloc_count(), 0u);
    EXPECT_EQ(a.free_count(), 0u);
}

TEST(CoreAllocator, AllocateArrayOverflowDetection) {
    TestAllocator a;
    
    // Try to allocate array that would overflow
    const core::memory_size huge_count = static_cast<core::memory_size>(~0ull) / sizeof(int);
    int* p = core::AllocateArray<int>(a, huge_count + 1);
    
    // Should return nullptr due to overflow detection
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(a.alloc_count(), 0u);
}

TEST(CoreAllocator, AllocateArrayLargeButValid) {
    TestAllocator a;
    
    // Allocate something large but valid (will fail due to allocator capacity, not overflow)
    const core::memory_size large_count = 500; // 500 * sizeof(int) = 2000 bytes
    int* p = core::AllocateArray<int>(a, large_count);
    
    // This should succeed (allocator has 4096 bytes)
    ASSERT_NE(p, nullptr);
    
    core::DeallocateArray<int>(a, p, large_count);
}

TEST(CoreAllocator, InternalMulOverflowHelper) {
    using core::detail::MulOverflow;
    
    core::memory_size result = 0;
    
    // Valid multiplication
    EXPECT_FALSE(MulOverflow(10u, 20u, result));
    EXPECT_EQ(result, 200u);
    
    // Zero cases
    EXPECT_FALSE(MulOverflow(0u, 100u, result));
    EXPECT_EQ(result, 0u);
    
    EXPECT_FALSE(MulOverflow(100u, 0u, result));
    EXPECT_EQ(result, 0u);
    
    // Overflow case
    const core::memory_size max_val = static_cast<core::memory_size>(~0ull);
    EXPECT_TRUE(MulOverflow(max_val, 2u, result));
    EXPECT_EQ(result, 0u); // Result should be 0 on overflow
}

TEST(CoreAllocator, InternalNormalizeAlignmentHelper) {
    using core::detail::NormalizeAlignment;
    
    // 0 should be normalized to default
    EXPECT_EQ(NormalizeAlignment(0), CORE_DEFAULT_ALIGNMENT);
    
    // Non-zero values should be preserved
    EXPECT_EQ(NormalizeAlignment(8), 8u);
    EXPECT_EQ(NormalizeAlignment(16), 16u);
    EXPECT_EQ(NormalizeAlignment(64), 64u);
}

TEST(CoreAllocator, InternalIsValidAlignmentHelper) {
    using core::detail::IsValidAlignment;
    
    // Valid alignments (powers of two)
    EXPECT_TRUE(IsValidAlignment(1));
    EXPECT_TRUE(IsValidAlignment(2));
    EXPECT_TRUE(IsValidAlignment(4));
    EXPECT_TRUE(IsValidAlignment(8));
    EXPECT_TRUE(IsValidAlignment(16));
    EXPECT_TRUE(IsValidAlignment(64));
    
    // Invalid alignments
    EXPECT_FALSE(IsValidAlignment(0));  // Zero
    EXPECT_FALSE(IsValidAlignment(3));  // Not power of two
    EXPECT_FALSE(IsValidAlignment(5));  // Not power of two
    EXPECT_FALSE(IsValidAlignment(12)); // Not power of two
}

TEST(CoreAllocator, ZeroSizeWithInvalidAlignmentAsserts) {
    TestAllocator a;
    
#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST
    EXPECT_DEATH({
        core::AllocateBytes(a, 0, 3);
    }, ".*");
#endif
}

TEST(CoreAllocator, DeallocateArrayWithOverflowCount) {
    TestAllocator a;
    
    int* arr = core::AllocateArray<int>(a, 10);
    ASSERT_NE(arr, nullptr);
    
    size_t initial_free_count = a.free_count();
    
    const core::memory_size huge = core::UsizeMax() / sizeof(int);
#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST
    EXPECT_DEATH({
        core::DeallocateArray<int>(a, arr, huge + 1);
    }, ".*");
#else
    core::DeallocateArray<int>(a, arr, huge + 1);
    EXPECT_EQ(a.free_count(), initial_free_count);
    
    core::DeallocateArray<int>(a, arr, 10);
    EXPECT_EQ(a.free_count(), initial_free_count + 1);
#endif
}

