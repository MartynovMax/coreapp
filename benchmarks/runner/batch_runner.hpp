#pragma once

// =============================================================================
// batch_runner.hpp
// Orchestrates a full matrix batch run: iterates all registered scenarios,
// creates timestamped output directories, and writes a batch-level manifest.
// =============================================================================

#include "run_config.hpp"
#include "exit_codes.hpp"

namespace core {
namespace bench {

class ExperimentRegistry;

// Per-scenario result collected during a batch run.
struct BatchScenarioResult {
    const char* scenarioName = nullptr;
    char sanitizedName[128] = {};
    char outputDir[512] = {};
    ExitCode exitCode = kFailure;
    u64 durationMs = 0;
};

class BatchRunner {
public:
    BatchRunner(ExperimentRegistry& registry, const RunConfig& baseConfig) noexcept;
    ~BatchRunner() noexcept = default;

    BatchRunner(const BatchRunner&) = delete;
    BatchRunner& operator=(const BatchRunner&) = delete;

    // Execute the full batch.  Returns kSuccess if all scenarios pass,
    // kPartialFailure if some fail, kFailure if all fail.
    ExitCode Run() noexcept;

private:
    ExperimentRegistry* _registry;
    RunConfig _baseConfig;

    static constexpr u32 kMaxBatchScenarios = 256;
    BatchScenarioResult _results[kMaxBatchScenarios];
    u32 _resultCount = 0;

    // Write the top-level batch manifest.json
    bool WriteBatchManifest(const char* batchDir, const char* timestamp,
                            u64 totalDurationMs) noexcept;

    // Merge per-scenario summary CSVs into a single top-level summary.csv
    bool MergeSummaries(const char* batchDir) noexcept;
};

} // namespace bench
} // namespace core


