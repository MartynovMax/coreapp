#pragma once

// =============================================================================
// atomic_ptr.hpp
// Atomic pointer type.
//
// Type-safe atomic pointer operations for void*.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_types.hpp"
#include "atomic_intrinsics.hpp"
#include "memory_order.hpp"

namespace core {

/// Atomic pointer for void*.
class atomic_ptr {
public:
    atomic_ptr() noexcept = default;
    
    explicit atomic_ptr(void* value) noexcept : m_value(value) {}

    atomic_ptr(const atomic_ptr&) = delete;
    atomic_ptr& operator=(const atomic_ptr&) = delete;
    atomic_ptr(atomic_ptr&&) = delete;
    atomic_ptr& operator=(atomic_ptr&&) = delete;

    CORE_NODISCARD void* load(memory_order order = memory_order::seq_cst) const noexcept {
        return detail::atomic_load_ptr(&m_value, order);
    }

    void store(void* value, memory_order order = memory_order::seq_cst) noexcept {
        detail::atomic_store_ptr(&m_value, value, order);
    }

    CORE_NODISCARD void* exchange(void* value, memory_order order = memory_order::seq_cst) noexcept {
        return detail::atomic_exchange_ptr(&m_value, value, order);
    }

    CORE_NODISCARD bool compare_exchange_weak(
        void*& expected,
        void* desired,
        memory_order success,
        memory_order failure) noexcept {
        return detail::atomic_compare_exchange_weak_ptr(
            &m_value, &expected, desired, success, failure);
    }

    CORE_NODISCARD bool compare_exchange_strong(
        void*& expected,
        void* desired,
        memory_order success,
        memory_order failure) noexcept {
        return detail::atomic_compare_exchange_strong_ptr(
            &m_value, &expected, desired, success, failure);
    }

    CORE_NODISCARD bool compare_exchange_weak(
        void*& expected,
        void* desired,
        memory_order order = memory_order::seq_cst) noexcept {
        return compare_exchange_weak(expected, desired, order, order);
    }

    CORE_NODISCARD bool compare_exchange_strong(
        void*& expected,
        void* desired,
        memory_order order = memory_order::seq_cst) noexcept {
        return compare_exchange_strong(expected, desired, order, order);
    }

private:
    alignas(sizeof(void*)) void* volatile m_value;
};

static_assert(sizeof(atomic_ptr) == sizeof(void*), "atomic_ptr must not have overhead");
static_assert(alignof(atomic_ptr) >= alignof(void*), "atomic_ptr must be properly aligned");

} // namespace core

