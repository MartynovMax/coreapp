#include "core/concurrency/core_atomic.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace core;
using core::test::RunConcurrent;

// =============================================================================
// thread_fence tests
// =============================================================================

TEST(AtomicFenceTest, ThreadFenceCompiles) {
    thread_fence(memory_order::relaxed);
    thread_fence(memory_order::acquire);
    thread_fence(memory_order::release);
    thread_fence(memory_order::acq_rel);
    thread_fence(memory_order::seq_cst);
    
    SUCCEED();
}

TEST(AtomicFenceTest, MessagePassing) {
    u32 data = 0;
    atomic_u32 ready{0};
    constexpr u32 kMagicValue = 0xDEADBEEF;
    
    std::thread producer([&]() {
        data = kMagicValue;
        thread_fence(memory_order::release);
        ready.store(1, memory_order::relaxed);
    });
    
    std::thread consumer([&]() {
        while (ready.load(memory_order::relaxed) == 0) {
            std::this_thread::yield();
        }
        thread_fence(memory_order::acquire);
        EXPECT_EQ(data, kMagicValue);
    });
    
    producer.join();
    consumer.join();
}

TEST(AtomicFenceTest, MessagePassingMultipleData) {
    u32 data1 = 0;
    u32 data2 = 0;
    u32 data3 = 0;
    atomic_u32 ready{0};
    
    std::thread producer([&]() {
        data1 = 111;
        data2 = 222;
        data3 = 333;
        
        thread_fence(memory_order::release);
        ready.store(1, memory_order::relaxed);
    });
    
    std::thread consumer([&]() {
        while (ready.load(memory_order::relaxed) == 0) {
            std::this_thread::yield();
        }
        thread_fence(memory_order::acquire);
        
        EXPECT_EQ(data1, 111u);
        EXPECT_EQ(data2, 222u);
        EXPECT_EQ(data3, 333u);
    });
    
    producer.join();
    consumer.join();
}

TEST(AtomicFenceTest, AcqRelFence) {
    u32 data = 0;
    atomic_u32 flag{0};
    
    std::thread t1([&]() {
        data = 42;
        thread_fence(memory_order::release);
        flag.store(1, memory_order::relaxed);
    });
    
    std::thread t2([&]() {
        while (flag.load(memory_order::relaxed) == 0) {
            std::this_thread::yield();
        }
        thread_fence(memory_order::acq_rel);
        EXPECT_EQ(data, 42u);
    });
    
    t1.join();
    t2.join();
}

TEST(AtomicFenceTest, DISABLED_ProducerConsumerChain) {
    constexpr int kStages = 5;
    u32 data[kStages] = {};
    atomic_u32 stage{0};
    
    std::thread producer([&]() {
        for (int i = 0; i < kStages; ++i) {
            data[i] = (i + 1) * 100;
            thread_fence(memory_order::release);
            stage.store(i + 1, memory_order::relaxed);
            std::this_thread::yield();
        }
    });
    
    std::thread consumer([&]() {
        for (int i = 0; i < kStages; ++i) {
            while (stage.load(memory_order::relaxed) <= static_cast<u32>(i)) {}
            thread_fence(memory_order::acquire);
            EXPECT_EQ(data[i], static_cast<u32>((i + 1) * 100));
        }
    });
    
    producer.join();
    consumer.join();
}

// =============================================================================
// signal_fence tests
// =============================================================================

TEST(AtomicFenceTest, SignalFenceCompiles) {
    signal_fence(memory_order::relaxed);
    signal_fence(memory_order::acquire);
    signal_fence(memory_order::release);
    signal_fence(memory_order::acq_rel);
    signal_fence(memory_order::seq_cst);
    
    SUCCEED();
}

TEST(AtomicFenceTest, SignalFenceBasic) {
    volatile int data = 0;
    atomic_u32 flag{0};
    
    data = 42;
    signal_fence(memory_order::release);
    flag.store(1, memory_order::relaxed);
    
    if (flag.load(memory_order::relaxed) != 0) {
        signal_fence(memory_order::acquire);
        EXPECT_EQ(data, 42);
    }
}

TEST(AtomicFenceTest, SignalFenceVsThreadFence) {
    u32 data = 0;
    atomic_u32 ready{0};
    
    data = 100;
    thread_fence(memory_order::release);
    ready.store(1, memory_order::relaxed);
    
    EXPECT_EQ(ready.load(), 1u);
    EXPECT_EQ(data, 100u);
    
    data = 200;
    signal_fence(memory_order::release);
    EXPECT_EQ(data, 200u);
}

TEST(AtomicFenceTest, MixedFences) {
    volatile int local_data = 0;
    u32 shared_data = 0;
    atomic_u32 flag{0};
    
    local_data = 1;
    signal_fence(memory_order::seq_cst);
    EXPECT_EQ(local_data, 1);
    
    shared_data = 2;
    thread_fence(memory_order::release);
    flag.store(1, memory_order::relaxed);
    
    EXPECT_EQ(flag.load(), 1u);
}

// =============================================================================
// Combined tests
// =============================================================================

TEST(AtomicFenceTest, FenceWithAtomicOps) {
    atomic_u32 counter{0};
    u32 data = 0;
    
    std::thread t1([&]() {
        for (int i = 0; i < 100; ++i) {
            data = i;
            thread_fence(memory_order::release);
            (void)counter.fetch_add(1, memory_order::relaxed);
        }
    });
    
    std::thread t2([&]() {
        u32 last_count = 0;
        while (last_count < 100) {
            u32 current = counter.load(memory_order::relaxed);
            if (current > last_count) {
                thread_fence(memory_order::acquire);
                EXPECT_LT(data, 100u);
                if (last_count > 0) {
                    EXPECT_GE(data, last_count - 1);
                }
                last_count = current;
            }
        }
    });
    
    t1.join();
    t2.join();
}

// =============================================================================
// Litmus tests (can be flaky, disabled by default)
// =============================================================================

// Classic Dekker-style test: with seq_cst fences, both cannot see 0
TEST(AtomicFenceTest, DISABLED_SeqCstFenceLitmus) {
    constexpr int kIterations = 10000;
    int violations = 0;
    
    for (int iter = 0; iter < kIterations; ++iter) {
        atomic_u32 x{0};
        atomic_u32 y{0};
        int r1 = 0, r2 = 0;
        
        std::thread t1([&]() {
            x.store(1, memory_order::relaxed);
            thread_fence(memory_order::seq_cst);
            r1 = y.load(memory_order::relaxed);
        });
        
        std::thread t2([&]() {
            y.store(1, memory_order::relaxed);
            thread_fence(memory_order::seq_cst);
            r2 = x.load(memory_order::relaxed);
        });
        
        t1.join();
        t2.join();
        
        if (r1 == 0 && r2 == 0) {
            violations++;
        }
    }
    
    EXPECT_EQ(violations, 0) << "Found " << violations << " violations in " << kIterations << " iterations";
}
