#pragma once

#include "core/base/core_types.hpp"
#include "core/base/core_macros.hpp"

// -----------------------------------------------------------------------------
// Identifier defaults
// -----------------------------------------------------------------------------

#define CORE_INVALID_ID32 (::core::kInvalidId32)
#define CORE_INVALID_ID64 (::core::kInvalidId64)

// Optional generic aliases (keep them subsystem-agnostic).
#define CORE_INVALID_ENTITY (CORE_INVALID_ID32)
#define CORE_INVALID_HANDLE (CORE_INVALID_ID64)

// Invalid index sentinel (signed index convention).
// If you use unsigned indices in some subsystems, prefer CORE_U32_MAX / CORE_USIZE_MAX.
#define CORE_INDEX_NONE (static_cast<::core::isize>(-1))

// -----------------------------------------------------------------------------
// Numeric limits and sentinel values (expressed via Core types)
// -----------------------------------------------------------------------------

#define CORE_U8_MAX  (::core::kU8Max)
#define CORE_U16_MAX (::core::kU16Max)
#define CORE_U32_MAX (::core::kU32Max)
#define CORE_U64_MAX (::core::kU64Max)

#define CORE_I8_MIN  (::core::kI8Min)
#define CORE_I8_MAX  (::core::kI8Max)
#define CORE_I16_MIN (::core::kI16Min)
#define CORE_I16_MAX (::core::kI16Max)
#define CORE_I32_MIN (::core::kI32Min)
#define CORE_I32_MAX (::core::kI32Max)
#define CORE_I64_MIN (::core::kI64Min)
#define CORE_I64_MAX (::core::kI64Max)

// Convenience aliases frequently needed in containers / allocators.
#define CORE_USIZE_MAX (static_cast<::core::usize>(CORE_U64_MAX))
#define CORE_ISIZE_MIN (static_cast<::core::isize>(CORE_I64_MIN))
#define CORE_ISIZE_MAX (static_cast<::core::isize>(CORE_I64_MAX))

// -----------------------------------------------------------------------------
// Boolean and default flag values (minimal / generic)
// -----------------------------------------------------------------------------

#define CORE_FLAGS_NONE (0u)