#pragma once

// =============================================================================
// phase_descriptor.hpp
// Phase description and callback-based reclaim model.
//
// Defines phase types, reclaim modes, and phase descriptors with callback
// support for flexible allocator-specific operations.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "workload_params.hpp"

namespace core {
namespace bench {

// Forward declarations
struct PhaseContext;
class PhaseExecutor;

} // namespace bench
} // namespace core
