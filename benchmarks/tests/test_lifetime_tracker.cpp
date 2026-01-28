#include "benchmarks/workload/lifetime_tracker.hpp"
#include "core/memory/malloc_allocator.hpp"
#include "core/memory/bump_arena.hpp"
#include "benchmarks/common/seeded_rng.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <set>
#include <algorithm>

using namespace core;
using namespace core::bench;

namespace {

constexpr core::memory_alignment kAlign = 8;
constexpr core::memory_tag kTag = 0;

void TrackN(LifetimeTracker& tracker, u32 N, u64 startOp = 0, u32 size = 16) {
    for (u32 i = 0; i < N; ++i)
        tracker.Track(reinterpret_cast<void*>(static_cast<core::usize>(0x1000 + i)), size, kAlign, kTag, startOp + i);
}

} // namespace

TEST(LifetimeTracker, FifoModel_FreesOldest) {
    MallocAllocator alloc;
    SeededRNG rng(1);
    LifetimeTracker tracker(10, LifetimeModel::Fifo, rng, &alloc);
    TrackN(tracker, 3);
    AllocInfo info{};
    ASSERT_TRUE(tracker.PopForFree(info));
    void* f1 = info.ptr;
    ASSERT_TRUE(tracker.PopForFree(info));
    void* f2 = info.ptr;
    ASSERT_TRUE(tracker.PopForFree(info));
    void* f3 = info.ptr;
    ASSERT_EQ(f1, reinterpret_cast<void*>(static_cast<core::usize>(0x1000)));
    ASSERT_EQ(f2, reinterpret_cast<void*>(static_cast<core::usize>(0x1001)));
    ASSERT_EQ(f3, reinterpret_cast<void*>(static_cast<core::usize>(0x1002)));
    ASSERT_EQ(tracker.GetLiveCount(), 0);
}

TEST(LifetimeTracker, LifoModel_FreesNewest) {
    MallocAllocator alloc;
    SeededRNG rng(2);
    LifetimeTracker tracker(10, LifetimeModel::Lifo, rng, &alloc);
    TrackN(tracker, 3);
    AllocInfo info{};
    ASSERT_TRUE(tracker.PopForFree(info));
    void* f1 = info.ptr;
    ASSERT_TRUE(tracker.PopForFree(info));
    void* f2 = info.ptr;
    ASSERT_TRUE(tracker.PopForFree(info));
    void* f3 = info.ptr;
    ASSERT_EQ(f1, reinterpret_cast<void*>(static_cast<core::usize>(0x1002)));
    ASSERT_EQ(f2, reinterpret_cast<void*>(static_cast<core::usize>(0x1001)));
    ASSERT_EQ(f3, reinterpret_cast<void*>(static_cast<core::usize>(0x1000)));
    ASSERT_EQ(tracker.GetLiveCount(), 0);
}

TEST(LifetimeTracker, RandomModel_AllObjectsFreed) {
    MallocAllocator alloc;
    SeededRNG rng(42);
    LifetimeTracker tracker(10, LifetimeModel::Random, rng, &alloc);
    TrackN(tracker, 10);
    std::set<void*> freed;
    AllocInfo info{};
    for (u32 i = 0; i < 10; ++i) {
        ASSERT_TRUE(tracker.PopForFree(info));
        freed.insert(info.ptr);
    }
    ASSERT_EQ(freed.size(), 10);
    ASSERT_EQ(tracker.GetLiveCount(), 0);
}

TEST(LifetimeTracker, RandomModel_Determinism) {
    MallocAllocator alloc;
    SeededRNG rng1(123), rng2(123);
    LifetimeTracker t1(10, LifetimeModel::Random, rng1, &alloc);
    LifetimeTracker t2(10, LifetimeModel::Random, rng2, &alloc);
    TrackN(t1, 10);
    TrackN(t2, 10);
    std::vector<core::usize> order1, order2;
    AllocInfo info1{}, info2{};
    for (u32 i = 0; i < 10; ++i) {
        ASSERT_TRUE(t1.PopForFree(info1));
        order1.push_back(reinterpret_cast<core::usize>(info1.ptr));
        ASSERT_TRUE(t2.PopForFree(info2));
        order2.push_back(reinterpret_cast<core::usize>(info2.ptr));
    }
    ASSERT_EQ(order1, order2);
}

TEST(LifetimeTracker, BoundedModel_FreeStartsAtMaxLive) {
    MallocAllocator alloc;
    SeededRNG rng(3);
    LifetimeTracker tracker(3, LifetimeModel::Bounded, rng, &alloc);
    tracker.Track(reinterpret_cast<void *>(1), 16, kAlign, kTag, 0);
    tracker.Track(reinterpret_cast<void *>(2), 16, kAlign, kTag, 1);
    tracker.Track(reinterpret_cast<void *>(3), 16, kAlign, kTag, 2);
    tracker.Track(reinterpret_cast<void *>(4), 16, kAlign, kTag, 3);
    ASSERT_EQ(tracker.GetLiveCount(), 3);
}

TEST(LifetimeTracker, LongLivedModel_SelectAlwaysNull) {
    MallocAllocator alloc;
    SeededRNG rng(4);
    LifetimeTracker tracker(10, LifetimeModel::LongLived, rng, &alloc);
    TrackN(tracker, 5);
    AllocInfo info{};
    ASSERT_FALSE(tracker.PopForFree(info));
}

TEST(LifetimeTracker, Track_UpdatesLiveCountAndBytes) {
    MallocAllocator alloc;
    SeededRNG rng(5);
    LifetimeTracker tracker(10, LifetimeModel::Fifo, rng, &alloc);
    tracker.Track(reinterpret_cast<void *>(1), 16, kAlign, kTag, 0);
    tracker.Track(reinterpret_cast<void *>(2), 32, kAlign, kTag, 1);
    ASSERT_EQ(tracker.GetLiveCount(), 2);
    ASSERT_EQ(tracker.GetLiveBytes(), 48);
    AllocInfo info{};
    ASSERT_TRUE(tracker.PopForFree(info));
    ASSERT_EQ(tracker.GetLiveCount(), 1);
    ASSERT_EQ(tracker.GetLiveBytes(), 32);
}

TEST(LifetimeTracker, PeakTracking) {
    MallocAllocator alloc;
    SeededRNG rng(6);
    LifetimeTracker tracker(10, LifetimeModel::Fifo, rng, &alloc);
    tracker.Track(reinterpret_cast<void *>(1), 16, kAlign, kTag, 0);
    tracker.Track(reinterpret_cast<void *>(2), 32, kAlign, kTag, 1);
    tracker.Track(reinterpret_cast<void *>(3), 64, kAlign, kTag, 2);
    AllocInfo info{};
    tracker.PopForFree(info);
    tracker.PopForFree(info);
    tracker.Track(reinterpret_cast<void *>(4), 128, kAlign, kTag, 3);
    ASSERT_EQ(tracker.GetPeakBytes(), 192); // 64+128
    ASSERT_EQ(tracker.GetLiveBytes(), 192); // 64+128
}

TEST(LifetimeTracker, GetAllLive_ReturnsCorrectSet) {
    MallocAllocator alloc;
    SeededRNG rng(7);
    LifetimeTracker tracker(10, LifetimeModel::Fifo, rng, &alloc);
    tracker.Track(reinterpret_cast<void *>(1), 16, kAlign, kTag, 0);
    tracker.Track(reinterpret_cast<void *>(2), 32, kAlign, kTag, 1);
    tracker.Track(reinterpret_cast<void *>(3), 64, kAlign, kTag, 2);
    ASSERT_EQ(tracker.GetLiveCount(), 3);
    AllocInfo info{};
    ASSERT_TRUE(tracker.PopForFree(info)); // remove (void*)1
    ASSERT_EQ(info.ptr, reinterpret_cast<void *>(1));
    ASSERT_EQ(tracker.GetLiveCount(), 2);
    AllocInfo* arr = nullptr;
    u32 count = 0;
    u32 head = 0;
    bool ringMode = false;
    tracker.GetAllLive(&arr, &count, &head, &ringMode);
    ASSERT_EQ(count, 2);
    ASSERT_TRUE(ringMode);
    std::set<void*> livePtrs;
    for (u32 i = 0; i < count; ++i) {
        u32 idx = (head + i) % tracker.GetCapacity();
        livePtrs.insert(arr[idx].ptr);
    }
    ASSERT_EQ(livePtrs.size(), 2);
    ASSERT_TRUE(livePtrs.count(reinterpret_cast<void*>(2)));
    ASSERT_TRUE(livePtrs.count(reinterpret_cast<void*>(3)));
}

TEST(LifetimeTracker, Clear_ResetsState) {
    MallocAllocator alloc;
    SeededRNG rng(8);
    LifetimeTracker tracker(10, LifetimeModel::Fifo, rng, &alloc);
    TrackN(tracker, 5);
    tracker.Clear();
    ASSERT_EQ(tracker.GetLiveCount(), 0);
    ASSERT_EQ(tracker.GetLiveBytes(), 0);
    ASSERT_EQ(tracker.GetPeakBytes(), 0);
}

TEST(LifetimeTracker, BufferOverflow_Assertion) {
    MallocAllocator alloc;
    SeededRNG rng(9);
    LifetimeTracker tracker(2, LifetimeModel::Fifo, rng, &alloc);
    tracker.Track(reinterpret_cast<void *>(1), 16, kAlign, kTag, 0);
    tracker.Track(reinterpret_cast<void *>(2), 16, kAlign, kTag, 1);
    const auto res = tracker.Track(reinterpret_cast<void *>(3), 16, kAlign, kTag, 2);
    ASSERT_FALSE(res.tracked);
}

TEST(LifetimeTracker, MallocAllocator_BufferManagement) {
    MallocAllocator alloc;
    SeededRNG rng(10);
    LifetimeTracker tracker(10, LifetimeModel::Fifo, rng, &alloc);
    TrackN(tracker, 10);
    AllocInfo info{};
    for (u32 i = 0; i < 10; ++i)
        tracker.PopForFree(info);
    ASSERT_EQ(tracker.GetLiveCount(), 0);
}

TEST(LifetimeTracker, BumpArena_BufferManagement) {
    static u8 arenaBuf[4096];
    BumpArena arena(arenaBuf, sizeof(arenaBuf));
    SeededRNG rng(11);
    LifetimeTracker tracker(10, LifetimeModel::Fifo, rng, reinterpret_cast<IAllocator*>(&arena));
    TrackN(tracker, 10);
    AllocInfo info{};
    for (u32 i = 0; i < 10; ++i)
        tracker.PopForFree(info);
    ASSERT_EQ(tracker.GetLiveCount(), 0);
}

TEST(LifetimeTracker, StressTest_Track100K_FreeRandom_NoLeaks) {
    MallocAllocator alloc;
    SeededRNG rng(777);
    const u32 N = 100000;
    LifetimeTracker tracker(N, LifetimeModel::Random, rng, &alloc);
    std::vector<void*> ptrs(N);
    for (u32 i = 0; i < N; ++i) {
        ptrs[i] = reinterpret_cast<void*>(static_cast<core::usize>(0x100000 + i));
        tracker.Track(ptrs[i], 8, kAlign, kTag, i);
    }
    std::set<void*> freed;
    AllocInfo info{};
    for (u32 i = 0; i < N; ++i) {
        ASSERT_TRUE(tracker.PopForFree(info));
        freed.insert(info.ptr);
    }
    ASSERT_EQ(freed.size(), N);
    ASSERT_EQ(tracker.GetLiveCount(), 0);
}
