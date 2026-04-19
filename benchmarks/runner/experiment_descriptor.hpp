#pragma once

// =============================================================================
// experiment_descriptor.hpp
// Metadata and factory for registering experiments.
// =============================================================================

#include "experiment_interface.hpp"

namespace core {
namespace bench {

// Factory function type: creates experiment instance
using ExperimentFactory = IExperiment* (*)();

// ----------------------------------------------------------------------------
// ExperimentDescriptor - Registration metadata
// ----------------------------------------------------------------------------

struct ExperimentDescriptor {
    const char* name = nullptr;             // Unique identifier
    const char* category = nullptr;         // Category for grouping
    const char* allocatorName = nullptr;    // Allocator under test
    const char* description = nullptr;      // Human-readable description
    ExperimentFactory factory = nullptr;    // Factory function

    // Per-scenario defaults (0 = use global RunConfig value / built-in default)
    u64 scenarioSeed        = 0;            // Default seed for this scenario
    u32 scenarioRepetitions = 0;            // Default repetition count for this scenario
};

} // namespace bench
} // namespace core
