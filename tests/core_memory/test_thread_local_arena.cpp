#include "core/memory/thread_local_arena.hpp"
#include "core/memory/scoped_arena.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

namespace {

using namespace core;

// =============================================================================
// Test fixture for TLS state cleanup
// =============================================================================

class ThreadLocalArenaTest : public ::testing::Test {
protected:
    void SetUp() override {
        DestroyThreadLocalArena();
    }
    
    void TearDown() override {
        DestroyThreadLocalArena();
    }
};

// =============================================================================
// Single-thread tests
// =============================================================================

TEST_F(ThreadLocalArenaTest, LazyInitialization) {
    EXPECT_FALSE(HasThreadLocalArena());
    
    IArena& arena = GetThreadLocalArena();
    
    EXPECT_TRUE(HasThreadLocalArena());
    EXPECT_NE(&arena, nullptr);
}

TEST_F(ThreadLocalArenaTest, BasicAllocation) {
    IArena& arena = GetThreadLocalArena();
    
    void* ptr = arena.Allocate(256);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_GT(arena.Used(), 0);
}

TEST_F(ThreadLocalArenaTest, Reset) {
    IArena& arena = GetThreadLocalArena();
    
    arena.Allocate(512);
    memory_size used = arena.Used();
    EXPECT_GT(used, 0);
    
    ResetThreadLocalArena();
    
    EXPECT_EQ(arena.Used(), 0);
}

TEST_F(ThreadLocalArenaTest, ResetWithoutInitialization) {
    EXPECT_FALSE(HasThreadLocalArena());
    
    ResetThreadLocalArena();
    
    EXPECT_FALSE(HasThreadLocalArena());
}

TEST_F(ThreadLocalArenaTest, WorksWithScopedArena) {
    IArena& arena = GetThreadLocalArena();
    memory_size initial = arena.Used();
    
    {
        ScopedArena scope(GetThreadLocalArena());
        void* ptr = scope.Allocate(128);
        ASSERT_NE(ptr, nullptr);
        EXPECT_GT(arena.Used(), initial);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

TEST_F(ThreadLocalArenaTest, NestedScopedArena) {
    IArena& arena = GetThreadLocalArena();
    arena.Reset();
    
    memory_size initial = arena.Used();
    
    {
        ScopedArena outer(GetThreadLocalArena());
        outer.Allocate(64);
        memory_size afterOuter = arena.Used();
        EXPECT_GT(afterOuter, initial);
        
        {
            ScopedArena inner(GetThreadLocalArena());
            inner.Allocate(128);
            memory_size afterInner = arena.Used();
            EXPECT_GT(afterInner, afterOuter);
        }
        
        EXPECT_EQ(arena.Used(), afterOuter);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

TEST_F(ThreadLocalArenaTest, HasName) {
    IArena& arena = GetThreadLocalArena();
    
    const char* name = arena.Name();
    ASSERT_NE(name, nullptr);
    EXPECT_NE(name[0], '\0');
}

TEST_F(ThreadLocalArenaTest, Introspection) {
    IArena& arena = GetThreadLocalArena();
    arena.Reset();
    
    memory_size capacity = arena.Capacity();
    EXPECT_GT(capacity, 0);
    
    memory_size initialUsed = arena.Used();
    memory_size initialRemaining = arena.Remaining();
    
    EXPECT_EQ(initialUsed, 0);
    EXPECT_EQ(initialRemaining, capacity);
    
    void* ptr = arena.Allocate(256);
    ASSERT_NE(ptr, nullptr);
    
    memory_size afterUsed = arena.Used();
    memory_size afterRemaining = arena.Remaining();
    
    EXPECT_GT(afterUsed, initialUsed);
    EXPECT_LT(afterRemaining, initialRemaining);
    EXPECT_EQ(afterUsed + afterRemaining, capacity);
}

TEST_F(ThreadLocalArenaTest, ExplicitDestroy) {
    GetThreadLocalArena();
    EXPECT_TRUE(HasThreadLocalArena());
    
    DestroyThreadLocalArena();
    
    EXPECT_FALSE(HasThreadLocalArena());
}

TEST_F(ThreadLocalArenaTest, ReinitAfterDestroy) {
    GetThreadLocalArena();
    DestroyThreadLocalArena();
    EXPECT_FALSE(HasThreadLocalArena());
    
    IArena& arena = GetThreadLocalArena();
    
    EXPECT_TRUE(HasThreadLocalArena());
    EXPECT_NE(&arena, nullptr);
    EXPECT_EQ(arena.Used(), 0);
}

// =============================================================================
// Multi-thread tests
// =============================================================================

TEST(ThreadLocalArena, MultipleThreadsGetDifferentArenas) {
    std::vector<uintptr_t> arenaAddresses;
    std::mutex mtx;
    
    auto worker = [&]() {
        IArena& arena = GetThreadLocalArena();
        std::lock_guard<std::mutex> lock(mtx);
        // Store address as integer to avoid dangling pointer issues
        arenaAddresses.push_back(reinterpret_cast<uintptr_t>(&arena));
    };
    
    constexpr int kThreadCount = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    
    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ASSERT_EQ(arenaAddresses.size(), kThreadCount);
    
    // Verify all addresses are distinct (each thread has its own arena)
    for (size_t i = 0; i < arenaAddresses.size(); ++i) {
        EXPECT_NE(arenaAddresses[i], 0u);
        for (size_t j = i + 1; j < arenaAddresses.size(); ++j) {
            EXPECT_NE(arenaAddresses[i], arenaAddresses[j]);
        }
    }
}

TEST(ThreadLocalArena, ThreadsHaveIndependentState) {
    std::mutex mtx;
    std::vector<memory_size> usedValues;
    std::atomic<int> failures{0};
    
    auto worker = [&](int id) {
        IArena& arena = GetThreadLocalArena();
        arena.Reset();
        
        memory_size requested = 128 * (id + 1);
        void* ptr = arena.Allocate(requested);
        
        bool ok = (ptr != nullptr);
        memory_size used = arena.Used();
        ok = ok && (used >= requested);
        
        if (!ok) {
            failures.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        
        std::lock_guard<std::mutex> lock(mtx);
        usedValues.push_back(used);
    };
    
    constexpr int kThreadCount = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    
    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(failures.load(), 0);
    ASSERT_EQ(usedValues.size(), kThreadCount);
    
    // Check that not all values are the same (threads are independent)
    memory_size minUsed = usedValues[0];
    memory_size maxUsed = usedValues[0];
    for (size_t i = 1; i < usedValues.size(); ++i) {
        if (usedValues[i] < minUsed) minUsed = usedValues[i];
        if (usedValues[i] > maxUsed) maxUsed = usedValues[i];
    }
    
    EXPECT_NE(minUsed, maxUsed);
}

TEST(ThreadLocalArena, StressTest_ManyThreadsCreatedAndDestroyed) {
    constexpr int kIterations = 10;
    constexpr int kThreadsPerIteration = 8;
    
    for (int iter = 0; iter < kIterations; ++iter) {
        std::vector<std::thread> threads;
        std::atomic<int> failures{0};
        threads.reserve(kThreadsPerIteration);
        
        for (int i = 0; i < kThreadsPerIteration; ++i) {
            threads.emplace_back([i, &failures]() {
                IArena& arena = GetThreadLocalArena();
                
                bool ok = true;
                for (int j = 0; j < 100; ++j) {
                    ScopedArena scope(arena);
                    void* ptr = scope.Allocate(64 + j);
                    if (ptr == nullptr) {
                        ok = false;
                        break;
                    }
                }
                
                if (!ok) {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
                
                DestroyThreadLocalArena();
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        EXPECT_EQ(failures.load(), 0);
    }
}

TEST(ThreadLocalArena, ScopedArenaInMultipleThreads) {
    std::atomic<int> successCount{0};
    
    auto worker = []() {
        IArena& arena = GetThreadLocalArena();
        arena.Reset();
        
        memory_size initial = arena.Used();
        
        {
            ScopedArena scope(arena);
            void* ptr = scope.Allocate(256);
            if (ptr != nullptr && arena.Used() > initial) {
                // Success
            } else {
                return false;
            }
        }
        
        return arena.Used() == initial;
    };
    
    constexpr int kThreadCount = 8;
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    
    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([&]() {
            if (worker()) {
                successCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(successCount.load(), kThreadCount);
}

} // namespace

