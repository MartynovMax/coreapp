#pragma once

#include "core_config.hpp"
#include "core_macros.hpp"

// -----------------------------------------------------------------------------
// 1) Build type detection
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