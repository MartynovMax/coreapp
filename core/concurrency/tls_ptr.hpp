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

// Helper macro for TLS specifier
#if CORE_HAS_THREADS && CORE_HAS_THREAD_LOCAL
    #define CORE_TLS_SPEC thread_local
#else
    #define CORE_TLS_SPEC
#endif

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
    CORE_NODISCARD static T* get() noexcept {
        return s_value;
    }

    /// Set current thread's pointer value.
    static void set(T* ptr) noexcept {
        s_value = ptr;
    }

private:
    inline static CORE_TLS_SPEC T* s_value = nullptr;
};

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
    CORE_NODISCARD static T get() noexcept {
        return s_value;
    }

    /// Set current thread's value.
    static void set(T value) noexcept {
        s_value = value;
    }

private:
    inline static CORE_TLS_SPEC T s_value = T{};
};

#undef CORE_TLS_SPEC

} // namespace core
