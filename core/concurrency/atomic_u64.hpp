#pragma once

// =============================================================================
// atomic_u64.hpp
// 64-bit unsigned atomic integer type.
//
// Only available on platforms with 64-bit atomic support
// (CORE_HAS_64BIT_ATOMICS == 1).
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_features.hpp"
#include "../base/core_types.hpp"
#include "atomic_intrinsics.hpp"
#include "memory_order.hpp"

#if CORE_HAS_64BIT_ATOMICS

namespace core {

/// 64-bit unsigned atomic integer.
class atomic_u64 {
public:
    atomic_u64() noexcept = default;
    
    explicit atomic_u64(u64 value) noexcept : m_value(static_cast<decltype(m_value)>(value)) {}

    atomic_u64(const atomic_u64&) = delete;
    atomic_u64& operator=(const atomic_u64&) = delete;
    atomic_u64(atomic_u64&&) = delete;
    atomic_u64& operator=(atomic_u64&&) = delete;

    CORE_NODISCARD u64 load(memory_order order = memory_order::seq_cst) const noexcept {
        return detail::atomic_load_u64(&m_value, order);
    }

    void store(u64 value, memory_order order = memory_order::seq_cst) noexcept {
        detail::atomic_store_u64(&m_value, value, order);
    }

    CORE_NODISCARD u64 exchange(u64 value, memory_order order = memory_order::seq_cst) noexcept {
        return detail::atomic_exchange_u64(&m_value, value, order);
    }

    CORE_NODISCARD bool compare_exchange_weak(
        u64& expected,
        u64 desired,
        memory_order success,
        memory_order failure) noexcept {
        return detail::atomic_compare_exchange_weak_u64(
            &m_value, &expected, desired, success, failure);
    }

    CORE_NODISCARD bool compare_exchange_strong(
        u64& expected,
        u64 desired,
        memory_order success,
        memory_order failure) noexcept {
        return detail::atomic_compare_exchange_strong_u64(
            &m_value, &expected, desired, success, failure);
    }

    CORE_NODISCARD bool compare_exchange_weak(
        u64& expected,
        u64 desired,
        memory_order order = memory_order::seq_cst) noexcept {
        return compare_exchange_weak(expected, desired, order, order);
    }

    CORE_NODISCARD bool compare_exchange_strong(
        u64& expected,
        u64 desired,
        memory_order order = memory_order::seq_cst) noexcept {
        return compare_exchange_strong(expected, desired, order, order);
    }

    CORE_NODISCARD u64 fetch_add(u64 value, memory_order order = memory_order::seq_cst) noexcept {
        return detail::atomic_fetch_add_u64(&m_value, value, order);
    }

    CORE_NODISCARD u64 fetch_sub(u64 value, memory_order order = memory_order::seq_cst) noexcept {
        return detail::atomic_fetch_sub_u64(&m_value, value, order);
    }

private:
    alignas(8) volatile detail::atomic_u64_storage_t m_value;
};

static_assert(sizeof(atomic_u64) == sizeof(u64), "atomic_u64 must not have overhead");
static_assert(alignof(atomic_u64) >= alignof(u64), "atomic_u64 must be properly aligned");

} // namespace core

#endif  // CORE_HAS_64BIT_ATOMICS

