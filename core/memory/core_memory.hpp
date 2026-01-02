#pragma once

// =============================================================================
// core_memory.hpp
// Lightweight foundational header for Core memory subsystem.
//
// This header defines:
//   - Memory configuration macros
//   - Fundamental memory types (size, alignment, tag)
//   - Allocation request/info structures
//   - Base allocator interface
//   - Basic alignment utilities
//   - Memory hook declarations
//
// This header serves as the root of the memory subsystem API and must remain
// extremely lightweight and broadly includable.
// =============================================================================

#include "core/base/core_config.hpp"
#include "core/base/core_assert.hpp"

namespace core {

// ----------------------------------------------------------------------------
// Memory configuration macros
// ----------------------------------------------------------------------------

#ifndef CORE_MEMORY_DEBUG
#define CORE_MEMORY_DEBUG CORE_DEBUG
#endif

#ifndef CORE_MEMORY_TRACKING
#define CORE_MEMORY_TRACKING 0
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

} // namespace core

