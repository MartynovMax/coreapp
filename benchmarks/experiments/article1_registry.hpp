#pragma once

// =============================================================================
// article1_registry.hpp
// Registration of all 31 Article 1 matrix scenarios into ExperimentRegistry.
//
// After calling RegisterArticle1Matrix(), the full matrix can be run with:
//   coreapp_benchmarks --filter=article1/*
//
// NOTE: The canonical source of truth for all scenario parameters is:
//   benchmarks/config/article1_matrix.json
// The C++ static table must be kept in sync with that file.
// =============================================================================

#include "alloc_bench_experiment.hpp"

namespace core::bench {

class ExperimentRegistry;

// Register all 31 Article 1 matrix scenarios from the built-in C++ static table.
// Scenarios are named: article1/{allocator}/{lifetime}/{workload}
// Safe to call multiple times (registry will reject duplicates).
//
// Prefer LoadScenariosFromJson("config/article1_matrix.json") for production runs,
// as it applies default_seed and default_repetitions from the JSON file.
// This function is provided as a fallback when the JSON file is unavailable.
void RegisterArticle1Matrix(ExperimentRegistry& registry) noexcept;

// Expose the static matrix for drift-detection tests.
// Returns pointer to internal array; count is set to the number of entries.
const AllocBenchConfig* GetArticle1MatrixEntries(u32& count) noexcept;

} // namespace core::bench

