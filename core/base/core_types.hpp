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
}