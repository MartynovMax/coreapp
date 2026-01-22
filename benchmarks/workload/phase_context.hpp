#pragma once

// =============================================================================
// phase_context.hpp
// Execution context for phase operations.
//
// Provides access to allocator, lifetime tracker, RNG, event sink,
// and runtime metrics during phase execution.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// Forward declarations
class IAllocator;
class LifetimeTracker;
class SeededRNG;
class IEventSink;

} // namespace bench
} // namespace core
