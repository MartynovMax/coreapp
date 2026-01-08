#pragma once

// =============================================================================
// spin_mutex.hpp
// Lightweight spin-based mutual exclusion primitive.
//
// Uses atomic operations to implement a busy-wait lock suitable for protecting
// short critical sections. Does not block at the OS level.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_features.hpp"
#include "../base/core_types.hpp"
#include "../base/core_assert.hpp"
#include "core_atomic.hpp"
#include "cpu_relax.hpp"

namespace core {

/// Spin-based mutex for protecting short critical sections.
class spin_mutex {
public:
    spin_mutex() noexcept = default;
    ~spin_mutex() = default;

    spin_mutex(const spin_mutex&) = delete;
    spin_mutex& operator=(const spin_mutex&) = delete;
    spin_mutex(spin_mutex&&) = delete;
    spin_mutex& operator=(spin_mutex&&) = delete;

    void lock() noexcept;
    bool try_lock() noexcept;
    void unlock() noexcept;

private:
#if CORE_HAS_THREADS
    atomic_u32 m_flag{0};  // 0 = unlocked, 1 = locked

    #if CORE_DEBUG
    atomic_u32 m_lock_count{0};
    #endif
#else
    bool m_locked{false};  // Single-thread verification flag
#endif
};

#if CORE_HAS_THREADS
    #if CORE_DEBUG
    static_assert(sizeof(spin_mutex) <= 2 * sizeof(u32), "spin_mutex debug size too large");
    #else
    static_assert(sizeof(spin_mutex) == sizeof(u32), "spin_mutex must have minimal overhead");
    #endif
#else
static_assert(sizeof(spin_mutex) == sizeof(bool), "spin_mutex single-thread size must be one bool");
#endif

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------

inline void spin_mutex::lock() noexcept {
#if CORE_HAS_THREADS
    #if CORE_DEBUG
    u32 prev_count = m_lock_count.fetch_add(1, memory_order::relaxed);
    CORE_ASSERT(prev_count == 0, "Double lock detected");
    #endif

    u32 spin_count = 0;
    constexpr u32 MAX_SPIN = 32;

    while (m_flag.exchange(1, memory_order::acquire) != 0) {
        spin_count = 0;
        while (m_flag.load(memory_order::relaxed) != 0) {
            if (spin_count < MAX_SPIN) {
                for (u32 i = 0; i < (1u << spin_count); ++i) {
                    cpu_relax();
                }
                ++spin_count;
            } else {
                cpu_relax();
            }
        }
    }
#else
    CORE_ASSERT(!m_locked, "Double lock detected");
    m_locked = true;
#endif
}

inline bool spin_mutex::try_lock() noexcept {
#if CORE_HAS_THREADS
    bool acquired = m_flag.exchange(1, memory_order::acquire) == 0;
    
    #if CORE_DEBUG
    if (acquired) {
        u32 prev_count = m_lock_count.fetch_add(1, memory_order::relaxed);
        CORE_ASSERT(prev_count == 0, "Double lock detected");
    }
    #endif
    
    return acquired;
#else
    if (!m_locked) {
        m_locked = true;
        return true;
    }
    return false;
#endif
}

inline void spin_mutex::unlock() noexcept {
#if CORE_HAS_THREADS
    #if CORE_DEBUG
    u32 prev_count = m_lock_count.fetch_sub(1, memory_order::relaxed);
    CORE_ASSERT(prev_count == 1, "Unlock without lock");
    #endif
    
    m_flag.store(0, memory_order::release);
#else
    CORE_ASSERT(m_locked, "Unlock without lock");
    m_locked = false;
#endif
}

} // namespace core

