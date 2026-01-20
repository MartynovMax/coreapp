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
    const char* name = nullptr;             // Unique identifier (e.g., "arena/monotonic_fixed_16")
    const char* category = nullptr;         // Category (e.g., "arena", "allocator", "stress")
    const char* allocatorName = nullptr;    // Allocator under test (e.g., "monotonic_arena")
    const char* description = nullptr;      // Human-readable description
    ExperimentFactory factory = nullptr;    // Factory function to create instance
};

} // namespace bench
} // namespace core
