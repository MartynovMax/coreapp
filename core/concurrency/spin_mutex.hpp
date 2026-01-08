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

} // namespace core

