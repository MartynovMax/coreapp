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

// ----------------------------------------------------------------------------
// SizeDistribution - Parameters for allocation size generation
// ----------------------------------------------------------------------------

struct SizeDistribution {
    DistributionType type;
    u32 minSize;
    u32 maxSize;
    
    // For statistical distributions (Normal, LogNormal):
    float mean = 0.0f;
    float stddev = 0.0f;
    
    // For shape-based distributions (Pareto, Exponential):
    float shape = 0.0f;
    
    // For Bimodal distribution:
    u32 peak1Min = 0;
    u32 peak1Max = 0;
    u32 peak2Min = 0;
    u32 peak2Max = 0;
    float peak1Weight = 0.5f;       // Probability of first peak (0.0 to 1.0)
    
    // For CustomBuckets distribution:
    const u32* buckets = nullptr;   // Array of size buckets
    const float* weights = nullptr; // Array of weights (must sum to 1.0)
    u32 bucketCount = 0;
};

// ----------------------------------------------------------------------------
// LifetimeModel - Object lifetime patterns
// ----------------------------------------------------------------------------

enum class LifetimeModel {
    Fifo,               // First-in first-out (queue pattern)
    Lifo,               // Last-in first-out (stack pattern)
    Random,             // Random lifetime selection
    Bounded,            // Bounded live-set (steady-state behavior)
    LongLived,          // Objects live until bulk reclaim
};

// ----------------------------------------------------------------------------
// WorkloadParams - Complete workload parameterization
// ----------------------------------------------------------------------------

struct WorkloadParams {
    // Core parameters:
    u64 seed = 0;                       // Deterministic seed for RNG
    u64 operationCount = 0;             // Number of operations to execute
    
    // Size distribution:
    SizeDistribution sizeDistribution = {};
    
    // Operation mix:
    float allocFreeRatio = 1.0f;        // 1.0 = alloc only, 0.5 = 50/50, 0.0 = free only
    
    // Lifetime behavior:
    LifetimeModel lifetimeModel = LifetimeModel::Fifo;
    u32 maxLiveObjects = 0;             // For Bounded model (0 = unlimited)
    
    // Tick configuration:
    u64 tickInterval = 0;               // Emit tick event every N operations (0 = disabled)
};

} // namespace bench
} // namespace core
