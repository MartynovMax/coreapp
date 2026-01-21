#pragma once

// =============================================================================
// experiment_runner.hpp
// Main experiment execution engine.
// =============================================================================

#include "experiment_registry.hpp"
#include "run_config.hpp"
#include "exit_codes.hpp"
#include "../events/event_sink.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// ExperimentRunner - Executes registered experiments
// ----------------------------------------------------------------------------

class ExperimentRunner {
public:
    explicit ExperimentRunner(ExperimentRegistry& registry) noexcept;
    ~ExperimentRunner() noexcept = default;

    ExperimentRunner(const ExperimentRunner&) = delete;
    ExperimentRunner& operator=(const ExperimentRunner&) = delete;

    // Run experiments based on configuration
    // Returns exit code indicating overall result
    ExitCode Run(const RunConfig& config) noexcept;

    // Attach event sink for instrumentation (optional)
    void AttachEventSink(IEventSink* sink) noexcept;

    // Run single experiment (public for testing)
    bool RunExperiment(IExperiment* experiment, const ExperimentParams& params) noexcept;

private:
    ExperimentRegistry* _registry;
    IEventSink* _eventSink = nullptr;
};

} // namespace bench
} // namespace core
