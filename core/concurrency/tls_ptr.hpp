#pragma once

// =============================================================================
// tls_ptr.hpp
// Thread-local storage primitives.
//
// Provides minimal TLS abstraction without STL dependencies.
// Degrades to single-threaded storage when threading is unavailable.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_features.hpp"

namespace core {

// =============================================================================
// tls_ptr<T> - Thread-local pointer storage
// =============================================================================

template<typename T>
class tls_ptr {
public:
    tls_ptr() = delete;
    tls_ptr(const tls_ptr&) = delete;
    tls_ptr& operator=(const tls_ptr&) = delete;

    /// Get current thread's pointer value.
    CORE_NODISCARD static T* get() noexcept;

    /// Set current thread's pointer value.
    static void set(T* ptr) noexcept;

private:
#if CORE_HAS_THREADS && CORE_HAS_THREAD_LOCAL
    static thread_local T* s_value;
#else
    static T* s_value;
#endif
};

// Static member definition
template<typename T>
#if CORE_HAS_THREADS && CORE_HAS_THREAD_LOCAL
thread_local T* tls_ptr<T>::s_value = nullptr;
#else
T* tls_ptr<T>::s_value = nullptr;
#endif

} // namespace core

