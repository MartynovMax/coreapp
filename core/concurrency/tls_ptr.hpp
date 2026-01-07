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

// Method implementations
template<typename T>
T* tls_ptr<T>::get() noexcept {
    return s_value;
}

template<typename T>
void tls_ptr<T>::set(T* ptr) noexcept {
    s_value = ptr;
}

// =============================================================================
// tls_value<T> - Thread-local scalar storage
// =============================================================================

template<typename T>
class tls_value {
public:
    tls_value() = delete;
    tls_value(const tls_value&) = delete;
    tls_value& operator=(const tls_value&) = delete;

    /// Get current thread's value.
    CORE_NODISCARD static T get() noexcept;

    /// Set current thread's value.
    static void set(T value) noexcept;

private:
#if CORE_HAS_THREADS && CORE_HAS_THREAD_LOCAL
    static thread_local T s_value;
#else
    static T s_value;
#endif
};

// Static member definition with zero-initialization
template<typename T>
#if CORE_HAS_THREADS && CORE_HAS_THREAD_LOCAL
thread_local T tls_value<T>::s_value = T{};
#else
T tls_value<T>::s_value = T{};
#endif

} // namespace core

