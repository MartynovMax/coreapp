#include "experiment_runner.hpp"
#include <stdio.h>

namespace core {
namespace bench {

ExperimentRunner::ExperimentRunner(ExperimentRegistry& registry) noexcept
    : _registry(&registry)
{
}

void ExperimentRunner::AttachEventSink(IEventSink* sink) noexcept {
    _eventSink = sink;
}

ExitCode ExperimentRunner::Run(const RunConfig& config) noexcept {
    static constexpr u32 kMaxFilteredExperiments = 256;
    const ExperimentDescriptor* experiments[kMaxFilteredExperiments];
    u32 experimentCount = 0;

    if (config.filter != nullptr) {
        experimentCount = _registry->Filter(config.filter, experiments, kMaxFilteredExperiments);
    } else {
        const ExperimentDescriptor* allExperiments = _registry->GetAll(experimentCount);
        for (u32 i = 0; i < experimentCount && i < kMaxFilteredExperiments; ++i) {
            experiments[i] = &allExperiments[i];
        }
    }

    if (experimentCount == 0) {
        return kNoExperiments;
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
            continue;
        }

        IExperiment* experiment = desc->factory();
        if (experiment == nullptr) {
            ++failureCount;
            continue;
        }

        if (_eventSink != nullptr) {
            experiment->AttachEventSink(_eventSink);
        }

        bool result = RunExperiment(experiment, params);
        if (result) {
            ++successCount;
        } else {
            ++failureCount;
        }

        delete experiment;
    }

    // Determine exit code
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

    bool success = true;

    try {
        // Setup
        experiment->Setup(params);

        try {
            // Warmup loop
            for (u32 i = 0; i < params.warmupIterations; ++i) {
                experiment->Warmup();
            }

            // Measured repetitions loop
            for (u32 i = 0; i < params.measuredRepetitions; ++i) {
                experiment->RunPhases();
            }
        } catch (...) {
            printf("Error: Exception in warmup/run_phases\n");
            success = false;
        }

        // Teardown (always called, even if previous stages failed)
        try {
            experiment->Teardown();
        } catch (...) {
            printf("Error: Exception in teardown\n");
            success = false;
        }

    } catch (...) {
        printf("Error: Exception in setup\n");
        success = false;

        // Still try to call teardown
        try {
            experiment->Teardown();
        } catch (...) {
            printf("Error: Exception in teardown after setup failure\n");
        }
    }

    return success;
}

} // namespace bench
} // namespace core
