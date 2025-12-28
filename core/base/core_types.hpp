#pragma once

//------------------------------------------------------------------------------
// core_types.hpp
// Fundamental Core-level primitive type definitions.
//------------------------------------------------------------------------------
//
// This header defines all low-level primitive types used across Core:
//  - Fixed-width integers (i8/u8, i16/u16, i32/u32, i64/u64)
//  - Pointer-sized integers (usize/isize)
//  - Floating-point types (f32/f64)
//  - Byte / boolean storage types (byte, b8)
//  - Identifier types (id32/id64)
//  - Time-related types (tick_t, time_ms/time_us/time_ns)
//  - Offset / pointer-diff types (offset_t, ptrdiff)
//  - Common constants (invalid IDs, integer limits)
//
// Constraints:
//  - No STL (no <cstdint>, <cstddef>, <limits>, etc.).
//  - No dependencies on other Core headers.
//  - Assumes 8/16/32/64-bit fundamental integer types and 32- or 64-bit
//  pointers.
//

namespace core {
//--------------------------------------------------------------------------
// Integer types
//--------------------------------------------------------------------------

/// Signed 8-bit integer.
using i8 = signed char;
/// Unsigned 8-bit integer.
using u8 = unsigned char;

/// Signed 16-bit integer.
using i16 = short;
/// Unsigned 16-bit integer.
using u16 = unsigned short;

/// Signed 32-bit integer.
using i32 = int;
/// Unsigned 32-bit integer.
using u32 = unsigned int;

/// Signed 64-bit integer.
using i64 = long long;
/// Unsigned 64-bit integer.
using u64 = unsigned long long;

//--------------------------------------------------------------------------
// Size / pointer-sized types
//--------------------------------------------------------------------------

namespace detail {
template <unsigned PointerSize> struct pointer_sized_signed;

template <> struct pointer_sized_signed<4> {
  using type = i32;
};

template <> struct pointer_sized_signed<8> {
  using type = i64;
};

template <unsigned PointerSize> struct pointer_sized_unsigned;

template <> struct pointer_sized_unsigned<4> {
  using type = u32;
};

template <> struct pointer_sized_unsigned<8> {
  using type = u64;
};
} // namespace detail

/// Unsigned integer large enough to hold a pointer.
using usize = detail::pointer_sized_unsigned<sizeof(void *)>::type;

/// Signed integer large enough to hold a pointer difference.
using isize = detail::pointer_sized_signed<sizeof(void *)>::type;

//--------------------------------------------------------------------------
// Floating-point types
//--------------------------------------------------------------------------

/// 32-bit floating-point.
using f32 = float;

/// 64-bit floating-point.
using f64 = double;

//--------------------------------------------------------------------------
// Byte / boolean storage types
//--------------------------------------------------------------------------

/// Raw byte (8-bit unsigned).
using byte = u8;

/// Boolean stored explicitly as 1 byte (0 = false, non-zero = true).
using b8 = u8;

//--------------------------------------------------------------------------
// Identifier types
//--------------------------------------------------------------------------

/// 32-bit identifier type.
using id32 = u32;

/// 64-bit identifier type.
using id64 = u64;

//--------------------------------------------------------------------------
// Time-related types
//--------------------------------------------------------------------------

/// Engine tick counter type (monotonic, wraps on overflow).
using tick_t = u64;

/// Time duration in milliseconds.
using time_ms = i64;

/// Time duration in microseconds.
using time_us = i64;

/// Time duration in nanoseconds.
using time_ns = i64;

//--------------------------------------------------------------------------
// Offset / pointer-diff types
//--------------------------------------------------------------------------

/// Generic offset type (typically used for byte offsets).
using offset_t = isize;

/// Pointer difference type.
using ptrdiff = isize;

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------

// Invalid identifier sentinel values.
constexpr id32 kInvalidId32 = 0xFFFFFFFFu;
constexpr id64 kInvalidId64 = 0xFFFFFFFFFFFFFFFFull;

// Integer limits (assuming two's complement representation).

// 8-bit.
constexpr u8 kU8Max = 0xFFu;
constexpr i8 kI8Min = -0x80;
constexpr i8 kI8Max = 0x7F;

// 16-bit.
constexpr u16 kU16Max = 0xFFFFu;
constexpr i16 kI16Min = -0x8000;
constexpr i16 kI16Max = 0x7FFF;

// 32-bit.
constexpr u32 kU32Max = 0xFFFFFFFFu;
constexpr i32 kI32Max = 0x7FFFFFFF;
constexpr i32 kI32Min = -kI32Max - 1;

// 64-bit.
constexpr u64 kU64Max = 0xFFFFFFFFFFFFFFFFull;
constexpr i64 kI64Max = 0x7FFFFFFFFFFFFFFFll;
constexpr i64 kI64Min = -kI64Max - 1;

//--------------------------------------------------------------------------
// Static asserts
//--------------------------------------------------------------------------

// Fixed-width integer sizes.
static_assert(sizeof(i8) == 1, "core::i8 must be 8 bits");
static_assert(sizeof(u8) == 1, "core::u8 must be 8 bits");

static_assert(sizeof(i16) == 2, "core::i16 must be 16 bits");
static_assert(sizeof(u16) == 2, "core::u16 must be 16 bits");

static_assert(sizeof(i32) == 4, "core::i32 must be 32 bits");
static_assert(sizeof(u32) == 4, "core::u32 must be 32 bits");

static_assert(sizeof(i64) == 8, "core::i64 must be 64 bits");
static_assert(sizeof(u64) == 8, "core::u64 must be 64 bits");

// Pointer size assumptions (32-bit or 64-bit only).
static_assert(sizeof(void *) == 4 || sizeof(void *) == 8,
              "core only supports 32-bit and 64-bit pointer sizes");

// Pointer-sized integer sizes.
static_assert(sizeof(usize) == sizeof(void *),
              "core::usize must match pointer size");
static_assert(sizeof(isize) == sizeof(void *),
              "core::isize must match pointer size");

// Floating-point sizes.
static_assert(sizeof(f32) == 4, "core::f32 must be 32-bit float");
static_assert(sizeof(f64) == 8, "core::f64 must be 64-bit float");

// Byte / boolean sizes.
static_assert(sizeof(byte) == 1, "core::byte must be 8 bits");
static_assert(sizeof(b8) == 1, "core::b8 must be 8 bits");

// ID sizes.
static_assert(sizeof(id32) == sizeof(u32), "core::id32 must be 32 bits");
static_assert(sizeof(id64) == sizeof(u64), "core::id64 must be 64 bits");

// Time type sizes.
static_assert(sizeof(tick_t) == sizeof(u64), "core::tick_t must be 64-bit");
static_assert(sizeof(time_ms) == sizeof(i64), "core::time_ms must be 64-bit");
static_assert(sizeof(time_us) == sizeof(i64), "core::time_us must be 64-bit");
static_assert(sizeof(time_ns) == sizeof(i64), "core::time_ns must be 64-bit");

// Offset / ptrdiff sizes.
static_assert(sizeof(offset_t) == sizeof(isize),
              "core::offset_t must match isize");
static_assert(sizeof(ptrdiff) == sizeof(isize),
              "core::ptrdiff must match isize");
} // namespace core