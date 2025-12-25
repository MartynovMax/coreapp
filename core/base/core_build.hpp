#pragma once

#include "core_config.hpp"
#include "core_macros.hpp"

// -----------------------------------------------------------------------------
// Build type detection
// -----------------------------------------------------------------------------

#ifndef CORE_BUILD_DEBUG
#define CORE_BUILD_DEBUG 0
#endif

#ifndef CORE_BUILD_RELEASE
#define CORE_BUILD_RELEASE 0
#endif

#ifndef CORE_BUILD_DEVELOPMENT
#define CORE_BUILD_DEVELOPMENT 0
#endif

#ifndef CORE_BUILD_SHIPPING
#define CORE_BUILD_SHIPPING 0
#endif

#if (CORE_BUILD_DEBUG + CORE_BUILD_RELEASE + CORE_BUILD_DEVELOPMENT + CORE_BUILD_SHIPPING) == 0
#if CORE_DEBUG
#undef CORE_BUILD_DEBUG
#define CORE_BUILD_DEBUG 1
#else
#undef CORE_BUILD_RELEASE
#define CORE_BUILD_RELEASE 1
#endif
#endif

#if (CORE_BUILD_DEBUG + CORE_BUILD_RELEASE + CORE_BUILD_DEVELOPMENT + CORE_BUILD_SHIPPING) != 1
#error "Build-type detection error: expected exactly one CORE_BUILD_* == 1."
#endif

// -----------------------------------------------------------------------------
// Build feature flags
// -----------------------------------------------------------------------------

#ifndef CORE_BUILD_WITH_ASSERTS
#define CORE_BUILD_WITH_ASSERTS (CORE_ASSERTIONS_ENABLED ? 1 : 0)
#endif

#ifndef CORE_BUILD_WITH_LOGS
#define CORE_BUILD_WITH_LOGS (CORE_ENABLE_LOGS ? 1 : 0)
#endif

// There is no fully portable macro to detect -O* across all toolchains,
// so by default we approximate: non-debug builds are "optimized".
#ifndef CORE_BUILD_WITH_OPTIMIZATIONS
#define CORE_BUILD_WITH_OPTIMIZATIONS (CORE_BUILD_DEBUG ? 0 : 1)
#endif

// Normalize to 0/1.
#if CORE_BUILD_WITH_ASSERTS
#undef CORE_BUILD_WITH_ASSERTS
#define CORE_BUILD_WITH_ASSERTS 1
#endif

#if CORE_BUILD_WITH_LOGS
#undef CORE_BUILD_WITH_LOGS
#define CORE_BUILD_WITH_LOGS 1
#endif

#if CORE_BUILD_WITH_OPTIMIZATIONS
#undef CORE_BUILD_WITH_OPTIMIZATIONS
#define CORE_BUILD_WITH_OPTIMIZATIONS 1
#endif