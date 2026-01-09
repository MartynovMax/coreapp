#pragma once

// =============================================================================
// memory_ops.hpp
// Core-level wrappers for basic memory operations.
// =============================================================================

#include "core_memory.hpp"

namespace core {

// Ensure usize is suitable for pointer arithmetic
static_assert(sizeof(usize) == sizeof(void*), "usize must be pointer-sized for address arithmetic");

namespace detail {

CORE_FORCE_INLINE bool MemoryRangesOverlap(
    const void* dst,
    const void* src,
    memory_size size) noexcept
{
    if (size == 0 || dst == src) {
        return false;
    }

    const usize a = reinterpret_cast<usize>(dst);
    const usize b = reinterpret_cast<usize>(src);
    const usize n = static_cast<usize>(size);

    // Check for potential overflow in end address computation
    const usize max = static_cast<usize>(~usize(0));
    if (a > (max - n) || b > (max - n)) {
        CORE_MEM_ASSERT(false && "MemoryRangesOverlap: size too large, address overflow");
        return false;
    }

    const usize a_end = a + n;
    const usize b_end = b + n;

    return (a < b_end) && (b < a_end);
}

CORE_FORCE_INLINE void* ManualMemcpy(
    void* dst,
    const void* src,
    memory_size size) noexcept
{
    auto* d = static_cast<u8*>(dst);
    const auto* s = static_cast<const u8*>(src);

    for (memory_size i = 0; i < size; ++i) {
        d[i] = s[i];
    }

    return dst;
}

CORE_FORCE_INLINE void* ManualMemmove(
    void* dst,
    const void* src,
    memory_size size) noexcept
{
    if (dst == src || size == 0) {
        return dst;
    }

    auto* d = static_cast<u8*>(dst);
    const auto* s = static_cast<const u8*>(src);

    const usize a = reinterpret_cast<usize>(d);
    const usize b = reinterpret_cast<usize>(s);

    if (a < b) {
        // Forward copy is safe
        for (memory_size i = 0; i < size; ++i) {
            d[i] = s[i];
        }
    } else {
        // Backward copy to handle overlap
        for (memory_size i = size; i > 0; --i) {
            d[i - 1] = s[i - 1];
        }
    }

    return dst;
}

CORE_FORCE_INLINE void* ManualMemset(
    void* dst,
    int value,
    memory_size size) noexcept
{
    auto* d = static_cast<u8*>(dst);
    const u8 byte_value = static_cast<u8>(value);

    for (memory_size i = 0; i < size; ++i) {
        d[i] = byte_value;
    }

    return dst;
}

} // namespace detail

CORE_FORCE_INLINE void* memory_copy(
    void* dst,
    const void* src,
    memory_size size) noexcept
{
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT((dst != nullptr || size == 0) && "memory_copy: dst is null");
    CORE_MEM_ASSERT((src != nullptr || size == 0) && "memory_copy: src is null");

    if (size > 0 && dst != src) {
        CORE_MEM_ASSERT(!detail::MemoryRangesOverlap(dst, src, size) &&
                        "memory_copy: overlapping ranges (use memory_move)");
    }
#endif

    if (size == 0 || dst == src) {
        return dst;
    }

#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
    return __builtin_memcpy(dst, src, size);
#else
    return detail::ManualMemcpy(dst, src, size);
#endif
}

CORE_FORCE_INLINE void* memory_move(
    void* dst,
    const void* src,
    memory_size size) noexcept
{
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT((dst != nullptr || size == 0) && "memory_move: dst is null");
    CORE_MEM_ASSERT((src != nullptr || size == 0) && "memory_move: src is null");
#endif

    if (size == 0 || dst == src) {
        return dst;
    }

#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
    return __builtin_memmove(dst, src, size);
#else
    return detail::ManualMemmove(dst, src, size);
#endif
}

CORE_FORCE_INLINE void* memory_set(
    void* dst,
    int value,
    memory_size size) noexcept
{
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT((dst != nullptr || size == 0) && "memory_set: dst is null");
#endif

    if (size == 0) {
        return dst;
    }

#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
    return __builtin_memset(dst, value, size);
#else
    return detail::ManualMemset(dst, value, size);
#endif
}

CORE_FORCE_INLINE void* memory_zero(void* dst, memory_size size) noexcept
{
    return memory_set(dst, 0, size);
}

} // namespace core

