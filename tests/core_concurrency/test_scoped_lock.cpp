#include "core/concurrency/core_sync.hpp"
#include "core/concurrency/core_atomic.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <thread>

using namespace core;
using core::test::RunConcurrent;

#if CORE_HAS_THREADS || CORE_DEBUG
TEST(ScopedLockTest, LockOnConstruction) {
    spin_mutex mutex;
    
    {
        scoped_lock<spin_mutex> lock(mutex);
        EXPECT_FALSE(mutex.try_lock());
    }
}
#endif

TEST(ScopedLockTest, UnlockOnDestruction) {
    spin_mutex mutex;
    
    {
        scoped_lock<spin_mutex> lock(mutex);
    }
    
    EXPECT_TRUE(mutex.try_lock());
    mutex.unlock();
}

TEST(ScopedLockTest, NestedScopes) {
    spin_mutex mutex;
    int counter = 0;
    
    {
        scoped_lock<spin_mutex> lock1(mutex);
        ++counter;
    }
    
    EXPECT_EQ(counter, 1);
    
    {
        scoped_lock<spin_mutex> lock2(mutex);
        ++counter;
    }
    
    EXPECT_EQ(counter, 2);
}

TEST(ScopedLockTest, WithSpinMutex) {
    spin_mutex mutex;
    int value = 0;
    
    {
        scoped_lock<spin_mutex> lock(mutex);
        value = 42;
    }
    
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(mutex.try_lock());
    mutex.unlock();
}

class MockMutex {
public:
    void lock() noexcept { locked = true; }
    void unlock() noexcept { locked = false; }
    bool is_locked() const { return locked; }
private:
    bool locked = false;
};

TEST(ScopedLockTest, GenericMutexInterface) {
    MockMutex mutex;
    
    EXPECT_FALSE(mutex.is_locked());
    
    {
        scoped_lock<MockMutex> lock(mutex);
        EXPECT_TRUE(mutex.is_locked());
    }
    
    EXPECT_FALSE(mutex.is_locked());
}

#if CORE_HAS_THREADS

TEST(ScopedLockTest, ConcurrentProtection) {
    constexpr int kThreadCount = 8;
    constexpr int kIterationsPerThread = 10000;
    
    spin_mutex mutex;
    int counter = 0;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            scoped_lock<spin_mutex> lock(mutex);
            ++counter;
        }
    });
    
    EXPECT_EQ(counter, kThreadCount * kIterationsPerThread);
}

TEST(ScopedLockTest, ConcurrentStressTest) {
    constexpr int kThreadCount = 16;
    constexpr int kIterationsPerThread = 5000;
    
    spin_mutex mutex;
    int counter = 0;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            scoped_lock<spin_mutex> lock(mutex);
            ++counter;
            --counter;
            ++counter;
        }
    });
    
    EXPECT_EQ(counter, kThreadCount * kIterationsPerThread);
}

TEST(ScopedLockTest, MutualExclusionVerification) {
    constexpr int kThreadCount = 8;
    constexpr int kIterationsPerThread = 1000;
    
    spin_mutex mutex;
    atomic_u32 in_critical_section{0};
    atomic_u32 violations{0};
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            scoped_lock<spin_mutex> lock(mutex);
            u32 current = in_critical_section.fetch_add(1, memory_order::relaxed);
            if (current != 0) {
                (void)violations.fetch_add(1, memory_order::relaxed);
            }
            (void)in_critical_section.fetch_sub(1, memory_order::relaxed);
        }
    });
    
    EXPECT_EQ(violations.load(), 0u);
}

TEST(ScopedLockTest, NestedScopesMultiThreaded) {
    constexpr int kThreadCount = 4;
    constexpr int kIterationsPerThread = 1000;
    
    spin_mutex mutex;
    int counter = 0;
    
    RunConcurrent(kThreadCount, [&](int) {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            {
                scoped_lock<spin_mutex> lock1(mutex);
                ++counter;
            }
            {
                scoped_lock<spin_mutex> lock2(mutex);
                ++counter;
            }
        }
    });
    
    EXPECT_EQ(counter, kThreadCount * kIterationsPerThread * 2);
}

#endif

