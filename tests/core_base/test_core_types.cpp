// tests/core_base/test_core_types.cpp

#include <gtest/gtest.h>
#include <limits>
#include <type_traits>

#include "core/base/core_types.hpp"

using namespace core;

TEST(CoreTypes, Sizes_FixedWidth) {
  EXPECT_EQ(sizeof(i8), 1u);
  EXPECT_EQ(sizeof(u8), 1u);

  EXPECT_EQ(sizeof(i16), 2u);
  EXPECT_EQ(sizeof(u16), 2u);

  EXPECT_EQ(sizeof(i32), 4u);
  EXPECT_EQ(sizeof(u32), 4u);

  EXPECT_EQ(sizeof(i64), 8u);
  EXPECT_EQ(sizeof(u64), 8u);
}

TEST(CoreTypes, Sizes_PointerAndRelated) {
  EXPECT_EQ(sizeof(usize), sizeof(void *));
  EXPECT_EQ(sizeof(isize), sizeof(void *));

  EXPECT_EQ(sizeof(offset_t), sizeof(isize));
  EXPECT_EQ(sizeof(ptrdiff), sizeof(isize));
}

TEST(CoreTypes, Sizes_FloatAndTimeAndIds) {
  EXPECT_EQ(sizeof(f32), 4u);
  EXPECT_EQ(sizeof(f64), 8u);

  EXPECT_EQ(sizeof(byte), 1u);
  EXPECT_EQ(sizeof(b8), 1u);

  EXPECT_EQ(sizeof(id32), 4u);
  EXPECT_EQ(sizeof(id64), 8u);

  EXPECT_EQ(sizeof(tick_t), sizeof(u64));
  EXPECT_EQ(sizeof(time_ms), sizeof(i64));
  EXPECT_EQ(sizeof(time_us), sizeof(i64));
  EXPECT_EQ(sizeof(time_ns), sizeof(i64));
}

TEST(CoreTypes, Signedness) {
  EXPECT_TRUE(std::is_signed_v<i8>);
  EXPECT_TRUE(std::is_signed_v<i16>);
  EXPECT_TRUE(std::is_signed_v<i32>);
  EXPECT_TRUE(std::is_signed_v<i64>);
  EXPECT_TRUE(std::is_signed_v<isize>);
  EXPECT_TRUE(std::is_signed_v<offset_t>);
  EXPECT_TRUE(std::is_signed_v<ptrdiff>);
  EXPECT_TRUE(std::is_signed_v<time_ms>);
  EXPECT_TRUE(std::is_signed_v<time_us>);
  EXPECT_TRUE(std::is_signed_v<time_ns>);

  EXPECT_TRUE(std::is_unsigned_v<u8>);
  EXPECT_TRUE(std::is_unsigned_v<u16>);
  EXPECT_TRUE(std::is_unsigned_v<u32>);
  EXPECT_TRUE(std::is_unsigned_v<u64>);
  EXPECT_TRUE(std::is_unsigned_v<usize>);
  EXPECT_TRUE(std::is_unsigned_v<byte>);
  EXPECT_TRUE(std::is_unsigned_v<b8>);
  EXPECT_TRUE(std::is_unsigned_v<id32>);
  EXPECT_TRUE(std::is_unsigned_v<id64>);
  EXPECT_TRUE(std::is_unsigned_v<tick_t>);
}

TEST(CoreTypes, IntegralLimits) {
  EXPECT_EQ(kU8Max, std::numeric_limits<u8>::max());
  EXPECT_EQ(kU16Max, std::numeric_limits<u16>::max());
  EXPECT_EQ(kU32Max, std::numeric_limits<u32>::max());
  EXPECT_EQ(kU64Max, std::numeric_limits<u64>::max());

  EXPECT_EQ(kI8Min, std::numeric_limits<i8>::min());
  EXPECT_EQ(kI8Max, std::numeric_limits<i8>::max());
  EXPECT_EQ(kI16Min, std::numeric_limits<i16>::min());
  EXPECT_EQ(kI16Max, std::numeric_limits<i16>::max());
  EXPECT_EQ(kI32Min, std::numeric_limits<i32>::min());
  EXPECT_EQ(kI32Max, std::numeric_limits<i32>::max());
  EXPECT_EQ(kI64Min, std::numeric_limits<i64>::min());
  EXPECT_EQ(kI64Max, std::numeric_limits<i64>::max());
}

TEST(CoreTypes, InvalidIds) {
  EXPECT_NE(kInvalidId32, static_cast<id32>(0));
  EXPECT_NE(kInvalidId64, static_cast<id64>(0));

  id32 a = kInvalidId32;
  id64 b = kInvalidId64;

  EXPECT_EQ(a, kInvalidId32);
  EXPECT_EQ(b, kInvalidId64);
}
