#include "core/concurrency/cpu_relax.hpp"
#include <gtest/gtest.h>

using namespace core;

TEST(CpuRelaxTest, CompileAndRun) {
    cpu_relax();
    SUCCEED();
}

TEST(CpuRelaxTest, CallInLoop) {
    for (int i = 0; i < 1000; ++i) {
        cpu_relax();
    }
    SUCCEED();
}

TEST(CpuRelaxTest, NoSideEffects) {
    int x = 42;
    cpu_relax();
    EXPECT_EQ(x, 42);
    
    for (int i = 0; i < 100; ++i) {
        cpu_relax();
    }
    EXPECT_EQ(x, 42);
}

