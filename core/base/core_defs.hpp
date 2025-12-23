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