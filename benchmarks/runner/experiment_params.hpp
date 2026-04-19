#pragma once

// =============================================================================
// experiment_params.hpp
// Parameters passed to experiments at setup time.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// ExperimentParams - Configuration for experiment execution
// ----------------------------------------------------------------------------

struct ExperimentParams {
    u64 seed = 0;                       // Deterministic seed for RNG
    u32 warmupIterations = 0;           // Number of warmup runs (not measured)
    u32 measuredRepetitions = 1;        // Number of measured runs
    u32 repetitionIndex = 0;            // Current repetition index (0-based, set per iteration)
};

} // namespace bench
} // namespace core
