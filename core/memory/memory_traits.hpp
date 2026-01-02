#pragma once

// =============================================================================
// memory_traits.hpp
// Low-level constexpr helpers for alignment, pointer arithmetic, size math,
// and memory-relevant type traits.
//
// This header is intended to be:
//   - independent from allocator interfaces,
//   - independent from STL (<type_traits>, <cstdint>, <cstddef>),
//   - cheap to include.
//
// It provides a single source of truth for "memory math" across Core.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_assert.hpp"
#include "../base/core_types.hpp"
#include "../base/core_inline.hpp"

namespace core {

// ----------------------------------------------------------------------------
// Alignment helpers
// ----------------------------------------------------------------------------

CORE_FORCE_INLINE constexpr bool IsPowerOfTwo(usize a) noexcept {
    return a != 0 && ((a & (a - 1)) == 0);
}

CORE_FORCE_INLINE constexpr usize AlignUp(usize value, usize alignment) noexcept {
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(IsPowerOfTwo(alignment));
#endif
    const usize mask = alignment - 1;
    return (value + mask) & ~mask;
}

CORE_FORCE_INLINE constexpr usize AlignDown(usize value, usize alignment) noexcept {
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(IsPowerOfTwo(alignment));
#endif
    const usize mask = alignment - 1;
    return value & ~mask;
}

CORE_FORCE_INLINE constexpr bool IsAlignedValue(usize value, usize alignment) noexcept {
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(IsPowerOfTwo(alignment));
#endif
    const usize mask = alignment - 1;
    return (value & mask) == 0;
}

CORE_FORCE_INLINE constexpr usize PaddingFor(usize value, usize alignment) noexcept {
    const usize aligned = AlignUp(value, alignment);
    return aligned - value;
}

template <class T>
CORE_FORCE_INLINE bool IsAlignedPtr(const T* p, usize alignment) noexcept {
    return IsAlignedValue(reinterpret_cast<usize>(p), alignment);
}

template <class T>
CORE_FORCE_INLINE T* AlignPtrUp(T* p, usize alignment) noexcept {
    const usize v = reinterpret_cast<usize>(p);
    const usize av = AlignUp(v, alignment);
    return reinterpret_cast<T*>(av);
}

template <class T>
CORE_FORCE_INLINE const T* AlignPtrUp(const T* p, usize alignment) noexcept {
    const usize v = reinterpret_cast<usize>(p);
    const usize av = AlignUp(v, alignment);
    return reinterpret_cast<const T*>(av);
}

// ----------------------------------------------------------------------------
// Pointer arithmetic helpers
// ----------------------------------------------------------------------------

template <class T>
CORE_FORCE_INLINE T* AddBytes(T* p, usize byte_offset) noexcept {
    return reinterpret_cast<T*>(reinterpret_cast<usize>(p) + byte_offset);
}

template <class T>
CORE_FORCE_INLINE const T* AddBytes(const T* p, usize byte_offset) noexcept {
    return reinterpret_cast<const T*>(reinterpret_cast<usize>(p) + byte_offset);
}

template <class T>
CORE_FORCE_INLINE T* SubBytes(T* p, usize byte_offset) noexcept {
    return reinterpret_cast<T*>(reinterpret_cast<usize>(p) - byte_offset);
}

template <class T>
CORE_FORCE_INLINE const T* SubBytes(const T* p, usize byte_offset) noexcept {
    return reinterpret_cast<const T*>(reinterpret_cast<usize>(p) - byte_offset);
}

CORE_FORCE_INLINE usize PtrDiffBytes(const void* a, const void* b) noexcept {
    return static_cast<usize>(reinterpret_cast<usize>(a) - reinterpret_cast<usize>(b));
}

} // namespace core

