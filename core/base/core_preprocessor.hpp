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

// -----------------------------------------------------------------------------
// Attribute detection
// -----------------------------------------------------------------------------

#if !defined(CORE_HAS_CPP_ATTRIBUTE)
#if defined(__has_cpp_attribute)
#define CORE_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define CORE_HAS_CPP_ATTRIBUTE(x) 0
#endif
#endif

#if !defined(CORE_HAS_NODISCARD)
#define CORE_HAS_NODISCARD ((CORE_HAS_CPP_ATTRIBUTE(nodiscard) != 0) || CORE_CPP17_OR_GREATER)
#endif

#if !defined(CORE_HAS_NORETURN)
#define CORE_HAS_NORETURN ((CORE_HAS_CPP_ATTRIBUTE(noreturn) != 0) || CORE_CPP11_OR_GREATER)
#endif

#if !defined(CORE_HAS_FALLTHROUGH)
#define CORE_HAS_FALLTHROUGH ((CORE_HAS_CPP_ATTRIBUTE(fallthrough) != 0) || CORE_CPP17_OR_GREATER)
#endif

#if !defined(CORE_HAS_DEPRECATED)
#define CORE_HAS_DEPRECATED ((CORE_HAS_CPP_ATTRIBUTE(deprecated) != 0) || CORE_CPP14_OR_GREATER)
#endif