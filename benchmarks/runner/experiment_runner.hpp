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

class IMeasurementSystem;
class MetricCollector;

class ExperimentRunner {
public:
    explicit ExperimentRunner(ExperimentRegistry& registry) noexcept;
    ~ExperimentRunner() noexcept;

    ExperimentRunner(const ExperimentRunner&) = delete;
    ExperimentRunner& operator=(const ExperimentRunner&) = delete;

    ExitCode Run(const RunConfig& config) noexcept;

    void AttachEventSink(IEventSink* sink) noexcept;
    void RegisterMeasurementSystem(IMeasurementSystem* system) noexcept;
    void EnableMeasurementSystem(const char* systemName, bool enabled) noexcept;

    bool RunExperiment(IExperiment* experiment, const ExperimentParams& params) noexcept;

    MetricCollector* GetMetricCollector() noexcept { return _metricCollector; }

private:
    static constexpr u32 kMaxMeasurementSystems = 16;

    ExperimentRegistry* _registry;
    IEventSink* _eventSink = nullptr;

    IMeasurementSystem* _measurementSystems[kMaxMeasurementSystems];
    u32 _measurementSystemCount;
    MetricCollector* _metricCollector;

    void NotifyRunStart(const char* experimentName, u64 seed, u32 repetitions) noexcept;
    void NotifyRunEnd() noexcept;
    void PublishAllMetrics() noexcept;
};

} // namespace bench
} // namespace core
