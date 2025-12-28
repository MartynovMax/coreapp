#include "core/base/core_assert.hpp"

#include <gtest/gtest.h>

#include <cstring> // std::strstr
#include <string>

// NOTE: Tests intentionally use the standard library.
// Core headers remain STL-free.

namespace {
struct Record {
  int calls = 0;
  const char *expr = nullptr;
  const char *msg = nullptr;
  const char *file = nullptr;
  int line = 0;
};

static Record g_rec{};

void ResetRecord() { g_rec = {}; }

void RecordingHandler(const char *expr_text, const char *msg, const char *file,
                      int line) {
  ++g_rec.calls;
  g_rec.expr = expr_text;
  g_rec.msg = msg;
  g_rec.file = file;
  g_rec.line = line;
  // Do NOT terminate. This allows testing in-process.
}

struct AssertHandlerGuard {
  core::AssertHandler prev;
  explicit AssertHandlerGuard(core::AssertHandler h)
      : prev(core::SetAssertHandler(h)) {}
  ~AssertHandlerGuard() { core::SetAssertHandler(prev); }

  AssertHandlerGuard(const AssertHandlerGuard &) = delete;
  AssertHandlerGuard &operator=(const AssertHandlerGuard &) = delete;
};

static int g_side_effect = 0;

bool SideEffectFalse() {
  ++g_side_effect;
  return false;
}

bool SideEffectTrue() {
  ++g_side_effect;
  return true;
}

bool FileContainsTestName(const char *file) {
  if (!file)
    return false;
  return std::strstr(file, "test_core_assert") != nullptr;
}
} // namespace

// 1) Correct assertion triggering in Debug (only when enabled).
#if CORE_ASSERTIONS_ENABLED
TEST(CoreAssert, AssertDispatchesOnceAndCapturesLocation) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  constexpr int expectedLine = __LINE__ + 1;
  ASSERT(false);

  EXPECT_EQ(g_rec.calls, 1);
  EXPECT_EQ(std::string(g_rec.expr), std::string("false"));
  EXPECT_EQ(g_rec.msg, nullptr);
  EXPECT_TRUE(FileContainsTestName(g_rec.file));
  EXPECT_EQ(g_rec.line, expectedLine);
}

TEST(CoreAssert, AssertMsgForwardsMessage) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  constexpr int expectedLine = __LINE__ + 1;
  ASSERT_MSG(false, "boom");

  EXPECT_EQ(g_rec.calls, 1);
  EXPECT_EQ(std::string(g_rec.expr), std::string("false"));
  ASSERT_NE(g_rec.msg, nullptr);
  EXPECT_EQ(std::string(g_rec.msg), std::string("boom"));
  EXPECT_TRUE(FileContainsTestName(g_rec.file));
  EXPECT_EQ(g_rec.line, expectedLine);
}

TEST(CoreAssert, AssertStringifiesExpression) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  constexpr int expectedLine = __LINE__ + 1;
  ASSERT(1 + 2 == 4);

  EXPECT_EQ(g_rec.calls, 1);
  EXPECT_EQ(std::string(g_rec.expr), std::string("1 + 2 == 4"));
  EXPECT_EQ(g_rec.line, expectedLine);
}
#endif // CORE_ASSERTIONS_ENABLED

// 2) Assertions compiling out in Release: ASSERT must not evaluate expression
// when disabled.
#if !CORE_ASSERTIONS_ENABLED
TEST(CoreAssert, AssertDoesNotEvaluateWhenDisabled) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  g_side_effect = 0;
  ASSERT(SideEffectFalse());

  EXPECT_EQ(g_side_effect, 0);
  EXPECT_EQ(g_rec.calls, 0);
}

TEST(CoreAssert, AssertMsgDoesNotEvaluateWhenDisabled) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  g_side_effect = 0;
  ASSERT_MSG(SideEffectFalse(), "x");

  EXPECT_EQ(g_side_effect, 0);
  EXPECT_EQ(g_rec.calls, 0);
}
#endif // !CORE_ASSERTIONS_ENABLED

// 3) VERIFY always evaluates expression (both modes)
TEST(CoreAssert, VerifyEvaluatesTrueWithoutDispatch) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  g_side_effect = 0;
  VERIFY(SideEffectTrue());

  EXPECT_EQ(g_side_effect, 1);
  EXPECT_EQ(g_rec.calls, 0);
}

#if CORE_ASSERTIONS_ENABLED
TEST(CoreAssert, VerifyEvaluatesFalseAndDispatchesWhenEnabled) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  g_side_effect = 0;
  VERIFY(SideEffectFalse());

  EXPECT_EQ(g_side_effect, 1);
  EXPECT_EQ(g_rec.calls, 1);
}
#else
TEST(CoreAssert, VerifyEvaluatesFalseWithoutDispatchWhenDisabled) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  g_side_effect = 0;
  VERIFY(SideEffectFalse());

  EXPECT_EQ(g_side_effect, 1);
  EXPECT_EQ(g_rec.calls, 0);
}
#endif

// 4) Custom handler installation
TEST(CoreAssert, SetAssertHandlerReturnsPreviousAndNullRestoresDefault) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  core::AssertHandler old = core::SetAssertHandler(&RecordingHandler);
  ASSERT_NE(old, nullptr);

#if CORE_ASSERTIONS_ENABLED
  ASSERT(false);
  EXPECT_EQ(g_rec.calls, 1);
#endif

  core::SetAssertHandler(nullptr);
  EXPECT_NE(core::GetAssertHandler(), nullptr);

  // Restore recording handler for remaining checks in this test.
  core::SetAssertHandler(&RecordingHandler);
}

// 5) Optional diagnostics: must dispatch unconditionally (both modes).
TEST(CoreAssert, FatalDispatches) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  constexpr int expectedLine = __LINE__ + 1;
  FATAL("fatal");

  EXPECT_EQ(g_rec.calls, 1);
  EXPECT_EQ(std::string(g_rec.expr), std::string("FATAL"));
  ASSERT_NE(g_rec.msg, nullptr);
  EXPECT_EQ(std::string(g_rec.msg), std::string("fatal"));
  EXPECT_EQ(g_rec.line, expectedLine);
}

TEST(CoreAssert, UnreachableDispatches) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  UNREACHABLE();

  EXPECT_EQ(g_rec.calls, 1);
  EXPECT_EQ(std::string(g_rec.expr), std::string("UNREACHABLE"));
  EXPECT_EQ(g_rec.msg, nullptr);
}

TEST(CoreAssert, NotImplementedDispatches) {
  AssertHandlerGuard guard(&RecordingHandler);

  ResetRecord();
  NOT_IMPLEMENTED();

  EXPECT_EQ(g_rec.calls, 1);
  EXPECT_EQ(std::string(g_rec.expr), std::string("NOT_IMPLEMENTED"));
  EXPECT_EQ(g_rec.msg, nullptr);
}
