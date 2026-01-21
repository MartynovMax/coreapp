#include "null_experiment.hpp"

namespace core {
namespace bench {

void NullExperiment::Setup(const ExperimentParams& params) {
    (void)params;
}

void NullExperiment::Warmup() {
}

void NullExperiment::RunPhases() {
}

void NullExperiment::Teardown() noexcept {
}

const char* NullExperiment::Name() const noexcept {
    return "null";
}

const char* NullExperiment::Category() const noexcept {
    return "test";
}

const char* NullExperiment::Description() const noexcept {
    return "No-op experiment for testing runner";
}

const char* NullExperiment::AllocatorName() const noexcept {
    return "none";
}

IExperiment* NullExperiment::Create() noexcept {
    return new NullExperiment();
}

} // namespace bench
} // namespace core
