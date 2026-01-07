#include "core/concurrency/tls_ptr.hpp"
#include "core/base/core_types.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace core;
using core::test::RunConcurrent;

// =============================================================================
// Basic tls_ptr tests
// =============================================================================

TEST(TlsPtrTest, DefaultValue) {
    struct Dummy {};
    using TLS = tls_ptr<Dummy>;
    
    EXPECT_EQ(TLS::get(), nullptr);
}

TEST(TlsPtrTest, SetAndGet) {
    struct Dummy {};
    using TLS = tls_ptr<Dummy>;
    
    Dummy obj;
    TLS::set(&obj);
    EXPECT_EQ(TLS::get(), &obj);
}

TEST(TlsPtrTest, SetNull) {
    struct Dummy {};
    using TLS = tls_ptr<Dummy>;
    
    Dummy obj;
    TLS::set(&obj);
    EXPECT_EQ(TLS::get(), &obj);
    
    TLS::set(nullptr);
    EXPECT_EQ(TLS::get(), nullptr);
}

TEST(TlsPtrTest, MultipleTypes) {
    struct TypeA {};
    struct TypeB {};
    
    TypeA a;
    TypeB b;
    
    tls_ptr<TypeA>::set(&a);
    tls_ptr<TypeB>::set(&b);
    
    EXPECT_EQ(tls_ptr<TypeA>::get(), &a);
    EXPECT_EQ(tls_ptr<TypeB>::get(), &b);
}

// =============================================================================
// Basic tls_value tests
// =============================================================================

TEST(TlsValueTest, DefaultValue) {
    using TLS = tls_value<int>;
    
    EXPECT_EQ(TLS::get(), 0);
}

TEST(TlsValueTest, SetAndGet) {
    using TLS = tls_value<int>;
    
    TLS::set(42);
    EXPECT_EQ(TLS::get(), 42);
}

TEST(TlsValueTest, MultipleValues) {
    using TLS_int = tls_value<int>;
    using TLS_bool = tls_value<bool>;
    
    TLS_int::set(100);
    TLS_bool::set(true);
    
    EXPECT_EQ(TLS_int::get(), 100);
    EXPECT_EQ(TLS_bool::get(), true);
}

TEST(TlsValueTest, U32Type) {
    using TLS = tls_value<u32>;
    
    TLS::set(0xDEADBEEF);
    EXPECT_EQ(TLS::get(), 0xDEADBEEF);
}

// =============================================================================
// Per-thread isolation tests (only when thread_local is available)
// =============================================================================

#if CORE_HAS_THREADS && CORE_HAS_THREAD_LOCAL

TEST(TlsPtrTest, PerThreadIsolation) {
    struct Data {
        int value = -1;
    };
    using TLS = tls_ptr<Data>;
    
    constexpr int kThreadCount = 4;
    std::vector<Data> objects(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        Data* expected = &objects[thread_id];
        TLS::set(expected);
        
        // Verify thread sees its own value
        EXPECT_EQ(TLS::get(), expected);
        EXPECT_EQ(TLS::get()->value, -1);
        
        // Modify through TLS
        TLS::get()->value = thread_id + 1000;
        EXPECT_EQ(TLS::get()->value, thread_id + 1000);
    });
    
    // Each object should have its unique value
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(objects[i].value, i + 1000);
    }
}

TEST(TlsValueTest, PerThreadIsolation) {
    using TLS = tls_value<int>;
    
    constexpr int kThreadCount = 8;
    std::vector<int> results(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        TLS::set(thread_id * 100);
        
        // Each thread should see its own value
        int value = TLS::get();
        EXPECT_EQ(value, thread_id * 100);
        
        results[thread_id] = value;
    });
    
    // Verify all threads saw their unique values
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(results[i], i * 100);
    }
}

TEST(TlsValueTest, InitialValuePerThread) {
    using TLS = tls_value<int>;
    
    constexpr int kThreadCount = 4;
    std::vector<int> initial_values(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        // Without calling set(), should see default value
        initial_values[thread_id] = TLS::get();
    });
    
    // All threads should see 0 as initial value
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(initial_values[i], 0);
    }
}

TEST(TlsPtrTest, InitialValuePerThread) {
    struct Data {};
    using TLS = tls_ptr<Data>;
    
    constexpr int kThreadCount = 4;
    std::vector<Data*> initial_values(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        // Without calling set(), should see nullptr
        initial_values[thread_id] = TLS::get();
    });
    
    // All threads should see nullptr as initial value
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(initial_values[i], nullptr);
    }
}

TEST(TlsValueTest, MainThreadVsWorkerThreads) {
    using TLS = tls_value<int>;
    
    // Main thread sets a value
    TLS::set(999);
    EXPECT_EQ(TLS::get(), 999);
    
    constexpr int kThreadCount = 4;
    std::vector<int> worker_values(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        // Worker threads should see default value (0), not main's value (999)
        worker_values[thread_id] = TLS::get();
    });
    
    // Workers should have seen initial value
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(worker_values[i], 0);
    }
    
    // Main thread should still have its value
    EXPECT_EQ(TLS::get(), 999);
}

TEST(TlsPtrTest, MainThreadVsWorkerThreads) {
    struct Data { int x; };
    using TLS = tls_ptr<Data>;
    
    Data main_obj{777};
    
    // Main thread sets a value
    TLS::set(&main_obj);
    EXPECT_EQ(TLS::get(), &main_obj);
    
    constexpr int kThreadCount = 4;
    std::vector<Data*> worker_values(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        // Worker threads should see nullptr, not main's pointer
        worker_values[thread_id] = TLS::get();
    });
    
    // Workers should have seen nullptr
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(worker_values[i], nullptr);
    }
    
    // Main thread should still have its pointer
    EXPECT_EQ(TLS::get(), &main_obj);
}

TEST(TlsPtrTest, ConcurrentAccessDifferentTypes) {
    struct TypeA { int a; };
    struct TypeB { int b; };
    
    constexpr int kThreadCount = 4;
    std::vector<TypeA> objects_a(kThreadCount);
    std::vector<TypeB> objects_b(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        tls_ptr<TypeA>::set(&objects_a[thread_id]);
        tls_ptr<TypeB>::set(&objects_b[thread_id]);
        
        tls_ptr<TypeA>::get()->a = thread_id;
        tls_ptr<TypeB>::get()->b = thread_id * 2;
        
        EXPECT_EQ(tls_ptr<TypeA>::get()->a, thread_id);
        EXPECT_EQ(tls_ptr<TypeB>::get()->b, thread_id * 2);
    });
}

// =============================================================================
// Stress tests
// =============================================================================

TEST(TlsPtrTest, StressGetSet) {
    struct Counter {
        int value = 0;
    };
    using TLS = tls_ptr<Counter>;
    
    constexpr int kThreadCount = 8;
    constexpr int kIterations = 1000;
    
    std::vector<Counter> counters(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        Counter* counter = &counters[thread_id];
        
        for (int i = 0; i < kIterations; ++i) {
            TLS::set(counter);
            EXPECT_EQ(TLS::get(), counter);
            TLS::get()->value++;
        }
    });
    
    // Each counter should have kIterations increments
    for (int i = 0; i < kThreadCount; ++i) {
        EXPECT_EQ(counters[i].value, kIterations);
    }
}

TEST(TlsValueTest, StressGetSet) {
    using TLS = tls_value<u32>;
    
    constexpr int kThreadCount = 8;
    constexpr int kIterations = 1000;
    
    std::vector<u32> results(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        u32 sum = 0;
        
        for (int i = 0; i < kIterations; ++i) {
            TLS::set(thread_id + i);
            sum += TLS::get();
        }
        
        results[thread_id] = sum;
    });
    
    // Verify each thread computed its sum correctly
    // Sum formula: sum of (thread_id + i) for i in [0, kIterations)
    //            = kIterations * thread_id + kIterations * (kIterations - 1) / 2
    for (int i = 0; i < kThreadCount; ++i) {
        u32 expected_sum = kIterations * i + kIterations * (kIterations - 1) / 2;
        EXPECT_EQ(results[i], expected_sum);
    }
}

TEST(TlsPtrTest, SetNullUnderConcurrency) {
    struct Data { int x; };
    using TLS = tls_ptr<Data>;
    
    constexpr int kThreadCount = 4;
    std::vector<Data> objects(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        Data* obj = &objects[thread_id];
        
        for (int i = 0; i < 100; ++i) {
            TLS::set(obj);
            EXPECT_EQ(TLS::get(), obj);
            
            TLS::set(nullptr);
            EXPECT_EQ(TLS::get(), nullptr);
        }
    });
}

#endif // CORE_HAS_THREADS && CORE_HAS_THREAD_LOCAL

// =============================================================================
// Fallback behavior: process-wide storage when thread_local is unavailable
// =============================================================================

#if CORE_HAS_THREADS && !CORE_HAS_THREAD_LOCAL

TEST(TlsPtrTest, FallbackIsProcessWide) {
    struct Data { int x; };
    using TLS = tls_ptr<Data>;
    
    Data obj1{111};
    Data obj2{222};
    
    TLS::set(nullptr);
    
    // Sequential writes from different threads to verify process-wide storage
    std::thread t1([&] { TLS::set(&obj1); });
    t1.join();
    EXPECT_EQ(TLS::get(), &obj1);
    
    std::thread t2([&] { TLS::set(&obj2); });
    t2.join();
    EXPECT_EQ(TLS::get(), &obj2);
}

TEST(TlsValueTest, FallbackIsProcessWide) {
    using TLS = tls_value<int>;
    
    TLS::set(0);
    
    // Sequential writes from different threads to verify process-wide storage
    std::thread t1([] { TLS::set(111); });
    t1.join();
    EXPECT_EQ(TLS::get(), 111);
    
    std::thread t2([] { TLS::set(222); });
    t2.join();
    EXPECT_EQ(TLS::get(), 222);
}

#endif // CORE_HAS_THREADS && !CORE_HAS_THREAD_LOCAL

// =============================================================================
// Single-thread fallback behavior
// =============================================================================

TEST(TlsPtrTest, SingleThreadBehavior) {
    struct Data { int x = 0; };
    using TLS = tls_ptr<Data>;
    
    Data obj1{10};
    Data obj2{20};
    
    TLS::set(&obj1);
    EXPECT_EQ(TLS::get(), &obj1);
    EXPECT_EQ(TLS::get()->x, 10);
    
    TLS::set(&obj2);
    EXPECT_EQ(TLS::get(), &obj2);
    EXPECT_EQ(TLS::get()->x, 20);
}

TEST(TlsValueTest, SingleThreadBehavior) {
    using TLS = tls_value<int>;
    
    TLS::set(100);
    EXPECT_EQ(TLS::get(), 100);
    
    TLS::set(200);
    EXPECT_EQ(TLS::get(), 200);
    
    TLS::set(0);
    EXPECT_EQ(TLS::get(), 0);
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(TlsPtrTest, RapidSetGet) {
    struct Data {};
    using TLS = tls_ptr<Data>;
    
    Data obj;
    
    for (int i = 0; i < 1000; ++i) {
        TLS::set(&obj);
        EXPECT_EQ(TLS::get(), &obj);
    }
}

TEST(TlsValueTest, RapidSetGet) {
    using TLS = tls_value<u32>;
    
    for (u32 i = 0; i < 1000; ++i) {
        TLS::set(i);
        EXPECT_EQ(TLS::get(), i);
    }
}

TEST(TlsValueTest, BoolType) {
    using TLS = tls_value<bool>;
    
    TLS::set(true);
    EXPECT_TRUE(TLS::get());
    
    TLS::set(false);
    EXPECT_FALSE(TLS::get());
}

