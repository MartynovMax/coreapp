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

// ----------------------------------------------------------------------------
// Type-based memory traits
// ----------------------------------------------------------------------------

namespace detail {

#if CORE_COMPILER_MSVC || CORE_COMPILER_CLANG || CORE_COMPILER_GCC
    #define CORE_DETAIL_HAS_INTRINSIC_TRAITS 1
#else
    #define CORE_DETAIL_HAS_INTRINSIC_TRAITS 0
#endif

template <class T>
struct IsTriviallyCopyableImpl {
#if CORE_DETAIL_HAS_INTRINSIC_TRAITS
    static constexpr bool value = __is_trivially_copyable(T);
#else
    static constexpr bool value = false;
#endif
};

} // namespace detail

template <class T>
struct is_trivially_copyable {
    static constexpr bool value = detail::IsTriviallyCopyableImpl<T>::value;
};
template <class T>
inline constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;

template <class T>
struct is_trivially_relocatable {
    static constexpr bool value = is_trivially_copyable_v<T>;
};
template <class T>
inline constexpr bool is_trivially_relocatable_v = is_trivially_relocatable<T>::value;

template <class T>
struct is_bitwise_serializable {
    static constexpr bool value = is_trivially_copyable_v<T>;
};
template <class T>
inline constexpr bool is_bitwise_serializable_v = is_bitwise_serializable<T>::value;

template <class T>
struct max_align_for {
    static constexpr usize value = static_cast<usize>(alignof(T));
};
template <class T>
inline constexpr usize max_align_for_v = max_align_for<T>::value;

// ----------------------------------------------------------------------------
// Size utilities
// ----------------------------------------------------------------------------

CORE_FORCE_INLINE constexpr usize UsizeMax() noexcept {
    return static_cast<usize>(~static_cast<usize>(0));
}

CORE_FORCE_INLINE constexpr usize SafeMulSize(usize a, usize b) noexcept {
#if CORE_MEMORY_DEBUG
    if (a != 0 && b != 0) {
        CORE_MEM_ASSERT(b <= (UsizeMax() / a) && "Size multiplication overflow");
    }
#endif
    return a * b;
}

template <class T>
CORE_FORCE_INLINE constexpr usize BytesFor(usize count) noexcept {
    return SafeMulSize(static_cast<usize>(sizeof(T)), count);
}

template <class T>
CORE_FORCE_INLINE constexpr usize StorageSizeFor(usize count) noexcept {
    return static_cast<usize>(sizeof(T)) * count;
}

// ----------------------------------------------------------------------------
// Integration helpers for Core config
// ----------------------------------------------------------------------------

CORE_FORCE_INLINE constexpr usize DefaultAlignment() noexcept {
#ifdef CORE_DEFAULT_ALIGNMENT
    return static_cast<usize>(CORE_DEFAULT_ALIGNMENT);
#else
    return static_cast<usize>(alignof(f64));
#endif
}

CORE_FORCE_INLINE constexpr usize CacheLineSize() noexcept {
#ifdef CORE_CACHE_LINE_SIZE
    return static_cast<usize>(CORE_CACHE_LINE_SIZE);
#else
    return static_cast<usize>(64);
#endif
}

} // namespace core

