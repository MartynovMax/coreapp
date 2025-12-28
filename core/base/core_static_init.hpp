#pragma once

// =============================================================================
// core_static_init.hpp
// Static initialization utilities and policies for Core.
//
// Goals
//   - Avoid unsafe global/static constructors.
//   - Control initialization order by preferring lazy initialization.
//   - Keep static usage explicit, documented, and easy to audit.
//
// Patterns encouraged by this header
//   - Prefer function-local statics accessed through a function.
//   - When a translation-unit static is needed, mark it explicitly.
// =============================================================================


#include "core_config.hpp"
#include "core_features.hpp"

// =============================================================================
// Policy flags
// =============================================================================

#ifndef CORE_ALLOW_STATIC_INIT
#define CORE_ALLOW_STATIC_INIT 1
#endif

#ifndef CORE_STRICT_STATIC_INIT
#define CORE_STRICT_STATIC_INIT 0
#endif

#ifndef CORE_FORBID_GLOBAL_STATICS
#define CORE_FORBID_GLOBAL_STATICS 0
#endif

#if CORE_FORBID_GLOBAL_STATICS
#undef CORE_ALLOW_STATIC_INIT
#define CORE_ALLOW_STATIC_INIT 0
#endif

// =============================================================================
// Annotations for static usage
// =============================================================================

#ifndef CORE_STATIC_ALLOWED
#define CORE_STATIC_ALLOWED
#endif

#ifndef CORE_STATIC_INTERNAL
#define CORE_STATIC_INTERNAL
#endif

// =============================================================================
// Feature: thread-safe function-local statics
// =============================================================================

#ifndef CORE_HAS_THREADSAFE_LOCAL_STATICS
#define CORE_HAS_THREADSAFE_LOCAL_STATICS (CORE_CPP11_OR_GREATER && CORE_HAS_THREADS)
#endif

// =============================================================================
// Lazy initialization helpers
// =============================================================================

#define CORE_LAZY_STATIC(Type, Name)                                           \
  static Type& Name() {                                                        \
    CORE_STATIC_INTERNAL static Type instance;                                 \
    return instance;                                                           \
  }

#define CORE_LAZY_STATIC_THREADSAFE(Type, Name)                                \
  static_assert(CORE_HAS_THREADSAFE_LOCAL_STATICS,                              \
                "CORE_LAZY_STATIC_THREADSAFE requires thread-safe local statics"); \
  CORE_LAZY_STATIC(Type, Name)

// =============================================================================
// Global/static helper (enforcement point)
// =============================================================================

namespace core {
    namespace detail {
        template <typename T>
        struct StaticInitFalse {
            static constexpr bool kValue = false;
        };
    } // namespace detail
} // namespace core

#if CORE_FORBID_GLOBAL_STATICS
#define CORE_GLOBAL_STATIC(Type, name)                                         \
  static_assert(::core::detail::StaticInitFalse<int>::kValue,                   \
                "Global/namespace-scope statics are forbidden in this build. "  \
                "Use CORE_LAZY_STATIC or remove static state.")
#else
#define CORE_GLOBAL_STATIC(Type, name) CORE_STATIC_ALLOWED static Type name
#endif