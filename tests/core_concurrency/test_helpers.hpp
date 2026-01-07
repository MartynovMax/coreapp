// tests/core_concurrency/test_helpers.hpp
// Common test utilities for concurrency tests

#pragma once

#include <thread>
#include <vector>
#include <functional>

namespace core::test {

inline void RunConcurrent(int thread_count, std::function<void(int)> func) {
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(func, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

} // namespace core::test

