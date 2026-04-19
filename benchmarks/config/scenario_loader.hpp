#pragma once

// =============================================================================
// scenario_loader.hpp
// Runtime loader: reads a JSON scenario matrix and produces AllocBenchConfig[].
//
// Usage:
//   ScenarioLoadResult r = LoadScenariosFromJson("config/my_matrix.json");
//   if (!r.ok) { printf("Error: %s\n", r.errorMessage); }
//
// JSON format expected:
//   {
//     "run_prefix": "<name>",        // optional — used for output dirs & category
//     "workload_profiles": { "<name>": { "size_min", "size_max", "operation_count",
//                                        "max_live_objects", "alloc_free_ratio" }, ... },
//     "scenarios": [ { "name", "allocator", "lifetime", "workload",
//                      "seed"?(u64), "repetitions"?(u32) }, ... ]
//   }
// =============================================================================

#include "../experiments/alloc_bench_experiment.hpp"

namespace core::bench {

static constexpr u32 kMaxLoadedScenarios = 256;

// Result of a single JSON load attempt.
struct ScenarioLoadResult {
    AllocBenchConfig scenarios[kMaxLoadedScenarios];
    u32  count        = 0;
    bool ok           = false;
    char errorMessage[256] = {};
    char runPrefix[64] = {};    // Parsed from JSON "run_prefix" or derived from filename
    char category[64] = {};     // Same as runPrefix — used as experiment category
};

// Load scenarios from a JSON file.
// Never throws — all exceptions are caught and converted to errorMessage.
ScenarioLoadResult LoadScenariosFromJson(const char* path) noexcept;

// Register a single AllocBenchConfig into an ExperimentRegistry.
// category is used as ExperimentDescriptor::category (e.g. "article1").
class ExperimentRegistry;
void RegisterLoadedScenario(ExperimentRegistry& registry,
                             const AllocBenchConfig& cfg,
                             const char* category = "experiment") noexcept;

} // namespace core::bench

