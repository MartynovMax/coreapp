#include "core/concurrency/core_atomic.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <thread>

using namespace core;
using core::test::RunConcurrent;

// =============================================================================
// Basic tests
// =============================================================================

TEST(AtomicUintptrTest, SizeMatchesPointerSize) {
    EXPECT_EQ(sizeof(atomic_uintptr), sizeof(void*));
    EXPECT_EQ(sizeof(atomic_uintptr), sizeof(usize));
}

TEST(AtomicUintptrTest, DefaultConstruction) {
    atomic_uintptr a;
}

TEST(AtomicUintptrTest, ValueConstruction) {
    atomic_uintptr a{12345};
    EXPECT_EQ(a.load(), static_cast<usize>(12345));
}

TEST(AtomicUintptrTest, LoadStore) {
    atomic_uintptr a{0};
    
    a.store(100);
    EXPECT_EQ(a.load(), static_cast<usize>(100));
    
    a.store(200, memory_order::relaxed);
    EXPECT_EQ(a.load(memory_order::relaxed), static_cast<usize>(200));
}

TEST(AtomicUintptrTest, Exchange) {
    atomic_uintptr a{10};
    
    usize old = a.exchange(20);
    EXPECT_EQ(old, static_cast<usize>(10));
    EXPECT_EQ(a.load(), static_cast<usize>(20));
}

TEST(AtomicUintptrTest, CompareExchange) {
    atomic_uintptr a{100};
    
    usize expected = 100;
    bool success = a.compare_exchange_strong(expected, 200);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(a.load(), static_cast<usize>(200));
}

TEST(AtomicUintptrTest, CompareExchangeStrongUpdatesExpected) {
    atomic_uintptr a{100};
    
    usize expected = 50;
    a.compare_exchange_strong(expected, 200);
    
    EXPECT_EQ(expected, static_cast<usize>(100));
}

TEST(AtomicUintptrTest, FetchAdd) {
    atomic_uintptr a{10};
    
    usize old = a.fetch_add(5);
    EXPECT_EQ(old, static_cast<usize>(10));
    EXPECT_EQ(a.load(), static_cast<usize>(15));
}

TEST(AtomicUintptrTest, FetchSub) {
    atomic_uintptr a{100};
    
    usize old = a.fetch_sub(30);
    EXPECT_EQ(old, static_cast<usize>(100));
    EXPECT_EQ(a.load(), static_cast<usize>(70));
}

// =============================================================================
// Pointer round-trip tests
// =============================================================================

TEST(AtomicUintptrTest, PointerRoundTrip) {
    int value = 42;
    int* ptr = &value;
    
    usize bits = reinterpret_cast<usize>(ptr);
    int* restored = reinterpret_cast<int*>(bits);
    
    EXPECT_EQ(restored, ptr);
    EXPECT_EQ(*restored, 42);
}

TEST(AtomicUintptrTest, PointerRoundTripThroughAtomic) {
    int value = 123;
    int* ptr = &value;
    
    atomic_uintptr a{reinterpret_cast<usize>(ptr)};
    
    usize stored = a.load();
    int* restored = reinterpret_cast<int*>(stored);
    
    EXPECT_EQ(restored, ptr);
    EXPECT_EQ(*restored, 123);
}

TEST(AtomicUintptrTest, NullPointerRoundTrip) {
    void* null_ptr = nullptr;
    
    atomic_uintptr a{reinterpret_cast<usize>(null_ptr)};
    
    usize stored = a.load();
    void* restored = reinterpret_cast<void*>(stored);
    
    EXPECT_EQ(restored, nullptr);
}

// =============================================================================
// Array index tests
// =============================================================================

TEST(AtomicUintptrTest, AsArrayIndex) {
    constexpr size_t kArraySize = 1000;
    int array[kArraySize] = {};
    atomic_uintptr index{0};
    
    constexpr int kThreadCount = 4;
    RunConcurrent(kThreadCount, [&](int thread_id) {
        for (int i = 0; i < 250; ++i) {
            usize idx = index.fetch_add(1, memory_order::relaxed);
            if (idx < kArraySize) {
                array[idx] = thread_id + 1;
            }
        }
    });
    
    for (size_t i = 0; i < kArraySize; ++i) {
        EXPECT_NE(array[i], 0) << "Index " << i << " not filled";
    }
}

// =============================================================================
// Concurrent tests
// =============================================================================

TEST(AtomicUintptrTest, ConcurrentIncrement) {
    atomic_uintptr counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kIncrementsPerThread = 100000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIncrementsPerThread; ++i) {
            (void)counter.fetch_add(1, memory_order::relaxed);
        }
    });
    
    EXPECT_EQ(counter.load(), static_cast<usize>(kThreadCount * kIncrementsPerThread));
}

TEST(AtomicUintptrTest, CompareExchangeRace) {
    atomic_uintptr counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kIncrementsPerThread = 10000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIncrementsPerThread; ++i) {
            usize expected = counter.load(memory_order::relaxed);
            while (!counter.compare_exchange_weak(expected, expected + 1, 
                                                   memory_order::relaxed)) {
            }
        }
    });
    
    EXPECT_EQ(counter.load(), static_cast<usize>(kThreadCount * kIncrementsPerThread));
}

// =============================================================================
// Stress tests
// =============================================================================

TEST(AtomicUintptrTest, HighContentionCounter) {
    atomic_uintptr counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kOperationsPerThread = 50000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kOperationsPerThread; ++i) {
            (void)counter.fetch_add(1, memory_order::seq_cst);
        }
    });
    
    EXPECT_EQ(counter.load(), static_cast<usize>(kThreadCount * kOperationsPerThread));
}
