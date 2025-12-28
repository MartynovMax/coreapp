#pragma once

#include "core/base/core_platform.hpp"

// -----------------------------------------------------------------------------
// C++ standard version helpers
// -----------------------------------------------------------------------------

#if !defined(CORE_CPP_VERSION)
#if CORE_COMPILER_MSVC && !defined(__clang__)
#define CORE_CPP_VERSION _MSVC_LANG
#else
#define CORE_CPP_VERSION __cplusplus
#endif
#endif

#define CORE_CPP11_OR_GREATER (CORE_CPP_VERSION >= 201103L)
#define CORE_CPP14_OR_GREATER (CORE_CPP_VERSION >= 201402L)
#define CORE_CPP17_OR_GREATER (CORE_CPP_VERSION >= 201703L)
#define CORE_CPP20_OR_GREATER (CORE_CPP_VERSION >= 202002L)