#include <gtest/gtest.h>

#include "core/base/core_api.hpp"

struct CORE_API PublicType {
    int value = 0;
};

CORE_API int PublicFunction(int x);

CORE_API_INTERNAL int InternalHelper(int x);

int PublicFunction(int x) { return x + 1; }
int InternalHelper(int x) { return x - 1; }

TEST(CoreApi, MacrosAreDefinedAndUsable) {
    // Basic sanity: macros exist.
#if !defined(CORE_API) || !defined(CORE_API_EXPORT) || !defined(CORE_API_IMPORT)
    FAIL() << "CORE_API macros must be defined.";
#else
    SUCCEED();
#endif

    // Runtime smoke that also forces ODR usage.
    PublicType t;
    t.value = PublicFunction(41);
    EXPECT_EQ(t.value, 42);

    EXPECT_EQ(InternalHelper(10), 9);
}
