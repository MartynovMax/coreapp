#pragma once

// =============================================================================
// memory_ops.hpp
// Core-level wrappers for basic memory operations.
// =============================================================================

#include "core_memory.hpp"

namespace core {

namespace detail {

CORE_FORCE_INLINE bool MemoryRangesOverlap(
    const void* dst,
    const void* src,
    memory_size size) noexcept
{
    if (size == 0 || dst == src) {
        return false;
    }

    const auto* dst_bytes = static_cast<const u8*>(dst);
    const auto* src_bytes = static_cast<const u8*>(src);
    const auto* dst_end = dst_bytes + size;
    const auto* src_end = src_bytes + size;

    return (dst_bytes < src_end) && (src_bytes < dst_end);
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
    auto* d = static_cast<u8*>(dst);
    const auto* s = static_cast<const u8*>(src);

    if (d < s) {
        for (memory_size i = 0; i < size; ++i) {
            d[i] = s[i];
        }
    } else if (d > s) {
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

    if (size == 0) {
        return dst;
    }

#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
    return __builtin_memcpy(dst, src, size);
#elif CORE_COMPILER_MSVC
    #if _MSC_VER >= 1928
        return __builtin_memcpy(dst, src, size);
    #else
        return detail::ManualMemcpy(dst, src, size);
    #endif
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

    if (size == 0) {
        return dst;
    }

#if CORE_COMPILER_CLANG || CORE_COMPILER_GCC
    return __builtin_memmove(dst, src, size);
#elif CORE_COMPILER_MSVC
    #if _MSC_VER >= 1928
        return __builtin_memmove(dst, src, size);
    #else
        return detail::ManualMemmove(dst, src, size);
    #endif
#else
    return detail::ManualMemmove(dst, src, size);
#endif
}

} // namespace core

