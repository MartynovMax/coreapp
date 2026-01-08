#include "core/concurrency/core_sync.hpp"
#include "core/concurrency/core_atomic.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace core;
using core::test::RunConcurrent;

TEST(SpinMutexTest, DefaultConstruction) {
    spin_mutex mutex;
    SUCCEED();
}

TEST(SpinMutexTest, BasicLockUnlock) {
    spin_mutex mutex;
    
    mutex.lock();
    mutex.unlock();
    
    SUCCEED();
}

TEST(SpinMutexTest, MultipleLockUnlockCycles) {
    spin_mutex mutex;
    
    for (int i = 0; i < 100; ++i) {
        mutex.lock();
        mutex.unlock();
    }
    
    SUCCEED();
}

TEST(SpinMutexTest, TryLockSuccess) {
    spin_mutex mutex;
    
    bool acquired = mutex.try_lock();
    EXPECT_TRUE(acquired);
    
    mutex.unlock();
}

#if CORE_HAS_THREADS || CORE_DEBUG
TEST(SpinMutexTest, TryLockFailure) {
    spin_mutex mutex;
    
    mutex.lock();
    bool acquired = mutex.try_lock();
    EXPECT_FALSE(acquired);
    
    mutex.unlock();
}
#endif

TEST(SpinMutexTest, TryLockAfterUnlock) {
    spin_mutex mutex;
    
    mutex.lock();
    mutex.unlock();
    
    bool acquired = mutex.try_lock();
    EXPECT_TRUE(acquired);
    
    mutex.unlock();
}

#if CORE_HAS_THREADS

TEST(SpinMutexTest, ConcurrentCounterIncrement) {
    constexpr int kThreadCount = 8;
    constexpr int kIterationsPerThread = 10000;
    
    spin_mutex mutex;
    int counter = 0;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            mutex.lock();
            ++counter;
            mutex.unlock();
        }
    });
    
    EXPECT_EQ(counter, kThreadCount * kIterationsPerThread);
}

TEST(SpinMutexTest, ConcurrentStressTest) {
    constexpr int kThreadCount = 16;
    constexpr int kIterationsPerThread = 5000;
    
    spin_mutex mutex;
    int counter = 0;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            mutex.lock();
            ++counter;
            --counter;
            ++counter;
            mutex.unlock();
        }
    });
    
    EXPECT_EQ(counter, kThreadCount * kIterationsPerThread);
}

TEST(SpinMutexTest, ConcurrentTryLockContention) {
    constexpr int kThreadCount = 8;
    constexpr int kIterationsPerThread = 1000;
    
    spin_mutex mutex;
    atomic_u32 success_count{0};
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            if (mutex.try_lock()) {
                (void)success_count.fetch_add(1, memory_order::relaxed);
                mutex.unlock();
            }
        }
    });
    
    EXPECT_GT(success_count.load(), 0u);
}

TEST(SpinMutexTest, MutualExclusionVerification) {
    constexpr int kThreadCount = 8;
    constexpr int kIterationsPerThread = 1000;
    
    spin_mutex mutex;
    atomic_u32 in_critical_section{0};
    atomic_u32 violations{0};
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            mutex.lock();
            u32 current = in_critical_section.fetch_add(1, memory_order::relaxed);
            if (current != 0) {
                (void)violations.fetch_add(1, memory_order::relaxed);
            }
            (void)in_critical_section.fetch_sub(1, memory_order::relaxed);
            mutex.unlock();
        }
    });
    
    EXPECT_EQ(violations.load(), 0u);
}

#endif

#if !CORE_HAS_THREADS

#if !CORE_DEBUG
TEST(SpinMutexTest, SingleThreadNoOpSize) {
    EXPECT_EQ(sizeof(spin_mutex), 1u);
}
#endif

#if CORE_DEBUG
TEST(SpinMutexTest, SingleThreadDebugSize) {
    EXPECT_EQ(sizeof(spin_mutex), sizeof(bool));
}
#endif

#endif

