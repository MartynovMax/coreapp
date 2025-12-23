#include <gtest/gtest.h>

#include "core/base/core_defs.hpp"
#include "core/base/core_types.hpp"

using namespace core;

namespace {

// Compile-time checks (minimal, but ensures invariants and "no surprises").
static_assert(CORE_INVALID_ID32 == kInvalidId32, "CORE_INVALID_ID32 mismatch");
static_assert(CORE_INVALID_ID64 == kInvalidId64, "CORE_INVALID_ID64 mismatch");

static_assert(CORE_U8_MAX == kU8Max, "CORE_U8_MAX mismatch");
static_assert(CORE_U16_MAX == kU16Max, "CORE_U16_MAX mismatch");
static_assert(CORE_U32_MAX == kU32Max, "CORE_U32_MAX mismatch");
static_assert(CORE_U64_MAX == kU64Max, "CORE_U64_MAX mismatch");

static_assert(CORE_I8_MIN == kI8Min, "CORE_I8_MIN mismatch");
static_assert(CORE_I8_MAX == kI8Max, "CORE_I8_MAX mismatch");
static_assert(CORE_I16_MIN == kI16Min, "CORE_I16_MIN mismatch");
static_assert(CORE_I16_MAX == kI16Max, "CORE_I16_MAX mismatch");
static_assert(CORE_I32_MIN == kI32Min, "CORE_I32_MIN mismatch");
static_assert(CORE_I32_MAX == kI32Max, "CORE_I32_MAX mismatch");
static_assert(CORE_I64_MIN == kI64Min, "CORE_I64_MIN mismatch");
static_assert(CORE_I64_MAX == kI64Max, "CORE_I64_MAX mismatch");

static_assert(CORE_KILOBYTE == 1024u, "CORE_KILOBYTE must be 1024");
static_assert(CORE_MEGABYTE == 1024u * 1024u, "CORE_MEGABYTE mismatch");
static_assert(CORE_GIGABYTE == 1024ull * 1024ull * 1024ull,
              "CORE_GIGABYTE mismatch");

static_assert(CORE_INVALID_OFFSET == static_cast<offset_t>(-1),
              "CORE_INVALID_OFFSET mismatch");
static_assert(CORE_INVALID_TICK == static_cast<tick_t>(-1),
              "CORE_INVALID_TICK mismatch");

static_assert(CORE_FLAGS_NONE == 0u, "CORE_FLAGS_NONE must be 0");

} // namespace

TEST(CoreDefs, InvalidIds) {
  EXPECT_EQ(CORE_INVALID_ID32, kInvalidId32);
  EXPECT_EQ(CORE_INVALID_ID64, kInvalidId64);

  EXPECT_EQ(CORE_INVALID_ENTITY, kInvalidId32);
  EXPECT_EQ(CORE_INVALID_HANDLE, kInvalidId64);
}

TEST(CoreDefs, IndexNone) {
  EXPECT_EQ(CORE_INDEX_NONE, static_cast<isize>(-1));
}

TEST(CoreDefs, Limits) {
  EXPECT_EQ(CORE_U8_MAX, kU8Max);
  EXPECT_EQ(CORE_U16_MAX, kU16Max);
  EXPECT_EQ(CORE_U32_MAX, kU32Max);
  EXPECT_EQ(CORE_U64_MAX, kU64Max);

  EXPECT_EQ(CORE_I8_MIN, kI8Min);
  EXPECT_EQ(CORE_I8_MAX, kI8Max);
}

TEST(CoreDefs, Sizes) {
  EXPECT_EQ(CORE_KILOBYTE, static_cast<usize>(1024u));
  EXPECT_EQ(CORE_MEGABYTE, static_cast<usize>(1024u) * 1024u);
  EXPECT_EQ(CORE_GIGABYTE, static_cast<usize>(1024u) * 1024u * 1024u);
}

TEST(CoreDefs, VersionMacros_AreUsable) {
  // Sanity: these must be usable in preprocessor-driven code paths and logging.
  EXPECT_GE(CORE_VERSION_MAJOR, 0);
  EXPECT_GE(CORE_VERSION_MINOR, 0);
  EXPECT_GE(CORE_VERSION_PATCH, 0);

  // String macros must exist and be non-empty.
  EXPECT_NE(CORE_VERSION_STRING[0], '\0');
  EXPECT_NE(CORE_VERSION_STRING_FULL[0], '\0');
}