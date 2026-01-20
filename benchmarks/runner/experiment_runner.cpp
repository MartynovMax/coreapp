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

    (void)experiment;
    (void)params;
    return true;
}

} // namespace bench
} // namespace core
