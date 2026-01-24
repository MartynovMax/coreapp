// =============================================================================
// simple_alloc_experiment.cpp
// Implementation of SimpleAllocExperiment for allocation benchmarks
// =============================================================================

#include "simple_alloc_experiment.hpp"
#include "../runner/experiment_params.hpp"
#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/workload_params.hpp"
#include "../workload/phase_context.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_types.hpp"
#include "core/memory/core_allocator.hpp"
#include <cstring>

namespace core {
namespace bench {

SimpleAllocExperiment* SimpleAllocExperiment::Create() noexcept {
    return new SimpleAllocExperiment();
}

const char* SimpleAllocExperiment::Name() const noexcept {
    return "SimpleAlloc";
}

const char* SimpleAllocExperiment::Category() const noexcept {
    return "Allocation";
}

const char* SimpleAllocExperiment::Description() const noexcept {
    return "A simple allocation/free experiment with phase-based workload model.";
}

const char* SimpleAllocExperiment::AllocatorName() const noexcept {
    return "DefaultAllocator";
}

void SimpleAllocExperiment::Setup(const ExperimentParams& params) {
    _allocator = &core::GetDefaultAllocator();
    _seed = params.seed;

}

void SimpleAllocExperiment::Warmup() {

}

void SimpleAllocExperiment::RunPhases() {

}

void SimpleAllocExperiment::Teardown() noexcept {

}

} // namespace bench
} // namespace core
