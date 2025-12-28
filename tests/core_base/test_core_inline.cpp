#include <gtest/gtest.h>

#include "core/base/core_inline.hpp"

namespace {

CORE_FORCE_INLINE int ForceAdd(int a, int b) { return a + b; }

CORE_NO_INLINE int NoInlineAdd(int a, int b) { return a + b; }

CORE_NO_INLINE_DEBUG int DebugOnlyNoInline(int x) { return x + 1; }

CORE_FLATTEN int FlattenCombo(int x) {
  // Split calls so that the attributes are at least syntactically exercised.
  const int a = ForceAdd(x, 1);
  const int b = NoInlineAdd(x, 2);
  const int c = DebugOnlyNoInline(x);
  return a + b + c;
}

} // namespace

TEST(CoreInline, Smoke) {
  EXPECT_EQ(ForceAdd(10, 20), 30);
  EXPECT_EQ(NoInlineAdd(10, 20), 30);
  EXPECT_EQ(DebugOnlyNoInline(41), 42);
  EXPECT_EQ(FlattenCombo(10), 10 + 1 + 10 + 2 + 10 + 1);
}
