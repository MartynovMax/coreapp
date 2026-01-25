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
#include "../runner/experiment_interface.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// SimpleAllocExperiment - Example experiment for allocation benchmarks
// ----------------------------------------------------------------------------
class SimpleAllocExperiment final : public IExperiment {
public:
    SimpleAllocExperiment() noexcept = default;
    ~SimpleAllocExperiment() noexcept override = default;

    void Setup(const ExperimentParams& params) override;
    void Warmup() override;
    void RunPhases() override;
    void Teardown() noexcept override;

    const char* Name() const noexcept override;
    const char* Category() const noexcept override;
    const char* Description() const noexcept override;
    const char* AllocatorName() const noexcept override;

    void AttachEventSink(IEventSink* sink) noexcept override { _eventSink = sink; }

    static IExperiment* Create() noexcept;

private:
    IAllocator* _allocator = nullptr;
    WorkloadParams _params{};
    u64 _seed = 0;
    PhaseContext _phaseCtx{};
    PhaseDescriptor _phaseDesc{};
    PhaseExecutor* _phaseExecutor = nullptr;
    IEventSink* _eventSink = nullptr;
    core::bench::SeededRNG _rng{0};
};

} // namespace bench
} // namespace core
