#pragma once

// =============================================================================
// atomic_ptr.hpp
// Atomic pointer type.
//
// Type-safe atomic pointer operations for void*.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_platform.hpp"
#include "../base/core_types.hpp"
#include "atomic_uintptr.hpp"
#include "memory_order.hpp"

namespace core {

/// Atomic pointer for void*.
/// Stores pointer as usize to avoid aliasing issues with interlocked operations.
class atomic_ptr {
public:
    atomic_ptr() noexcept : m_storage(static_cast<usize>(0)) {}
    
    explicit atomic_ptr(void* value) noexcept 
        : m_storage(reinterpret_cast<usize>(value)) {}

    atomic_ptr(const atomic_ptr&) = delete;
    atomic_ptr& operator=(const atomic_ptr&) = delete;
    atomic_ptr(atomic_ptr&&) = delete;
    atomic_ptr& operator=(atomic_ptr&&) = delete;

    CORE_NODISCARD void* load(memory_order order = memory_order::seq_cst) const noexcept {
        usize bits = m_storage.load(order);
        return reinterpret_cast<void*>(bits);
    }

    void store(void* value, memory_order order = memory_order::seq_cst) noexcept {
        usize bits = reinterpret_cast<usize>(value);
        m_storage.store(bits, order);
    }

    CORE_NODISCARD void* exchange(void* value, memory_order order = memory_order::seq_cst) noexcept {
        usize bits = reinterpret_cast<usize>(value);
        usize old_bits = m_storage.exchange(bits, order);
        return reinterpret_cast<void*>(old_bits);
    }

    CORE_NODISCARD bool compare_exchange_weak(
        void*& expected,
        void* desired,
        memory_order success,
        memory_order failure) noexcept {
        usize expected_bits = reinterpret_cast<usize>(expected);
        usize desired_bits = reinterpret_cast<usize>(desired);
        bool result = m_storage.compare_exchange_weak(expected_bits, desired_bits, success, failure);
        expected = reinterpret_cast<void*>(expected_bits);
        return result;
    }

    CORE_NODISCARD bool compare_exchange_strong(
        void*& expected,
        void* desired,
        memory_order success,
        memory_order failure) noexcept {
        usize expected_bits = reinterpret_cast<usize>(expected);
        usize desired_bits = reinterpret_cast<usize>(desired);
        bool result = m_storage.compare_exchange_strong(expected_bits, desired_bits, success, failure);
        expected = reinterpret_cast<void*>(expected_bits);
        return result;
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
    atomic_uintptr m_storage;
};

static_assert(sizeof(atomic_ptr) == sizeof(void*), "atomic_ptr must not have overhead");
static_assert(alignof(atomic_ptr) >= alignof(void*), "atomic_ptr must be properly aligned");

} // namespace core

