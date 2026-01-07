#pragma once

// =============================================================================
// atomic_u32.hpp
// 32-bit unsigned atomic integer type.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_types.hpp"
#include "atomic_intrinsics.hpp"
#include "memory_order.hpp"

namespace core {

/// 32-bit unsigned atomic integer.
class atomic_u32 {
public:
    atomic_u32() noexcept = default;
    
    explicit atomic_u32(u32 value) noexcept : m_value(value) {}

    atomic_u32(const atomic_u32&) = delete;
    atomic_u32& operator=(const atomic_u32&) = delete;
    atomic_u32(atomic_u32&&) = delete;
    atomic_u32& operator=(atomic_u32&&) = delete;

    CORE_NODISCARD u32 load(memory_order order = memory_order::seq_cst) const noexcept {
        return detail::atomic_load_u32(&m_value, order);
    }

    void store(u32 value, memory_order order = memory_order::seq_cst) noexcept {
        detail::atomic_store_u32(&m_value, value, order);
    }

    CORE_NODISCARD u32 exchange(u32 value, memory_order order = memory_order::seq_cst) noexcept {
        return detail::atomic_exchange_u32(&m_value, value, order);
    }

    CORE_NODISCARD bool compare_exchange_weak(
        u32& expected,
        u32 desired,
        memory_order success,
        memory_order failure) noexcept {
        return detail::atomic_compare_exchange_weak_u32(
            &m_value, &expected, desired, success, failure);
    }

    CORE_NODISCARD bool compare_exchange_strong(
        u32& expected,
        u32 desired,
        memory_order success,
        memory_order failure) noexcept {
        return detail::atomic_compare_exchange_strong_u32(
            &m_value, &expected, desired, success, failure);
    }

    CORE_NODISCARD bool compare_exchange_weak(
        u32& expected,
        u32 desired,
        memory_order order = memory_order::seq_cst) noexcept {
        return compare_exchange_weak(expected, desired, order, order);
    }

    CORE_NODISCARD bool compare_exchange_strong(
        u32& expected,
        u32 desired,
        memory_order order = memory_order::seq_cst) noexcept {
        return compare_exchange_strong(expected, desired, order, order);
    }

    CORE_NODISCARD u32 fetch_add(u32 value, memory_order order = memory_order::seq_cst) noexcept {
        return detail::atomic_fetch_add_u32(&m_value, value, order);
    }

    CORE_NODISCARD u32 fetch_sub(u32 value, memory_order order = memory_order::seq_cst) noexcept {
        return detail::atomic_fetch_sub_u32(&m_value, value, order);
    }

private:
    alignas(4) volatile u32 m_value;
};

static_assert(sizeof(atomic_u32) == sizeof(u32), "atomic_u32 must not have overhead");
static_assert(alignof(atomic_u32) >= alignof(u32), "atomic_u32 must be properly aligned");

} // namespace core

