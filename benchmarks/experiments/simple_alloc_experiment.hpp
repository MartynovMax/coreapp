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

class LifetimeTracker;

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

    [[nodiscard]] const char* Name() const noexcept override;
    [[nodiscard]] const char* Category() const noexcept override;
    [[nodiscard]] const char* Description() const noexcept override;
    [[nodiscard]] const char* AllocatorName() const noexcept override;

    void AttachEventSink(IEventSink* sink) noexcept override { _eventSink = sink; }

    void SetAllocatorForTests(IAllocator* allocator) noexcept { _allocatorOverride = allocator; }
    void SetResetCallbackForTests(void (*callback)(void*), void* userData) noexcept {
        _resetCallback = callback;
        _resetUserData = userData;
    }

    static IExperiment* Create() noexcept;

private:
    IAllocator* _allocator = nullptr;
    IAllocator* _allocatorOverride = nullptr;
    WorkloadParams _params{};
    u64 _seed = 0;
    PhaseContext _phaseCtx{};
    PhaseDescriptor _phaseDesc{};
    PhaseExecutor* _phaseExecutor = nullptr;
    IEventSink* _eventSink = nullptr;
    core::bench::SeededRNG _rng{0};
    void (*_resetCallback)(void*) = nullptr;
    void* _resetUserData = nullptr;

    u32 _warmupIterations = 0;
    
    // Pre-allocated shared tracker (protocol requirement: isolation)
    LifetimeTracker* _sharedTracker = nullptr;
    void* _trackerMemory = nullptr;
};

} // namespace bench
} // namespace core
