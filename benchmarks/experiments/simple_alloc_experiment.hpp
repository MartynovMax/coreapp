#pragma once

// =============================================================================
// simple_alloc_experiment.hpp
// Simple allocation experiment for benchmark framework.
// =============================================================================

#include "../workload/phase_descriptor.hpp"
#include "../workload/phase_context.hpp"
#include "../workload/phase_executor.hpp"
#include "../workload/workload_params.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_types.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// SimpleAllocExperiment - Example experiment for allocation benchmarks
// ----------------------------------------------------------------------------
class SimpleAllocExperiment {
public:
    SimpleAllocExperiment() noexcept = default;
    ~SimpleAllocExperiment() noexcept = default;

    // Metadata
    const char* Name() const noexcept;
    const char* Category() const noexcept;
    const char* Description() const noexcept;
    const char* AllocatorName() const noexcept;

    // Lifecycle
    void Setup();
    void Warmup();
    void RunPhases();
    void Teardown();

private:

};

// Factory function for registration
SimpleAllocExperiment* CreateSimpleAllocExperiment();

} // namespace bench
} // namespace core
