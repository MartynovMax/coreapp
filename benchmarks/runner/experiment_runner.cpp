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

    (void)config;
    return kSuccess;
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
