#include <gtest/gtest.h>

#include "core/base/core_static_init.hpp"

namespace {

    struct Counter {
        Counter() : value(1) { ++constructed; }
        int value;
        static int constructed;
    };

    int Counter::constructed = 0;

    CORE_LAZY_STATIC(Counter, GetLazyCounter)

} // namespace

// Compile-time sanity: policy macros must be 0 or 1.
static_assert((CORE_ALLOW_STATIC_INIT == 0) || (CORE_ALLOW_STATIC_INIT == 1),
    "CORE_ALLOW_STATIC_INIT must be 0/1");
static_assert(
    (CORE_STRICT_STATIC_INIT == 0) || (CORE_STRICT_STATIC_INIT == 1),
    "CORE_STRICT_STATIC_INIT must be 0/1");
static_assert(
    (CORE_FORBID_GLOBAL_STATICS == 0) || (CORE_FORBID_GLOBAL_STATICS == 1),
    "CORE_FORBID_GLOBAL_STATICS must be 0/1");

#if CORE_FORBID_GLOBAL_STATICS
static_assert(CORE_ALLOW_STATIC_INIT == 0,
    "CORE_ALLOW_STATIC_INIT must be 0 when globals are forbidden");
#endif

TEST(CoreStaticInit, LazyStaticDoesNotConstructBeforeUse) {
    // The local static inside GetLazyCounter() should not run before first call.
    EXPECT_EQ(Counter::constructed, 0);
}

TEST(CoreStaticInit, LazyStaticConstructsOnce) {
    Counter& a = GetLazyCounter();
    EXPECT_EQ(Counter::constructed, 1);
    EXPECT_EQ(a.value, 1);

    Counter& b = GetLazyCounter();
    EXPECT_EQ(Counter::constructed, 1);
    EXPECT_EQ(&a, &b);
}
