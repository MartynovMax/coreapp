#pragma once

// -----------------------------------------------------------------------------
// Stringification and concatenation
// -----------------------------------------------------------------------------

#define CORE_STRINGIFY_IMPL(x) #x
#define CORE_STRINGIFY(x) CORE_STRINGIFY_IMPL(x)

// Expands x first, then stringifies.
#define CORE_STRINGIFY_VALUE(x) CORE_STRINGIFY_IMPL(x)

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