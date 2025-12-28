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