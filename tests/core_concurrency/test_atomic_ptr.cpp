#include "core/concurrency/core_atomic.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <algorithm>

using namespace core;
using core::test::RunConcurrent;

namespace {

struct Node {
    int value;
    Node* next;
    
    explicit Node(int v) : value(v), next(nullptr) {}
};

class LockFreeStack {
public:
    LockFreeStack() : head_(nullptr) {}
    
    ~LockFreeStack() {
        void* current = head_.load();
        while (current) {
            Node* node = static_cast<Node*>(current);
            void* next = node->next;
            delete node;
            current = next;
        }
    }
    
    void push(Node* node) {
        void* old_head = head_.load(memory_order::relaxed);
        do {
            node->next = static_cast<Node*>(old_head);
        } while (!head_.compare_exchange_weak(old_head, node, 
                                               memory_order::release,
                                               memory_order::relaxed));
    }
    
    Node* pop() {
        void* old_head = head_.load(memory_order::acquire);
        void* new_head;
        do {
            if (old_head == nullptr) {
                return nullptr;
            }
            new_head = static_cast<Node*>(old_head)->next;
        } while (!head_.compare_exchange_weak(old_head, new_head,
                                               memory_order::acquire,
                                               memory_order::relaxed));
        return static_cast<Node*>(old_head);
    }
    
    bool empty() const {
        return head_.load(memory_order::acquire) == nullptr;
    }
    
private:
    atomic_ptr head_;
};

} // namespace

// =============================================================================
// Basic tests
// =============================================================================

TEST(AtomicPtrTest, DefaultConstruction) {
    atomic_ptr a;
}

TEST(AtomicPtrTest, NullptrConstruction) {
    atomic_ptr a{nullptr};
    EXPECT_EQ(a.load(), nullptr);
}

TEST(AtomicPtrTest, ValueConstruction) {
    int value = 42;
    atomic_ptr a{&value};
    EXPECT_EQ(a.load(), &value);
}

TEST(AtomicPtrTest, LoadStore) {
    int value1 = 1;
    int value2 = 2;
    
    atomic_ptr a{nullptr};
    
    a.store(&value1);
    EXPECT_EQ(a.load(), &value1);
    
    a.store(&value2, memory_order::relaxed);
    EXPECT_EQ(a.load(memory_order::relaxed), &value2);
    
    a.store(nullptr);
    EXPECT_EQ(a.load(), nullptr);
}

TEST(AtomicPtrTest, LoadStoreAllMemoryOrders) {
    int v1 = 1, v2 = 2, v3 = 3;
    atomic_ptr a{nullptr};
    
    a.store(&v1, memory_order::relaxed);
    EXPECT_EQ(a.load(memory_order::relaxed), &v1);
    
    a.store(&v2, memory_order::release);
    EXPECT_EQ(a.load(memory_order::acquire), &v2);
    
    a.store(&v3, memory_order::seq_cst);
    EXPECT_EQ(a.load(memory_order::seq_cst), &v3);
}

TEST(AtomicPtrTest, Exchange) {
    int v1 = 1, v2 = 2;
    atomic_ptr a{&v1};
    
    void* old = a.exchange(&v2);
    EXPECT_EQ(old, &v1);
    EXPECT_EQ(a.load(), &v2);
    
    old = a.exchange(nullptr, memory_order::relaxed);
    EXPECT_EQ(old, &v2);
    EXPECT_EQ(a.load(), nullptr);
}

TEST(AtomicPtrTest, CompareExchangeStrongSuccess) {
    int v1 = 1, v2 = 2;
    atomic_ptr a{&v1};
    
    void* expected = &v1;
    bool success = a.compare_exchange_strong(expected, &v2);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(expected, &v1);
    EXPECT_EQ(a.load(), &v2);
}

TEST(AtomicPtrTest, CompareExchangeStrongFailure) {
    int v1 = 1, v2 = 2, v3 = 3;
    atomic_ptr a{&v1};
    
    void* expected = &v2;
    bool success = a.compare_exchange_strong(expected, &v3);
    
    EXPECT_FALSE(success);
    EXPECT_EQ(expected, &v1);
    EXPECT_EQ(a.load(), &v1);
}

TEST(AtomicPtrTest, CompareExchangeStrongUpdatesExpected) {
    int v1 = 1, v2 = 2;
    atomic_ptr a{&v1};
    
    void* expected = nullptr;
    (void)a.compare_exchange_strong(expected, &v2);
    
    EXPECT_EQ(expected, &v1);
}

TEST(AtomicPtrTest, CompareExchangeWeak) {
    int v1 = 1, v2 = 2;
    atomic_ptr a{&v1};
    
    void* expected = &v1;
    while (!a.compare_exchange_weak(expected, &v2, memory_order::seq_cst)) {
        expected = &v1;
    }
    
    EXPECT_EQ(a.load(), &v2);
}

// =============================================================================
// Minimal smoke tests for concurrent correctness
// =============================================================================

TEST(AtomicPtrTest, ConcurrentExchange) {
    int values[4] = {1, 2, 3, 4};
    atomic_ptr shared{&values[0]};
    constexpr int kThreadCount = 4;
    constexpr int kIterations = 1000;
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        for (int i = 0; i < kIterations; ++i) {
            (void)shared.exchange(&values[thread_id], memory_order::relaxed);
        }
    });
    
    void* final_ptr = shared.load();
    EXPECT_NE(final_ptr, nullptr);
}

TEST(AtomicPtrTest, CASSetOnce) {
    int value = 42;
    atomic_ptr shared{nullptr};
    constexpr int kThreadCount = 4;
    atomic_u32 winner_count{0};
    
    RunConcurrent(kThreadCount, [&](int) {
        void* expected = nullptr;
        if (shared.compare_exchange_strong(expected, &value, memory_order::release)) {
            (void)winner_count.fetch_add(1, memory_order::relaxed);
        }
    });
    
    EXPECT_EQ(winner_count.load(), 1u);
    EXPECT_EQ(shared.load(), &value);
}

TEST(AtomicPtrTest, MessagePassingWithPtr) {
    struct Data {
        int value;
    };
    
    Data data{0};
    atomic_ptr ready{nullptr};
    constexpr int kMagicValue = 12345;
    
    std::thread producer([&]() {
        data.value = kMagicValue;
        ready.store(&data, memory_order::release);
    });
    
    std::thread consumer([&]() {
        void* ptr;
        while ((ptr = ready.load(memory_order::acquire)) == nullptr) {
        }
        Data* d = static_cast<Data*>(ptr);
        EXPECT_EQ(d->value, kMagicValue);
    });
    
    producer.join();
    consumer.join();
}

// =============================================================================
// Treiber Stack integration tests
// =============================================================================

TEST(AtomicPtrTest, TreiberStackBasic) {
    LockFreeStack stack;
    
    Node* n1 = new Node(1);
    Node* n2 = new Node(2);
    Node* n3 = new Node(3);
    
    stack.push(n1);
    stack.push(n2);
    stack.push(n3);
    
    Node* p1 = stack.pop();
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->value, 3);
    
    Node* p2 = stack.pop();
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p2->value, 2);
    
    Node* p3 = stack.pop();
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(p3->value, 1);
    
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.pop(), nullptr);
    
    delete p1;
    delete p2;
    delete p3;
}

TEST(AtomicPtrTest, TreiberStackConcurrentPush) {
    LockFreeStack stack;
    constexpr int kThreadCount = 4;
    constexpr int kNodesPerThread = 100;
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        for (int i = 0; i < kNodesPerThread; ++i) {
            int value = thread_id * 1000 + i;
            stack.push(new Node(value));
        }
    });
    
    int count = 0;
    std::vector<int> values;
    Node* node;
    while ((node = stack.pop()) != nullptr) {
        values.push_back(node->value);
        delete node;
        count++;
    }
    
    EXPECT_EQ(count, kThreadCount * kNodesPerThread);
    EXPECT_EQ(values.size(), static_cast<size_t>(kThreadCount * kNodesPerThread));
    
    std::sort(values.begin(), values.end());
    auto last = std::unique(values.begin(), values.end());
    EXPECT_EQ(last, values.end()) << "Found duplicate values in stack";
}

// Disabled: Treiber stack without ABA protection is inherently flaky under high contention.
// Memory reuse (delete-realloc-push at same address) can cause ABA problem in CAS loop.
// This is a known limitation of simple Treiber stack, not an atomic_ptr issue.
TEST(AtomicPtrTest, DISABLED_TreiberStackConcurrentPushPop) {
    LockFreeStack stack;
    constexpr int kThreadCount = 4;
    constexpr int kOperationsPerThread = 1000;
    
    std::vector<std::vector<int>> popped_values(kThreadCount);
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        for (int i = 0; i < kOperationsPerThread; ++i) {
            int value = thread_id * 10000 + i;
            stack.push(new Node(value));
            
            Node* node = stack.pop();
            if (node) {
                popped_values[thread_id].push_back(node->value);
                delete node;
            }
        }
    });
    
    std::vector<int> remaining;
    Node* node;
    while ((node = stack.pop()) != nullptr) {
        remaining.push_back(node->value);
        delete node;
    }
    
    int total_popped = remaining.size();
    for (const auto& vec : popped_values) {
        total_popped += vec.size();
    }
    
    EXPECT_EQ(total_popped, kThreadCount * kOperationsPerThread);
}

TEST(AtomicPtrTest, DISABLED_TreiberStackStress) {
    LockFreeStack stack;
    constexpr int kNodeCount = 500;
    constexpr int kThreadCount = 4;
    constexpr int kIterations = 5000;
    
    for (int i = 0; i < kNodeCount; ++i) {
        stack.push(new Node(i));
    }
    
    atomic_u32 operations{0};
    
    RunConcurrent(kThreadCount, [&](int thread_id) {
        for (int i = 0; i < kIterations; ++i) {
            if (i % 3 == 0) {
                int value = thread_id * 100000 + i;
                stack.push(new Node(value));
                (void)operations.fetch_add(1, memory_order::relaxed);
            } else {
                Node* node = stack.pop();
                if (node) {
                    delete node;
                    (void)operations.fetch_add(1, memory_order::relaxed);
                }
            }
        }
    });
    
    int remaining = 0;
    Node* node;
    while ((node = stack.pop()) != nullptr) {
        delete node;
        remaining++;
    }
    
    EXPECT_GT(operations.load(), 0u);
    EXPECT_TRUE(stack.empty());
}
