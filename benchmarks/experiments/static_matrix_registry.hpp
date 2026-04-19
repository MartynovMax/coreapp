#pragma once

// =============================================================================
// static_matrix_registry.hpp
// Registration of all 31 static matrix scenarios into ExperimentRegistry.
//
// After calling RegisterStaticMatrix(), the full matrix can be run with:
//   coreapp_benchmarks --filter=<prefix>/*
//
// NOTE: The canonical source of truth for all scenario parameters is
// the corresponding JSON config file (e.g. workspace/config/article1_matrix.json).
// The C++ static table must be kept in sync with that file.
// =============================================================================

#include "alloc_bench_experiment.hpp"

namespace core::bench {

class ExperimentRegistry;

// Register all 31 static matrix scenarios from the built-in C++ static table.
// Safe to call multiple times (registry will reject duplicates).
//
// Prefer LoadScenariosFromJson() with the appropriate config for production runs,
// as it applies default_seed and default_repetitions from the JSON file.
// This function is provided as a fallback when the JSON file is unavailable.
void RegisterStaticMatrix(ExperimentRegistry& registry) noexcept;

// Legacy alias for backward compatibility with existing tests.
inline void RegisterArticle1Matrix(ExperimentRegistry& registry) noexcept {
    RegisterStaticMatrix(registry);
}

// Expose the static matrix for drift-detection tests.
// Returns pointer to internal array; count is set to the number of entries.
const AllocBenchConfig* GetStaticMatrixEntries(u32& count) noexcept;

// Legacy alias
inline const AllocBenchConfig* GetArticle1MatrixEntries(u32& count) noexcept {
    return GetStaticMatrixEntries(count);
}

} // namespace core::bench
