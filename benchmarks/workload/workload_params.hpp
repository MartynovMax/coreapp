#pragma once

// =============================================================================
// workload_params.hpp
// Workload parameterization for phase-based experiments.
//
// Defines distribution types, lifetime models, and workload parameters
// for deterministic, reproducible benchmark workloads.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "core/memory/core_allocator.hpp" // AllocationFlags + memory types/macros

namespace core::bench {

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
    DistributionType type = DistributionType::Uniform;
    u32 minSize = 8;
    u32 maxSize = 64;

    // For statistical distributions (Normal, LogNormal):
    f32 mean = 0.0f;
    f32 stddev = 0.0f;

    bool meanInLogSpace = true;

    // For shape-based distributions (Pareto, Exponential):
    f32 shape = 0.0f;

    // For Bimodal distribution:
    u32 peak1Min = 0;
    u32 peak1Max = 0;
    u32 peak2Min = 0;
    u32 peak2Max = 0;
    f32 peak1Weight = 0.5f;       // Probability of first peak (0.0 to 1.0)

    // For CustomBuckets distribution:
    const u32* buckets = nullptr;   // Array of size buckets
    const f32* weights = nullptr; // Array of weights (must sum to 1.0)
    u32 bucketCount = 0;
};

// ----------------------------------------------------------------------------
// AlignmentDistribution - Parameters for alignment generation
// ----------------------------------------------------------------------------

enum class AlignmentDistributionType {
    Fixed,              // Always use fixedAlignment (0 = allocator default)
    PowerOfTwoRange,    // Choose power-of-two in [minAlignment, maxAlignment]
    MatchSizePow2,      // alignment = clamp(next_pow2(size), minAlignment..maxAlignment)
    Typical64,          // Weighted {8,16,32,64} (weights)
    CustomBuckets,      // User-defined buckets with weights
};

struct AlignmentDistribution {
    AlignmentDistributionType type = AlignmentDistributionType::Fixed;
    core::memory_alignment fixedAlignment = 0;
    core::memory_alignment minAlignment = 8;
    core::memory_alignment maxAlignment = 64;
    const core::memory_alignment* buckets = nullptr;
    const f32* weights = nullptr;
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
    

    // Alignment distribution:
    AlignmentDistribution alignmentDistribution = {};

    // Allocation metadata:
    core::memory_tag tag = 0;
    core::AllocationFlags flags = core::AllocationFlags::None;
    // Operation mix:
    f32 allocFreeRatio = 1.0f;        // 1.0 = alloc only, 0.5 = 50/50, 0.0 = free only

    // Lifetime behavior:
    LifetimeModel lifetimeModel = LifetimeModel::Fifo;
    u32 maxLiveObjects = 0;             // For Bounded model (0 = "unlimited", but internally capped by operationCount or a hard limit)

    // Tick configuration:
    u64 tickInterval = 0;               // Emit tick event every N operations (0 = disabled)
};

// ----------------------------------------------------------------------------
// SizePresets - Factory functions for common size distributions
// ----------------------------------------------------------------------------

namespace SizePresets {

inline SizeDistribution SmallObjects() noexcept {
    return SizeDistribution{
        .type = DistributionType::Uniform,
        .minSize = 8,
        .maxSize = 64,
    };
}

inline SizeDistribution MediumObjects() noexcept {
    return SizeDistribution{
        .type = DistributionType::Uniform,
        .minSize = 64,
        .maxSize = 512,
    };
}

inline SizeDistribution LargeObjects() noexcept {
    return SizeDistribution{
        .type = DistributionType::Uniform,
        .minSize = 512,
        .maxSize = 4096,
    };
}

inline SizeDistribution WebServer() noexcept {
    return SizeDistribution{
        .type = DistributionType::LogNormal,
        .minSize = 16,
        .maxSize = 4096,
        .mean = 256.0f,
        .stddev = 512.0f,
    };
}

inline SizeDistribution GameEntityPool() noexcept {
    return SizeDistribution{
        .type = DistributionType::Bimodal,
        .minSize = 8,
        .maxSize = 8192,
        .peak1Min = 16,
        .peak1Max = 64,
        .peak2Min = 1024,
        .peak2Max = 2048,
        .peak1Weight = 0.85f,
    };
}

inline SizeDistribution DatabasePages() noexcept {
    return SizeDistribution{
        .type = DistributionType::PowerOfTwo,
        .minSize = 4096,
        .maxSize = 16384,
    };
}

} // namespace SizePresets

// ----------------------------------------------------------------------------
// AlignmentPresets - Common alignment distributions
// ----------------------------------------------------------------------------

namespace AlignmentPresets {

inline AlignmentDistribution Default() noexcept {
    // FixedAlignment=0 => allocator will normalize to CORE_DEFAULT_ALIGNMENT.
    return AlignmentDistribution{
        .type = AlignmentDistributionType::Fixed,
        .fixedAlignment = 0,
    };
}

inline AlignmentDistribution CacheLine() noexcept {
    return AlignmentDistribution{
        .type = AlignmentDistributionType::Fixed,
        .fixedAlignment = CORE_CACHE_LINE_SIZE,
    };
}

inline AlignmentDistribution MatchSizePow2(
    core::memory_alignment minA = 8,
    core::memory_alignment maxA = 64) noexcept
{
    return AlignmentDistribution{
        .type = AlignmentDistributionType::MatchSizePow2,
        .fixedAlignment = 0,
        .minAlignment = minA,
        .maxAlignment = maxA,
    };
}

inline AlignmentDistribution PowerOfTwoRange(
    core::memory_alignment minA = 8,
    core::memory_alignment maxA = 64) noexcept
{
    return AlignmentDistribution{
        .type = AlignmentDistributionType::PowerOfTwoRange,
        .fixedAlignment = 0,
        .minAlignment = minA,
        .maxAlignment = maxA,
    };
}

} // namespace AlignmentPresets

} // namespace core::bench
