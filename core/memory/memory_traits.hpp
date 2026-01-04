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
    return reinterpret_cast<T*>(reinterpret_cast<char*>(p) + byte_offset);
}

template <class T>
CORE_FORCE_INLINE const T* AddBytes(const T* p, usize byte_offset) noexcept {
    return reinterpret_cast<const T*>(reinterpret_cast<const char*>(p) + byte_offset);
}

template <class T>
CORE_FORCE_INLINE T* SubBytes(T* p, usize byte_offset) noexcept {
    return reinterpret_cast<T*>(reinterpret_cast<char*>(p) - byte_offset);
}

template <class T>
CORE_FORCE_INLINE const T* SubBytes(const T* p, usize byte_offset) noexcept {
    return reinterpret_cast<const T*>(reinterpret_cast<const char*>(p) - byte_offset);
}

CORE_FORCE_INLINE usize PtrDiffBytes(const void* a, const void* b) noexcept {
    return static_cast<usize>(reinterpret_cast<const char*>(a) - reinterpret_cast<const char*>(b));
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
constexpr bool is_trivially_copyable_v() noexcept {
    return is_trivially_copyable<T>::value;
}

template <class T>
struct is_trivially_relocatable {
    static constexpr bool value = is_trivially_copyable_v<T>();
};
template <class T>
constexpr bool is_trivially_relocatable_v() noexcept {
    return is_trivially_relocatable<T>::value;
}

template <class T>
struct is_bitwise_serializable {
    static constexpr bool value = is_trivially_copyable_v<T>();
};
template <class T>
constexpr bool is_bitwise_serializable_v() noexcept {
    return is_bitwise_serializable<T>::value;
}

template <class T>
struct max_align_for {
    static constexpr usize value = static_cast<usize>(alignof(T));
};
template <class T>
constexpr usize max_align_for_v() noexcept {
    return max_align_for<T>::value;
}

// ----------------------------------------------------------------------------
// Size utilities
// ----------------------------------------------------------------------------

CORE_FORCE_INLINE constexpr usize UsizeMax() noexcept {
    return static_cast<usize>(~static_cast<usize>(0));
}

CORE_FORCE_INLINE constexpr usize SafeMulSize(usize a, usize b) noexcept {
    if (a == 0 || b == 0) {
        return 0;
    }
    const usize max_val = UsizeMax();
    if (b > (max_val / a)) {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(false && "Size multiplication overflow");
#endif
        return max_val;
    }
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

CORE_FORCE_INLINE constexpr usize CacheAlignUp(usize value) noexcept {
    return AlignUp(value, CacheLineSize());
}

CORE_FORCE_INLINE constexpr usize CacheAlignDown(usize value) noexcept {
    return AlignDown(value, CacheLineSize());
}

// ----------------------------------------------------------------------------
// Static assertions and validation
// ----------------------------------------------------------------------------

namespace detail {

static_assert(IsPowerOfTwo(1), "1 is power of two");
static_assert(IsPowerOfTwo(2), "2 is power of two");
static_assert(IsPowerOfTwo(16), "16 is power of two");
static_assert(!IsPowerOfTwo(0), "0 is not power of two");
static_assert(!IsPowerOfTwo(3), "3 is not power of two");

static_assert(AlignUp(0, 16) == 0, "AlignUp(0, 16) == 0");
static_assert(AlignUp(1, 16) == 16, "AlignUp(1, 16) == 16");
static_assert(AlignUp(16, 16) == 16, "AlignUp(16, 16) == 16");
static_assert(AlignUp(17, 16) == 32, "AlignUp(17, 16) == 32");

static_assert(AlignDown(0, 16) == 0, "AlignDown(0, 16) == 0");
static_assert(AlignDown(15, 16) == 0, "AlignDown(15, 16) == 0");
static_assert(AlignDown(16, 16) == 16, "AlignDown(16, 16) == 16");
static_assert(AlignDown(31, 16) == 16, "AlignDown(31, 16) == 16");

static_assert(IsAlignedValue(0, 16), "0 is aligned to 16");
static_assert(IsAlignedValue(16, 16), "16 is aligned to 16");
static_assert(!IsAlignedValue(17, 16), "17 is not aligned to 16");

static_assert(PaddingFor(0, 16) == 0, "PaddingFor(0, 16) == 0");
static_assert(PaddingFor(1, 16) == 15, "PaddingFor(1, 16) == 15");
static_assert(PaddingFor(16, 16) == 0, "PaddingFor(16, 16) == 0");
static_assert(PaddingFor(17, 16) == 15, "PaddingFor(17, 16) == 15");

static_assert(UsizeMax() > 0, "UsizeMax must be positive");
static_assert(SafeMulSize(0, 100) == 0, "SafeMulSize(0, 100) == 0");
static_assert(SafeMulSize(10, 20) == 200, "SafeMulSize(10, 20) == 200");

static_assert(BytesFor<u32>(0) == 0, "BytesFor<u32>(0) == 0");
static_assert(BytesFor<u32>(1) == 4, "BytesFor<u32>(1) == 4");
static_assert(BytesFor<u32>(10) == 40, "BytesFor<u32>(10) == 40");

static_assert(StorageSizeFor<u64>(5) == 40, "StorageSizeFor<u64>(5) == 40");

static_assert(DefaultAlignment() > 0, "DefaultAlignment must be positive");
static_assert(IsPowerOfTwo(DefaultAlignment()), "DefaultAlignment must be power of two");
static_assert(CacheLineSize() > 0, "CacheLineSize must be positive");
static_assert(IsPowerOfTwo(CacheLineSize()), "CacheLineSize must be power of two");

struct TrivialStruct { int x; float y; };
static_assert(is_trivially_copyable_v<int>(), "int is trivially copyable");
static_assert(is_trivially_copyable_v<TrivialStruct>(), "TrivialStruct is trivially copyable");
static_assert(is_trivially_relocatable_v<int>(), "int is trivially relocatable");
static_assert(max_align_for_v<int>() == alignof(int), "max_align_for<int> matches alignof");
static_assert(max_align_for_v<f64>() == alignof(f64), "max_align_for<f64> matches alignof");

} // namespace detail 

} // namespace core

