#include "core/concurrency/core_atomic.hpp"
#include "core/base/core_features.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>

#if CORE_HAS_64BIT_ATOMICS

#include <thread>

using namespace core;
using core::test::RunConcurrent;

// =============================================================================
// Basic single-threaded tests
// =============================================================================

TEST(AtomicU64Test, DefaultConstruction) {
    atomic_u64 a;
}

TEST(AtomicU64Test, ValueConstruction) {
    atomic_u64 a{0xDEADBEEFCAFEBABE};
    EXPECT_EQ(a.load(), 0xDEADBEEFCAFEBABEull);
}

TEST(AtomicU64Test, LoadStore) {
    atomic_u64 a{0};
    
    a.store(100);
    EXPECT_EQ(a.load(), 100ull);
    
    a.store(0x123456789ABCDEF0ull, memory_order::relaxed);
    EXPECT_EQ(a.load(memory_order::relaxed), 0x123456789ABCDEF0ull);
}

TEST(AtomicU64Test, LoadStoreAllMemoryOrders) {
    atomic_u64 a{0};
    
    a.store(1, memory_order::relaxed);
    EXPECT_EQ(a.load(memory_order::relaxed), 1ull);
    
    a.store(2, memory_order::release);
    EXPECT_EQ(a.load(memory_order::acquire), 2ull);
    
    a.store(3, memory_order::seq_cst);
    EXPECT_EQ(a.load(memory_order::seq_cst), 3ull);
}

TEST(AtomicU64Test, Exchange) {
    atomic_u64 a{1000};
    
    u64 old = a.exchange(2000);
    EXPECT_EQ(old, 1000ull);
    EXPECT_EQ(a.load(), 2000ull);
    
    old = a.exchange(3000, memory_order::relaxed);
    EXPECT_EQ(old, 2000ull);
    EXPECT_EQ(a.load(), 3000ull);
}

TEST(AtomicU64Test, CompareExchangeStrongSuccess) {
    atomic_u64 a{0x100000000ull};
    
    u64 expected = 0x100000000ull;
    bool success = a.compare_exchange_strong(expected, 0x200000000ull);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(expected, 0x100000000ull);
    EXPECT_EQ(a.load(), 0x200000000ull);
}

TEST(AtomicU64Test, CompareExchangeStrongFailure) {
    atomic_u64 a{100};
    
    u64 expected = 50;
    bool success = a.compare_exchange_strong(expected, 200);
    
    EXPECT_FALSE(success);
    EXPECT_EQ(expected, 100ull);
    EXPECT_EQ(a.load(), 100ull);
}

TEST(AtomicU64Test, CompareExchangeStrongUpdatesExpected) {
    atomic_u64 a{0xCAFEBABE};
    
    u64 expected = 0xDEADBEEF;
    (void)a.compare_exchange_strong(expected, 0x12345678);
    
    EXPECT_EQ(expected, 0xCAFEBABEull);
}

TEST(AtomicU64Test, CompareExchangeFailureOrder) {
    atomic_u64 a{42};
    
    u64 expected = 42;
    bool result = a.compare_exchange_strong(expected, 100,
                                             memory_order::acq_rel,
                                             memory_order::acquire);
    EXPECT_TRUE(result);
    EXPECT_EQ(a.load(), 100ull);
    
    expected = 99;
    result = a.compare_exchange_strong(expected, 200,
                                        memory_order::acq_rel,
                                        memory_order::acquire);
    EXPECT_FALSE(result);
    EXPECT_EQ(expected, 100ull);
}

TEST(AtomicU64Test, CompareExchangeWeak) {
    atomic_u64 a{100};
    
    u64 expected = 100;
    while (!a.compare_exchange_weak(expected, 200, memory_order::seq_cst)) {
        expected = 100;
    }
    
    EXPECT_EQ(a.load(), 200ull);
}

TEST(AtomicU64Test, FetchAdd) {
    atomic_u64 a{0x100000000ull};
    
    u64 old = a.fetch_add(0x1ull);
    EXPECT_EQ(old, 0x100000000ull);
    EXPECT_EQ(a.load(), 0x100000001ull);
    
    old = a.fetch_add(0xFFull, memory_order::relaxed);
    EXPECT_EQ(old, 0x100000001ull);
    EXPECT_EQ(a.load(), 0x100000100ull);
}

TEST(AtomicU64Test, FetchSub) {
    atomic_u64 a{0x200000000ull};
    
    u64 old = a.fetch_sub(0x100000000ull);
    EXPECT_EQ(old, 0x200000000ull);
    EXPECT_EQ(a.load(), 0x100000000ull);
    
    old = a.fetch_sub(1, memory_order::relaxed);
    EXPECT_EQ(old, 0x100000000ull);
    EXPECT_EQ(a.load(), 0xFFFFFFFFull);
}

// =============================================================================
// Concurrent correctness tests
// =============================================================================

TEST(AtomicU64Test, ConcurrentIncrement) {
    atomic_u64 counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kIncrementsPerThread = 250000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIncrementsPerThread; ++i) {
            (void)counter.fetch_add(1, memory_order::relaxed);
        }
    });
    
    EXPECT_EQ(counter.load(), kThreadCount * kIncrementsPerThread);
}

TEST(AtomicU64Test, ProducerConsumer) {
    u64 data = 0;
    atomic_u64 ready{0};
    constexpr u64 kMagicValue = 0xDEADBEEFCAFEBABEull;
    
    std::thread producer([&]() {
        data = kMagicValue;
        ready.store(1, memory_order::release);
    });
    
    std::thread consumer([&]() {
        while (ready.load(memory_order::acquire) == 0) {
        }
        EXPECT_EQ(data, kMagicValue);
    });
    
    producer.join();
    consumer.join();
}

TEST(AtomicU64Test, LargeValueIncrement) {
    atomic_u64 counter{0x7FFFFFFF00000000ull};
    constexpr int kThreadCount = 4;
    constexpr int kIncrementsPerThread = 10000;
    
    u64 initial = counter.load();
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIncrementsPerThread; ++i) {
            (void)counter.fetch_add(1, memory_order::relaxed);
        }
    });
    
    EXPECT_EQ(counter.load(), initial + kThreadCount * kIncrementsPerThread);
}

TEST(AtomicU64Test, CompareExchangeRace) {
    atomic_u64 counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kIncrementsPerThread = 10000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIncrementsPerThread; ++i) {
            u64 expected = counter.load(memory_order::relaxed);
            while (!counter.compare_exchange_weak(expected, expected + 1, 
                                                   memory_order::relaxed)) {
            }
        }
    });
    
    EXPECT_EQ(counter.load(), kThreadCount * kIncrementsPerThread);
}

// =============================================================================
// Stress tests
// =============================================================================

TEST(AtomicU64Test, HighContentionCounter) {
    atomic_u64 counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kOperationsPerThread = 50000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kOperationsPerThread; ++i) {
            (void)counter.fetch_add(1, memory_order::seq_cst);
        }
    });
    
    EXPECT_EQ(counter.load(), kThreadCount * kOperationsPerThread);
}

TEST(AtomicU64Test, MixedOperations) {
    atomic_u64 value{0x100000000ull};
    constexpr int kThreadCount = 4;
    constexpr int kOperationsPerThread = 10000;
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        for (int i = 0; i < kOperationsPerThread; ++i) {
            if (thread_id % 2 == 0) {
                (void)value.fetch_add(1, memory_order::relaxed);
            } else {
                (void)value.fetch_sub(1, memory_order::relaxed);
            }
        }
    });
    
    u64 final = value.load();
    EXPECT_EQ(final, 0x100000000ull);
}

#else

TEST(AtomicU64Test, NotAvailableOn32Bit) {
    GTEST_SKIP() << "64-bit atomics not available on this platform";
}

#endif
