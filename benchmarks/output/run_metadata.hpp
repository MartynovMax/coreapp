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

    // Environment and build metadata (required for offline comparison)
    const char* runTimestampUtc = "0";        // Run start timestamp (nanoseconds string)
    const char* osName = "unknown";           // OS name (Windows, Linux, macOS)
    const char* osVersion = "unknown";        // OS version string
    const char* arch = "unknown";             // Architecture (x64, ARM64, etc.)
    const char* compilerName = "unknown";     // Compiler (MSVC, GCC, Clang)
    const char* compilerVersion = "unknown";  // Compiler version string
    const char* buildType = "unknown";        // Build type (Debug, Release, etc.)
    const char* buildFlags = "unknown";       // Key build flags (normalized)
    const char* cpuModel = "unknown";         // CPU model string
    u32 cpuCoresLogical = 0;                  // Logical cores (0 if unavailable)
    u32 cpuCoresPhysical = 0;                 // Physical cores (0 if unavailable)
};

} // namespace bench
} // namespace core
