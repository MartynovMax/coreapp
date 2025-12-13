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