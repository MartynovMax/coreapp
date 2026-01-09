#include <gtest/gtest.h>

#include "core/base/core_config.hpp"

namespace {

    CORE_NODISCARD int ComputeValue() { return 42; }

    CORE_NORETURN void NeverReturns() {
        // Do not actually call this in a test.
        for (;;) {}
    }

    CORE_DEPRECATED void OldApi() {}
    CORE_DEPRECATED_MSG("use NewApi() instead") void OldApiWithMsg() {}

    int Branchy(int x) {
        if (CORE_LIKELY(x > 0)) return 1;
        if (CORE_UNLIKELY(x < 0)) return -1;
        return 0;
    }

    int SwitchFallthrough(int v) {
        int out = 0;
        switch (v) {
        case 0:
            out += 1;
            CORE_FALLTHROUGH;
        case 1:
            out += 2;
            break;
        default:
            break;
        }
        return out;
    }

    int AssumeNonZero(int x) {
        CORE_ASSUME(x != 0);
        return 100 / x;
    }

} // namespace

TEST(CoreAttributes, CompilesAndRuns) {
    EXPECT_EQ(Branchy(5), 1);
    EXPECT_EQ(Branchy(0), 0);
    EXPECT_EQ(Branchy(-5), -1);

    EXPECT_EQ(SwitchFallthrough(0), 3);
    EXPECT_EQ(SwitchFallthrough(1), 2);

    EXPECT_EQ(AssumeNonZero(4), 25);

    OldApi();
    OldApiWithMsg();

    EXPECT_EQ(ComputeValue(), 42);

    // Reference to NeverReturns to avoid unused function warning
    (void)&NeverReturns;
}
