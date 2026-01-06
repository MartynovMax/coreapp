#include "core/memory/allocation_tracker.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>

namespace {

// Test fixture for allocation tracker tests
class AllocationTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
#if CORE_MEMORY_TRACKING
        core::InitializeAllocationTracker();
        core::ResetAllocationTrackerStats();
        core::ResetTagStats();
        core::ClearAllocationListeners();
#endif
    }

    void TearDown() override {
#if CORE_MEMORY_TRACKING
        core::ClearAllocationListeners();
        core::ShutdownAllocationTracker();
#endif
    }
};

// Helper for listener tests
struct ListenerData {
    int callCount = 0;
    core::memory_size lastSize = 0;
};

void TestListener(
    core::AllocationEvent event,
    const core::IAllocator* allocator,
    const core::AllocationRequest* request,
    const core::AllocationInfo* info,
    void* user) noexcept
{
    CORE_UNUSED(allocator);
    CORE_UNUSED(request);
    CORE_UNUSED(info);
    
    auto* data = static_cast<ListenerData*>(user);
    data->callCount++;
    if (event == core::AllocationEvent::AllocateEnd && info) {
        data->lastSize = info->size;
    }
}

// ----------------------------------------------------------------------------
// Global Statistics Tests
// ----------------------------------------------------------------------------

TEST_F(AllocationTrackerTest, InitializeAndShutdown) {
#if CORE_MEMORY_TRACKING
    EXPECT_TRUE(core::IsAllocationTrackingEnabled());
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 0u);
    EXPECT_EQ(stats.peak_allocated, 0u);
    EXPECT_EQ(stats.total_allocations, 0u);
    EXPECT_EQ(stats.total_deallocations, 0u);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, GlobalStatsBasicAllocation) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    
    void* ptr = core::AllocateBytes(allocator, 1024);
    ASSERT_NE(ptr, nullptr);
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 1024u);
    EXPECT_EQ(stats.peak_allocated, 1024u);
    EXPECT_EQ(stats.total_allocations, 1u);
    EXPECT_EQ(stats.total_deallocations, 0u);
    
    core::DeallocateBytes(allocator, ptr, 1024);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, GlobalStatsDeallocation) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    
    void* ptr = core::AllocateBytes(allocator, 512);
    ASSERT_NE(ptr, nullptr);
    
    core::DeallocateBytes(allocator, ptr, 512);
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 0u);
    EXPECT_EQ(stats.total_allocations, 1u);
    EXPECT_EQ(stats.total_deallocations, 1u);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, GlobalStatsPeakTracking) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    
    void* ptr1 = core::AllocateBytes(allocator, 1024);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = core::AllocateBytes(allocator, 2048);
    ASSERT_NE(ptr2, nullptr);
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 3072u);
    EXPECT_EQ(stats.peak_allocated, 3072u);
    
    core::DeallocateBytes(allocator, ptr1, 1024);
    
    stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 2048u);
    EXPECT_EQ(stats.peak_allocated, 3072u); // Peak should remain
    
    core::DeallocateBytes(allocator, ptr2, 2048);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, GlobalStatsMultipleAllocations) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    
    constexpr int count = 5;
    void* ptrs[count];
    
    for (int i = 0; i < count; ++i) {
        ptrs[i] = core::AllocateBytes(allocator, 256);
        ASSERT_NE(ptrs[i], nullptr);
    }
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 1280u);
    EXPECT_EQ(stats.total_allocations, 5u);
    
    for (int i = 0; i < count; ++i) {
        core::DeallocateBytes(allocator, ptrs[i], 256);
    }
    
    stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 0u);
    EXPECT_EQ(stats.total_deallocations, 5u);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, ResetGlobalStats) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    
    void* ptr = core::AllocateBytes(allocator, 128);
    ASSERT_NE(ptr, nullptr);
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_GT(stats.total_allocations, 0u);
    
    core::ResetAllocationTrackerStats();
    
    stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.current_allocated, 0u);
    EXPECT_EQ(stats.peak_allocated, 0u);
    EXPECT_EQ(stats.total_allocations, 0u);
    EXPECT_EQ(stats.total_deallocations, 0u);
    
    core::DeallocateBytes(allocator, ptr, 128);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

// ----------------------------------------------------------------------------
// Per-Tag Statistics Tests
// ----------------------------------------------------------------------------

TEST_F(AllocationTrackerTest, TagStatsBasicTracking) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    constexpr core::memory_tag TAG = 42;
    
    void* ptr = core::AllocateBytes(allocator, 512, CORE_DEFAULT_ALIGNMENT, TAG);
    ASSERT_NE(ptr, nullptr);
    
    core::TagStats tagStats;
    bool found = core::GetTagStats(TAG, tagStats);
    EXPECT_TRUE(found);
    EXPECT_EQ(tagStats.tag, TAG);
    EXPECT_EQ(tagStats.current_allocated, 512u);
    EXPECT_EQ(tagStats.peak_allocated, 512u);
    EXPECT_EQ(tagStats.alloc_count, 1u);
    EXPECT_EQ(tagStats.dealloc_count, 0u);
    
    core::DeallocateBytes(allocator, ptr, 512, CORE_DEFAULT_ALIGNMENT, TAG);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, TagStatsMultipleTags) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    constexpr core::memory_tag TAG1 = 10;
    constexpr core::memory_tag TAG2 = 20;
    
    void* ptr1 = core::AllocateBytes(allocator, 256, CORE_DEFAULT_ALIGNMENT, TAG1);
    void* ptr2 = core::AllocateBytes(allocator, 512, CORE_DEFAULT_ALIGNMENT, TAG2);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    core::TagStats stats1, stats2;
    EXPECT_TRUE(core::GetTagStats(TAG1, stats1));
    EXPECT_TRUE(core::GetTagStats(TAG2, stats2));
    
    EXPECT_EQ(stats1.current_allocated, 256u);
    EXPECT_EQ(stats2.current_allocated, 512u);
    
    core::DeallocateBytes(allocator, ptr1, 256, CORE_DEFAULT_ALIGNMENT, TAG1);
    core::DeallocateBytes(allocator, ptr2, 512, CORE_DEFAULT_ALIGNMENT, TAG2);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, TagStatsPeakTracking) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    constexpr core::memory_tag TAG = 99;
    
    void* ptr1 = core::AllocateBytes(allocator, 1024, CORE_DEFAULT_ALIGNMENT, TAG);
    void* ptr2 = core::AllocateBytes(allocator, 2048, CORE_DEFAULT_ALIGNMENT, TAG);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    core::TagStats stats;
    EXPECT_TRUE(core::GetTagStats(TAG, stats));
    EXPECT_EQ(stats.current_allocated, 3072u);
    EXPECT_EQ(stats.peak_allocated, 3072u);
    
    core::DeallocateBytes(allocator, ptr1, 1024, CORE_DEFAULT_ALIGNMENT, TAG);
    
    EXPECT_TRUE(core::GetTagStats(TAG, stats));
    EXPECT_EQ(stats.current_allocated, 2048u);
    EXPECT_EQ(stats.peak_allocated, 3072u); // Peak should remain
    
    core::DeallocateBytes(allocator, ptr2, 2048, CORE_DEFAULT_ALIGNMENT, TAG);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, EnumerateTagStats) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    constexpr core::memory_tag TAG1 = 100;
    constexpr core::memory_tag TAG2 = 200;
    constexpr core::memory_tag TAG3 = 300;
    
    void* ptr1 = core::AllocateBytes(allocator, 128, CORE_DEFAULT_ALIGNMENT, TAG1);
    void* ptr2 = core::AllocateBytes(allocator, 256, CORE_DEFAULT_ALIGNMENT, TAG2);
    void* ptr3 = core::AllocateBytes(allocator, 512, CORE_DEFAULT_ALIGNMENT, TAG3);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    
    int callbackCount = 0;
    core::EnumerateTagStats([](const core::TagStats& stats, void* user) {
        int* count = static_cast<int*>(user);
        (*count)++;
        EXPECT_GT(stats.tag, 0u);
        EXPECT_GT(stats.current_allocated, 0u);
    }, &callbackCount);
    
    EXPECT_EQ(callbackCount, 3);
    
    core::DeallocateBytes(allocator, ptr1, 128, CORE_DEFAULT_ALIGNMENT, TAG1);
    core::DeallocateBytes(allocator, ptr2, 256, CORE_DEFAULT_ALIGNMENT, TAG2);
    core::DeallocateBytes(allocator, ptr3, 512, CORE_DEFAULT_ALIGNMENT, TAG3);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, ResetTagStats) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    constexpr core::memory_tag TAG = 555;
    
    void* ptr = core::AllocateBytes(allocator, 1024, CORE_DEFAULT_ALIGNMENT, TAG);
    ASSERT_NE(ptr, nullptr);
    
    core::TagStats stats;
    EXPECT_TRUE(core::GetTagStats(TAG, stats));
    
    core::ResetTagStats();
    
    EXPECT_FALSE(core::GetTagStats(TAG, stats));
    
    core::DeallocateBytes(allocator, ptr, 1024, CORE_DEFAULT_ALIGNMENT, TAG);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, TagStatsNotFound) {
#if CORE_MEMORY_TRACKING
    constexpr core::memory_tag UNUSED_TAG = 9999;
    
    core::TagStats stats;
    bool found = core::GetTagStats(UNUSED_TAG, stats);
    EXPECT_FALSE(found);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

// ----------------------------------------------------------------------------
// Enable/Disable Behavior Tests
// ----------------------------------------------------------------------------

TEST_F(AllocationTrackerTest, DisableTracking) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    
    void* ptr1 = core::AllocateBytes(allocator, 256);
    ASSERT_NE(ptr1, nullptr);
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.total_allocations, 1u);
    
    core::DisableAllocationTracking();
    EXPECT_FALSE(core::IsAllocationTrackingEnabled());
    
    void* ptr2 = core::AllocateBytes(allocator, 512);
    ASSERT_NE(ptr2, nullptr);
    
    stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.total_allocations, 1u); // Should not increase
    EXPECT_EQ(stats.current_allocated, 256u); // Should not increase
    
    core::DeallocateBytes(allocator, ptr2, 512);
    core::DeallocateBytes(allocator, ptr1, 256);
    
    core::EnableAllocationTracking();
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, ReEnableTracking) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    
    core::DisableAllocationTracking();
    void* ptr1 = core::AllocateBytes(allocator, 128);
    ASSERT_NE(ptr1, nullptr);
    
    auto stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.total_allocations, 0u);
    
    core::EnableAllocationTracking();
    EXPECT_TRUE(core::IsAllocationTrackingEnabled());
    
    void* ptr2 = core::AllocateBytes(allocator, 256);
    ASSERT_NE(ptr2, nullptr);
    
    stats = core::GetAllocationTrackerStats();
    EXPECT_EQ(stats.total_allocations, 1u); // Should track again
    
    core::DeallocateBytes(allocator, ptr2, 256);
    core::DeallocateBytes(allocator, ptr1, 128);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, IsTrackingEnabled) {
#if CORE_MEMORY_TRACKING
    EXPECT_TRUE(core::IsAllocationTrackingEnabled());
    
    core::DisableAllocationTracking();
    EXPECT_FALSE(core::IsAllocationTrackingEnabled());
    
    core::EnableAllocationTracking();
    EXPECT_TRUE(core::IsAllocationTrackingEnabled());
#else
    EXPECT_FALSE(core::IsAllocationTrackingEnabled());
#endif
}

// ----------------------------------------------------------------------------
// Listener Management Tests
// ----------------------------------------------------------------------------

TEST_F(AllocationTrackerTest, RegisterListener) {
#if CORE_MEMORY_TRACKING
    ListenerData data;
    
    auto handle = core::RegisterAllocationListener(TestListener, &data);
    EXPECT_NE(handle, core::kInvalidListenerHandle);
    
    core::UnregisterAllocationListener(handle);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, UnregisterListener) {
#if CORE_MEMORY_TRACKING
    ListenerData data;
    
    auto handle = core::RegisterAllocationListener(TestListener, &data);
    ASSERT_NE(handle, core::kInvalidListenerHandle);
    
    bool removed = core::UnregisterAllocationListener(handle);
    EXPECT_TRUE(removed);
    
    // Try to remove again - should fail
    removed = core::UnregisterAllocationListener(handle);
    EXPECT_FALSE(removed);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, ListenerNotCalledWhenDisabled) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    ListenerData data;
    
    auto handle = core::RegisterAllocationListener(TestListener, &data);
    ASSERT_NE(handle, core::kInvalidListenerHandle);
    
    void* ptr1 = core::AllocateBytes(allocator, 128);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_GT(data.callCount, 0);
    
    int countBefore = data.callCount;
    core::DisableAllocationTracking();
    
    void* ptr2 = core::AllocateBytes(allocator, 256);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(data.callCount, countBefore); // Should not increase
    
    core::EnableAllocationTracking();
    core::DeallocateBytes(allocator, ptr2, 256);
    core::DeallocateBytes(allocator, ptr1, 128);
    
    core::UnregisterAllocationListener(handle);
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, ClearListeners) {
#if CORE_MEMORY_TRACKING
    ListenerData data1, data2;
    
    auto handle1 = core::RegisterAllocationListener(TestListener, &data1);
    auto handle2 = core::RegisterAllocationListener(TestListener, &data2);
    ASSERT_NE(handle1, core::kInvalidListenerHandle);
    ASSERT_NE(handle2, core::kInvalidListenerHandle);
    
    core::ClearAllocationListeners();
    
    // Both should be removed
    EXPECT_FALSE(core::UnregisterAllocationListener(handle1));
    EXPECT_FALSE(core::UnregisterAllocationListener(handle2));
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

// ----------------------------------------------------------------------------
// Edge Cases
// ----------------------------------------------------------------------------

TEST_F(AllocationTrackerTest, MaxListeners) {
#if CORE_MEMORY_TRACKING
    ListenerData datas[CORE_ALLOCATION_TRACKER_MAX_LISTENERS + 1];
    core::AllocationListenerHandle handles[CORE_ALLOCATION_TRACKER_MAX_LISTENERS + 1];
    
    // Fill up the listener table
    for (int i = 0; i < CORE_ALLOCATION_TRACKER_MAX_LISTENERS; ++i) {
        handles[i] = core::RegisterAllocationListener(TestListener, &datas[i]);
        EXPECT_NE(handles[i], core::kInvalidListenerHandle);
    }
    
    // Try to register one more - should fail
    handles[CORE_ALLOCATION_TRACKER_MAX_LISTENERS] = 
        core::RegisterAllocationListener(TestListener, &datas[CORE_ALLOCATION_TRACKER_MAX_LISTENERS]);
    EXPECT_EQ(handles[CORE_ALLOCATION_TRACKER_MAX_LISTENERS], core::kInvalidListenerHandle);
    
    // Clean up
    for (int i = 0; i < CORE_ALLOCATION_TRACKER_MAX_LISTENERS; ++i) {
        core::UnregisterAllocationListener(handles[i]);
    }
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

TEST_F(AllocationTrackerTest, MaxTags) {
#if CORE_MEMORY_TRACKING
    core::MallocAllocator allocator;
    void* ptrs[CORE_ALLOCATION_TRACKER_MAX_TAGS + 1];
    
    // Fill up the tag table
    for (int i = 0; i < CORE_ALLOCATION_TRACKER_MAX_TAGS; ++i) {
        core::memory_tag tag = static_cast<core::memory_tag>(i + 1);
        ptrs[i] = core::AllocateBytes(allocator, 64, CORE_DEFAULT_ALIGNMENT, tag);
        ASSERT_NE(ptrs[i], nullptr);
    }
    
    // Try to allocate with one more tag - should still work (allocation succeeds)
    // but stats might not be tracked
    core::memory_tag extraTag = CORE_ALLOCATION_TRACKER_MAX_TAGS + 1;
    ptrs[CORE_ALLOCATION_TRACKER_MAX_TAGS] = 
        core::AllocateBytes(allocator, 64, CORE_DEFAULT_ALIGNMENT, extraTag);
    ASSERT_NE(ptrs[CORE_ALLOCATION_TRACKER_MAX_TAGS], nullptr);
    
    // Clean up
    for (int i = 0; i <= CORE_ALLOCATION_TRACKER_MAX_TAGS; ++i) {
        core::memory_tag tag = static_cast<core::memory_tag>(i + 1);
        core::DeallocateBytes(allocator, ptrs[i], 64, CORE_DEFAULT_ALIGNMENT, tag);
    }
#else
    GTEST_SKIP() << "CORE_MEMORY_TRACKING disabled";
#endif
}

} // namespace

