#pragma once

#include "core/base/core_config.hpp"
#include "core/base/core_features.hpp"

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