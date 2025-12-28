#pragma once

// =============================================================================
// Core-wide global constants and macros.
// =============================================================================

#include "core_macros.hpp"
#include "core_types.hpp"

// -----------------------------------------------------------------------------
// Identifier defaults
// -----------------------------------------------------------------------------

#define CORE_INVALID_ID32 (::core::kInvalidId32)
#define CORE_INVALID_ID64 (::core::kInvalidId64)

// Optional generic aliases (keep them subsystem-agnostic).
#define CORE_INVALID_ENTITY (CORE_INVALID_ID32)
#define CORE_INVALID_HANDLE (CORE_INVALID_ID64)

// Invalid index sentinel (signed index convention).
// If you use unsigned indices in some subsystems, prefer CORE_U32_MAX /
// CORE_USIZE_MAX.
#define CORE_INDEX_NONE (static_cast<::core::isize>(-1))

// -----------------------------------------------------------------------------
// Numeric limits and sentinel values (expressed via Core types)
// -----------------------------------------------------------------------------

#define CORE_U8_MAX (::core::kU8Max)
#define CORE_U16_MAX (::core::kU16Max)
#define CORE_U32_MAX (::core::kU32Max)
#define CORE_U64_MAX (::core::kU64Max)

#define CORE_I8_MIN (::core::kI8Min)
#define CORE_I8_MAX (::core::kI8Max)
#define CORE_I16_MIN (::core::kI16Min)
#define CORE_I16_MAX (::core::kI16Max)
#define CORE_I32_MIN (::core::kI32Min)
#define CORE_I32_MAX (::core::kI32Max)
#define CORE_I64_MIN (::core::kI64Min)
#define CORE_I64_MAX (::core::kI64Max)

// Convenience aliases frequently needed in containers / allocators.
// Build these from usize to avoid 32-bit truncation warnings.
#define CORE_USIZE_MAX                                                         \
  (static_cast<::core::usize>(~static_cast<::core::usize>(0)))
#define CORE_ISIZE_MAX (static_cast<::core::isize>(CORE_USIZE_MAX >> 1))
#define CORE_ISIZE_MIN (static_cast<::core::isize>(-CORE_ISIZE_MAX - 1))

// -----------------------------------------------------------------------------
// Boolean and default flag values (minimal / generic)
// -----------------------------------------------------------------------------

#define CORE_FLAGS_NONE (0u)

// -----------------------------------------------------------------------------
// Version macros (Core version; overridable from build system)
// -----------------------------------------------------------------------------

#ifndef CORE_VERSION_MAJOR
#define CORE_VERSION_MAJOR 0
#endif

#ifndef CORE_VERSION_MINOR
#define CORE_VERSION_MINOR 1
#endif

#ifndef CORE_VERSION_PATCH
#define CORE_VERSION_PATCH 0
#endif

// Optional build number, suitable for CI injection.
#ifndef CORE_VERSION_BUILD
#define CORE_VERSION_BUILD 0
#endif

// "M.m.p"
#define CORE_VERSION_STRING                                                    \
  CORE_STRINGIFY(CORE_VERSION_MAJOR)                                           \
  "." CORE_STRINGIFY(CORE_VERSION_MINOR) "." CORE_STRINGIFY(CORE_VERSION_PATCH)

// "M.m.p.b" (handy for CI artifacts; do not treat as SemVer by default)
#define CORE_VERSION_STRING_FULL                                               \
  CORE_VERSION_STRING "." CORE_STRINGIFY(CORE_VERSION_BUILD)

// -----------------------------------------------------------------------------
// General-purpose constants (sizes, offsets, time sentinels)
// -----------------------------------------------------------------------------

#define CORE_KILOBYTE (static_cast<::core::usize>(1024u))
#define CORE_MEGABYTE (static_cast<::core::usize>(1024u) * CORE_KILOBYTE)
#define CORE_GIGABYTE (static_cast<::core::usize>(1024u) * CORE_MEGABYTE)

// Invalid offset sentinel (common for "no offset / not found").
#define CORE_INVALID_OFFSET (static_cast<::core::offset_t>(-1))

// Optional time-related sentinel (generic enough for Core timing primitives).
#define CORE_INVALID_TICK (static_cast<::core::tick_t>(-1))