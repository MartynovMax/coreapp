#pragma once

// =============================================================================
// core_macros.hpp
// Standalone preprocessor + utility macros.
// IMPORTANT: No platform/compiler detection here (lives in core_config.hpp).
// =============================================================================

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

// Forces one more expansion step.
#define CORE_EXPAND(x) x

// Avoid unused variable warnings.
#define CORE_UNUSED_VAR(x) (void)(x)

// -----------------------------------------------------------------------------
// Stringification and concatenation
// -----------------------------------------------------------------------------

// Stringify without macro expansion ("as written").
#define CORE_STRINGIFY_RAW(x) #x

// Expand x first, then stringify.
#define CORE_STRINGIFY_IMPL(x) #x
#define CORE_STRINGIFY(x) CORE_STRINGIFY_IMPL(x)

// Optional alias for readability.
#define CORE_STRINGIFY_VALUE(x) CORE_STRINGIFY(x)

#define CORE_CONCAT_IMPL(a, b) a##b
#define CORE_CONCAT(a, b) CORE_CONCAT_IMPL(a, b)

// -----------------------------------------------------------------------------
// Array size
// NOTE: Intentionally unsafe for pointers (will "work" incorrectly by design).
// -----------------------------------------------------------------------------

#define CORE_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// -----------------------------------------------------------------------------
// Bit / flag helpers
// -----------------------------------------------------------------------------

#define CORE_BIT(n) (1u << (n))

#define CORE_HAS_FLAG(value, flag) (((value) & (flag)) != 0)
#define CORE_SET_FLAG(value, flag) ((value) |= (flag))
#define CORE_CLEAR_FLAG(value, flag) ((value) &= ~(flag))

// -----------------------------------------------------------------------------
// Static-assert helpers
// -----------------------------------------------------------------------------

#define CORE_STATIC_ASSERT(expr, msg) static_assert((expr), msg)

#define CORE_STATIC_ASSERT_SIZE(type, expected)                                \
  static_assert(sizeof(type) == (expected), "Unexpected sizeof(" #type ")")

// -----------------------------------------------------------------------------
// Optional annotations
// -----------------------------------------------------------------------------

// Avoid compiler-specific pragmas here.
#define CORE_TODO(msg) /* TODO: msg */

// Standard C++ attribute when available; otherwise no-op.
#if defined(__cplusplus) && (__cplusplus >= 201402L)
#define CORE_DEPRECATED(msg) [[deprecated(msg)]]
#else
#define CORE_DEPRECATED(msg)
#endif