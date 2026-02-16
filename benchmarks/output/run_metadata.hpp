#pragma once

// =============================================================================
// run_metadata.hpp
// Metadata required for structured output (run_id, experiment context).
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

struct RunMetadata {
    const char* runId = nullptr;              // Unique run identifier
    const char* experimentName = nullptr;     // Experiment name
    const char* experimentCategory = nullptr; // Experiment category
    const char* allocatorName = nullptr;      // Allocator being tested
    u64 seed = 0;                             // Deterministic seed
    u32 warmupIterations = 0;                 // Warmup iteration count
    u32 measuredRepetitions = 0;              // Measured repetition count
    const char* filter = nullptr;             // Filter pattern (if any)
    u64 startTimestampNs = 0;                 // Run start timestamp
};

} // namespace bench
} // namespace core
