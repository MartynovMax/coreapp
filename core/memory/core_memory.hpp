#pragma once

// =============================================================================
// core_memory.hpp
// Memory configuration and fundamental types.
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_assert.hpp"
#include "../base/core_types.hpp"

namespace core {

// ----------------------------------------------------------------------------
// Memory configuration macros
// ----------------------------------------------------------------------------

#ifndef CORE_MEMORY_DEBUG
#define CORE_MEMORY_DEBUG CORE_DEBUG
#endif

#ifndef CORE_MEMORY_TRACKING
#define CORE_MEMORY_TRACKING CORE_DEBUG
#endif

#ifndef CORE_DEFAULT_ALIGNMENT
#define CORE_DEFAULT_ALIGNMENT 16u
#endif

#ifndef CORE_CACHE_LINE_SIZE
#define CORE_CACHE_LINE_SIZE 64u
#endif

#ifndef CORE_MEM_ASSERT
#define CORE_MEM_ASSERT(expr) ASSERT(expr)
#endif

// ----------------------------------------------------------------------------
// Fundamental memory types
// ----------------------------------------------------------------------------

using memory_size = usize;
using memory_alignment = u32;
using memory_tag = u32;

} // namespace core

