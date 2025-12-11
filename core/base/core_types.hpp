#pragma once

namespace core
{
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

    namespace detail
    {
        template <unsigned PointerSize>
        struct pointer_sized_signed;

        template <>
        struct pointer_sized_signed<4>
        {
            using type = i32;
        };

        template <>
        struct pointer_sized_signed<8>
        {
            using type = i64;
        };

        template <unsigned PointerSize>
        struct pointer_sized_unsigned;

        template <>
        struct pointer_sized_unsigned<4>
        {
            using type = u32;
        };

        template <>
        struct pointer_sized_unsigned<8>
        {
            using type = u64;
        };
    } // namespace detail

    /// Unsigned integer large enough to hold a pointer.
    using usize = typename detail::pointer_sized_unsigned<sizeof(void*)>::type;

    /// Signed integer large enough to hold a pointer difference.
    using isize = typename detail::pointer_sized_signed<sizeof(void*)>::type;

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
    constexpr id32 kInvalidId32 = static_cast<id32>(0xFFFFFFFFu);
    constexpr id64 kInvalidId64 = static_cast<id64>(0xFFFFFFFFFFFFFFFFull);

    // Integer limits (assuming two's complement representation).

    // 8-bit.
    constexpr u8  kU8Max = static_cast<u8>(0xFFu);
    constexpr i8  kI8Min = static_cast<i8>(-0x80);
    constexpr i8  kI8Max = static_cast<i8>(0x7F);

    // 16-bit.
    constexpr u16 kU16Max = static_cast<u16>(0xFFFFu);
    constexpr i16 kI16Min = static_cast<i16>(-0x8000);
    constexpr i16 kI16Max = static_cast<i16>(0x7FFF);

    // 32-bit.
    constexpr u32 kU32Max = static_cast<u32>(0xFFFFFFFFu);
    constexpr i32 kI32Min = static_cast<i32>(-0x80000000);
    constexpr i32 kI32Max = static_cast<i32>(0x7FFFFFFF);

    // 64-bit.
    constexpr u64 kU64Max = static_cast<u64>(0xFFFFFFFFFFFFFFFFull);
    constexpr i64 kI64Min = static_cast<i64>(-0x8000000000000000ll);
    constexpr i64 kI64Max = static_cast<i64>(0x7FFFFFFFFFFFFFFFll);

}