#pragma once

// =============================================================================
// scoped_lock.hpp
// RAII guard for mutex-like types.
//
// Provides automatic lock acquisition on construction and release on
// destruction, ensuring exception-safe mutex usage.
// =============================================================================

namespace core {
template<typename Mutex>
class scoped_lock {
public:
    explicit scoped_lock(Mutex& mutex) noexcept
        : m_mutex(mutex)
    {
        m_mutex.lock();
    }

    ~scoped_lock() noexcept {
        m_mutex.unlock();
    }

    scoped_lock(const scoped_lock&) = delete;
    scoped_lock& operator=(const scoped_lock&) = delete;
    scoped_lock(scoped_lock&&) = delete;
    scoped_lock& operator=(scoped_lock&&) = delete;

private:
    Mutex& m_mutex;
};

} // namespace core

