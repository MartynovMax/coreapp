#include "core/concurrency/core_atomic.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <thread>

using namespace core;
using core::test::RunConcurrent;

// =============================================================================
// Basic single-threaded tests
// =============================================================================

TEST(AtomicU32Test, DefaultConstruction) {
    atomic_u32 a;
}

TEST(AtomicU32Test, ValueConstruction) {
    atomic_u32 a{42};
    EXPECT_EQ(a.load(), 42u);
}

TEST(AtomicU32Test, LoadStore) {
    atomic_u32 a{0};
    
    a.store(100);
    EXPECT_EQ(a.load(), 100u);
    
    a.store(200, memory_order::relaxed);
    EXPECT_EQ(a.load(memory_order::relaxed), 200u);
}

TEST(AtomicU32Test, LoadStoreAllMemoryOrders) {
    atomic_u32 a{0};
    
    a.store(1, memory_order::relaxed);
    EXPECT_EQ(a.load(memory_order::relaxed), 1u);
    
    a.store(2, memory_order::release);
    EXPECT_EQ(a.load(memory_order::acquire), 2u);
    
    a.store(3, memory_order::seq_cst);
    EXPECT_EQ(a.load(memory_order::seq_cst), 3u);
}

TEST(AtomicU32Test, Exchange) {
    atomic_u32 a{10};
    
    u32 old = a.exchange(20);
    EXPECT_EQ(old, 10u);
    EXPECT_EQ(a.load(), 20u);
    
    old = a.exchange(30, memory_order::relaxed);
    EXPECT_EQ(old, 20u);
    EXPECT_EQ(a.load(), 30u);
}

TEST(AtomicU32Test, CompareExchangeStrongSuccess) {
    atomic_u32 a{100};
    
    u32 expected = 100;
    bool success = a.compare_exchange_strong(expected, 200);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(expected, 100u);
    EXPECT_EQ(a.load(), 200u);
}

TEST(AtomicU32Test, CompareExchangeStrongFailure) {
    atomic_u32 a{100};
    
    u32 expected = 50;
    bool success = a.compare_exchange_strong(expected, 200);
    
    EXPECT_FALSE(success);
    EXPECT_EQ(expected, 100u);
    EXPECT_EQ(a.load(), 100u);
}

TEST(AtomicU32Test, CompareExchangeStrongUpdatesExpected) {
    atomic_u32 a{100};
    
    u32 expected = 50;
    a.compare_exchange_strong(expected, 200);
    
    EXPECT_EQ(expected, 100u);
}

TEST(AtomicU32Test, CompareExchangeFailureOrder) {
    atomic_u32 a{42};
    
    u32 expected = 42;
    bool result = a.compare_exchange_strong(expected, 100, 
                                             memory_order::acq_rel,
                                             memory_order::acquire);
    EXPECT_TRUE(result);
    EXPECT_EQ(a.load(), 100u);
    
    expected = 99;
    result = a.compare_exchange_strong(expected, 200,
                                        memory_order::acq_rel,
                                        memory_order::acquire);
    EXPECT_FALSE(result);
    EXPECT_EQ(expected, 100u);
}

TEST(AtomicU32Test, CompareExchangeWeak) {
    atomic_u32 a{100};
    
    u32 expected = 100;
    while (!a.compare_exchange_weak(expected, 200, memory_order::seq_cst)) {
        expected = 100;
    }
    
    EXPECT_EQ(a.load(), 200u);
}

TEST(AtomicU32Test, FetchAdd) {
    atomic_u32 a{10};
    
    u32 old = a.fetch_add(5);
    EXPECT_EQ(old, 10u);
    EXPECT_EQ(a.load(), 15u);
    
    old = a.fetch_add(20, memory_order::relaxed);
    EXPECT_EQ(old, 15u);
    EXPECT_EQ(a.load(), 35u);
}

TEST(AtomicU32Test, FetchSub) {
    atomic_u32 a{100};
    
    u32 old = a.fetch_sub(30);
    EXPECT_EQ(old, 100u);
    EXPECT_EQ(a.load(), 70u);
    
    old = a.fetch_sub(20, memory_order::relaxed);
    EXPECT_EQ(old, 70u);
    EXPECT_EQ(a.load(), 50u);
}

// =============================================================================
// Concurrent correctness tests
// =============================================================================

TEST(AtomicU32Test, ConcurrentIncrement) {
    atomic_u32 counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kIncrementsPerThread = 250000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIncrementsPerThread; ++i) {
            (void)counter.fetch_add(1, memory_order::relaxed);
        }
    });
    
    EXPECT_EQ(counter.load(), kThreadCount * kIncrementsPerThread);
}

TEST(AtomicU32Test, ProducerConsumer) {
    u32 data = 0;
    atomic_u32 ready{0};
    constexpr u32 kMagicValue = 0xDEADBEEF;
    
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

TEST(AtomicU32Test, MultipleReadersSingleWriter) {
    atomic_u32 value{0};
    atomic_u32 stop{0};
    constexpr int kReaderCount = 3;
    
    std::thread writer([&]() {
        for (int i = 0; i < 1000; ++i) {
            (void)value.fetch_add(1, memory_order::release);
            std::this_thread::yield();
        }
        stop.store(1, memory_order::release);
    });
    
    RunConcurrent(kReaderCount, [&](int) {
        u32 last_seen = 0;
        while (stop.load(memory_order::acquire) == 0) {
            u32 current = value.load(memory_order::acquire);
            EXPECT_GE(current, last_seen);
            last_seen = current;
        }
    });
    
    writer.join();
}

TEST(AtomicU32Test, CompareExchangeRace) {
    atomic_u32 counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kIncrementsPerThread = 10000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIncrementsPerThread; ++i) {
            u32 expected = counter.load(memory_order::relaxed);
            while (!counter.compare_exchange_weak(expected, expected + 1, 
                                                   memory_order::relaxed)) {
            }
        }
    });
    
    EXPECT_EQ(counter.load(), kThreadCount * kIncrementsPerThread);
}

// =============================================================================
// Stress tests (can be disabled on CI with DISABLED_ prefix if flaky)
// =============================================================================

TEST(AtomicU32Test, HighContentionCounter) {
    atomic_u32 counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kOperationsPerThread = 50000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kOperationsPerThread; ++i) {
            (void)counter.fetch_add(1, memory_order::seq_cst);
        }
    });
    
    EXPECT_EQ(counter.load(), kThreadCount * kOperationsPerThread);
}

TEST(AtomicU32Test, StressCAS) {
    atomic_u32 counter{0};
    constexpr int kThreadCount = 4;
    constexpr int kCASPerThread = 5000;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kCASPerThread; ++i) {
            u32 expected = counter.load(memory_order::relaxed);
            while (!counter.compare_exchange_strong(expected, expected + 1, 
                                                     memory_order::relaxed)) {
            }
        }
    });
    
    EXPECT_EQ(counter.load(), kThreadCount * kCASPerThread);
}

TEST(AtomicU32Test, MixedOperations) {
    atomic_u32 value{1000};
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
    
    u32 final = value.load();
    EXPECT_EQ(final, 1000u);
}
