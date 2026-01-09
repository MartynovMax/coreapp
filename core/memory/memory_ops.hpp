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

} // namespace detail

} // namespace core

