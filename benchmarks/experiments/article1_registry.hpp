#pragma once

// =============================================================================
// article1_registry.hpp
// Registration of all 31 Article 1 matrix scenarios into ExperimentRegistry.
//
// After calling RegisterArticle1Matrix(), the full matrix can be run with:
//   coreapp_benchmarks --filter=article1/*
// =============================================================================

namespace core::bench {

class ExperimentRegistry;

// Register all 31 Article 1 matrix scenarios.
// Scenarios are named: article1/{allocator}/{lifetime}/{workload}
// Safe to call multiple times (registry will reject duplicates).
void RegisterArticle1Matrix(ExperimentRegistry& registry) noexcept;

} // namespace core::bench

