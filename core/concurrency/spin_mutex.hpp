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
#endif
};

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
#endif
}

} // namespace core

