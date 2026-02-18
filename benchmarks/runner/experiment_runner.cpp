#include "experiment_runner.hpp"
#include "../events/event_types.hpp"
#include "../measurement/measurement_system.hpp"
#include "../measurement/metric_collector.hpp"
#include "../common/string_utils.hpp"
#include "../common/high_res_timer.hpp"
#include "../output/output_manager.hpp"
#include "../output/output_config.hpp"
#include "../output/run_metadata.hpp"
#include "../system/environment_metadata.hpp"
#include <stdio.h>

namespace core {
namespace bench {

namespace {

// Generate unique run identifier
void GenerateRunId(char* buffer, size_t bufferSize, u64 seed) noexcept {
    HighResTimer timer;
    u64 timestampNs = timer.Now();
    // Convert nanoseconds to seconds for readability
    u64 timestampSec = timestampNs / 1000000000ull;
    snprintf(buffer, bufferSize, "run_%llu_seed%llu",
             static_cast<unsigned long long>(timestampSec),
             static_cast<unsigned long long>(seed));
}

} // anonymous namespace

ExperimentRunner::ExperimentRunner(ExperimentRegistry& registry) noexcept
    : _registry(&registry),
      _measurementSystems{},
      _measurementSystemCount(0),
      _metricCollector(nullptr)
{
    _metricCollector = new MetricCollector();
}

ExperimentRunner::~ExperimentRunner() noexcept {
    delete _metricCollector;
}

void ExperimentRunner::AttachEventSink(IEventSink* sink) noexcept {
    _eventSink = sink;
}

void ExperimentRunner::RegisterMeasurementSystem(IMeasurementSystem* system) noexcept {
    if (system == nullptr || _measurementSystemCount >= kMaxMeasurementSystems) {
        return;
    }
    _measurementSystems[_measurementSystemCount++] = system;
}

void ExperimentRunner::EnableMeasurementSystem(const char* systemName, bool enabled) noexcept {
    if (systemName == nullptr) return;

    for (u32 i = 0; i < _measurementSystemCount; ++i) {
        IMeasurementSystem* system = _measurementSystems[i];
        if (system != nullptr && StringsEqual(system->Name(), systemName)) {
            system->SetEnabled(enabled);
            return;
        }
    }
}

void ExperimentRunner::NotifyRunStart(const char* experimentName, u64 seed, u32 repetitions) noexcept {
    for (u32 i = 0; i < _measurementSystemCount; ++i) {
        if (_measurementSystems[i] != nullptr) {
            _measurementSystems[i]->OnRunStart(experimentName, seed, repetitions);
        }
    }
}

void ExperimentRunner::NotifyRunEnd() noexcept {
    for (u32 i = 0; i < _measurementSystemCount; ++i) {
        if (_measurementSystems[i] != nullptr) {
            _measurementSystems[i]->OnRunEnd();
        }
    }
}

void ExperimentRunner::PublishAllMetrics() noexcept {
    if (_metricCollector == nullptr) return;

    _metricCollector->Clear();

    for (u32 i = 0; i < _measurementSystemCount; ++i) {
        if (_measurementSystems[i] != nullptr) {
            _measurementSystems[i]->PublishMetrics(*_metricCollector);
        }
    }
}

ExitCode ExperimentRunner::Run(const RunConfig& config) noexcept {
    static constexpr u32 kMaxFilteredExperiments = 256;
    const ExperimentDescriptor* experiments[kMaxFilteredExperiments];
    u32 experimentCount = 0;

    if (config.filter != nullptr) {
        experimentCount = _registry->Filter(config.filter, experiments, kMaxFilteredExperiments);
    } else {
        const ExperimentDescriptor* allExperiments = _registry->GetAll(experimentCount);
        if (experimentCount > kMaxFilteredExperiments) {
            experimentCount = kMaxFilteredExperiments;
        }
        for (u32 i = 0; i < experimentCount; ++i) {
            experiments[i] = &allExperiments[i];
        }
    }

    if (experimentCount == 0) {
        return kNoExperiments;
    }

    // Setup OutputManager if structured outputs enabled
    OutputManager* outputManager = nullptr;
    static char runIdBuffer[128];

    if (config.enableTimeSeriesOutput || config.enableSummaryOutput) {
        GenerateRunId(runIdBuffer, sizeof(runIdBuffer), config.seed);

        OutputConfig outputConfig;
        outputConfig.enableTextOutput = config.enableTextOutput;
        outputConfig.enableTimeSeriesOutput = config.enableTimeSeriesOutput;
        outputConfig.enableSummaryOutput = config.enableSummaryOutput;
        outputConfig.outputPath = config.outputPath;
        outputConfig.verbose = config.verbose;

        outputManager = new OutputManager(outputConfig);

        // Initialize with first experiment metadata (will be updated per experiment)
        HighResTimer timer;
        RunMetadata metadata;
        metadata.runId = runIdBuffer;
        metadata.experimentName = experiments[0]->name;
        metadata.experimentCategory = experiments[0]->category;
        metadata.allocatorName = experiments[0]->allocatorName;
        metadata.seed = config.seed;
        metadata.warmupIterations = config.warmupIterations;
        metadata.measuredRepetitions = config.measuredRepetitions;
        metadata.filter = config.filter;
        metadata.startTimestampNs = timer.Now();

        // Collect environment and build metadata
        CollectEnvironmentMetadata(metadata);

        if (!outputManager->Initialize(metadata)) {
            if (config.verbose) {
                printf("[ExperimentRunner] Failed to initialize OutputManager\n");
            }
            delete outputManager;
            outputManager = nullptr;
        }
    }

    ExperimentParams params;
    params.seed = config.seed;
    params.warmupIterations = config.warmupIterations;
    params.measuredRepetitions = config.measuredRepetitions;

    u32 successCount = 0;
    u32 failureCount = 0;

    for (u32 i = 0; i < experimentCount; ++i) {
        const ExperimentDescriptor* desc = experiments[i];
        if (desc == nullptr || desc->factory == nullptr) {
            ++failureCount;
            continue;
        }

        IExperiment* experiment = desc->factory();
        if (experiment == nullptr) {
            ++failureCount;
            continue;
        }

        // Attach event sink to experiment
        if (_eventSink != nullptr) {
            experiment->AttachEventSink(_eventSink);
        }

        // Attach OutputManager if enabled
        if (outputManager != nullptr) {
            experiment->AttachEventSink(outputManager);
        }

        // Attach measurement systems as event sinks
        for (u32 j = 0; j < _measurementSystemCount; ++j) {
            if (_measurementSystems[j] != nullptr) {
                experiment->AttachEventSink(_measurementSystems[j]);
            }
        }

        // Notify measurement systems: run start
        NotifyRunStart(desc->name, config.seed, config.measuredRepetitions);

        // Emit ExperimentBegin event
        if (_eventSink != nullptr) {
            Event event;
            event.type = EventType::ExperimentBegin;
            event.experimentName = desc->name;
            event.repetitionId = i;
            event.timestamp = 0;
            _eventSink->OnEvent(event);
        }

        bool result = RunExperiment(experiment, params);

        // Validate minimum repetitions (timing protocol requirement)
        if (result && params.measuredRepetitions < config.minRepetitions) {
            _currentRunStatus = RunStatus::Invalid;
            _currentFailureClass = FailureClass::InsufficientRepetitions;
            result = false;
            
            if (config.verbose) {
                printf("[ExperimentRunner] Insufficient repetitions: %u < %u (min required)\n",
                       params.measuredRepetitions, config.minRepetitions);
            }
        }

        // Emit ExperimentEnd event
        if (_eventSink != nullptr) {
            Event event;
            event.type = EventType::ExperimentEnd;
            event.experimentName = desc->name;
            event.repetitionId = i;
            event.timestamp = 0;
            _eventSink->OnEvent(event);
        }

        // Notify measurement systems: run end
        NotifyRunEnd();

        // Publish metrics
        PublishAllMetrics();

        // Update metadata with status before finalization
        if (outputManager != nullptr) {
            RunMetadata updatedMetadata;
            updatedMetadata.runId = runIdBuffer;
            updatedMetadata.experimentName = desc->name;
            updatedMetadata.experimentCategory = desc->category;
            updatedMetadata.allocatorName = desc->allocatorName;
            updatedMetadata.seed = config.seed;
            updatedMetadata.warmupIterations = config.warmupIterations;
            updatedMetadata.measuredRepetitions = config.measuredRepetitions;
            updatedMetadata.filter = config.filter;
            updatedMetadata.status = _currentRunStatus;
            updatedMetadata.failureClass = _currentFailureClass;
            outputManager->SetMetadata(updatedMetadata);
        }

        if (result) {
            ++successCount;
        } else {
            ++failureCount;
        }

        delete experiment;
    }

    if (outputManager != nullptr) {
        if (_metricCollector != nullptr) {
            outputManager->Finalize(*_metricCollector);
        }
        delete outputManager;
    }

    if (failureCount == 0) {
        return kSuccess;
    } else if (successCount == 0) {
        return kFailure;
    } else {
        return kPartialFailure;
    }
}

bool ExperimentRunner::RunExperiment(IExperiment* experiment, const ExperimentParams& params) noexcept {
    if (experiment == nullptr) {
        return false;
    }

    // Reset status for this experiment
    _currentRunStatus = RunStatus::Valid;
    _currentFailureClass = FailureClass::None;

    bool success = true;

    try {
        // Setup
        experiment->Setup(params);

        try {
            for (u32 i = 0; i < params.warmupIterations; ++i) {
                experiment->Warmup();
            }

            for (u32 i = 0; i < params.measuredRepetitions; ++i) {
                experiment->RunPhases();
            }
        } catch (...) {
            _currentRunStatus = RunStatus::Invalid;
            _currentFailureClass = FailureClass::RuntimeError;
            success = false;
        }

        experiment->Teardown();

    } catch (...) {
        _currentRunStatus = RunStatus::Invalid;
        _currentFailureClass = FailureClass::RuntimeError;
        success = false;

        experiment->Teardown();
    }

    return success;
}

} // namespace bench
} // namespace core
