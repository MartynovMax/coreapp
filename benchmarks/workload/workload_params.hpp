#pragma once

// =============================================================================
// workload_params.hpp
// Workload parameterization for phase-based experiments.
//
// Defines distribution types, lifetime models, and workload parameters
// for deterministic, reproducible benchmark workloads.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// DistributionType - Size distribution patterns for allocations
// ----------------------------------------------------------------------------

enum class DistributionType {
    // Basic distributions:
    Uniform,            // Uniform distribution in [min, max]
    PowerOfTwo,         // Only powers of two (8, 16, 32, 64, ...)
    
    // Statistical distributions:
    Normal,             // Normal (Gaussian) distribution
    LogNormal,          // Log-normal distribution (many small, rare large)
    Exponential,        // Exponential distribution
    Pareto,             // Pareto distribution (80/20 rule)
    
    // Practical patterns:
    SmallBiased,        // 90% small (8-64 bytes), 10% large
    LargeBiased,        // 90% large, 10% small
    Bimodal,            // Two distinct peaks (e.g., 16-32 and 1024-2048)
    
    // Realistic workload presets:
    WebServerAlloc,     // Web server allocation pattern
    GameEngine,         // Game engine allocation pattern
    DatabaseCache,      // Database cache allocation pattern
    
    // Custom:
    CustomBuckets,      // User-defined buckets with weights
};

} // namespace bench
} // namespace core
