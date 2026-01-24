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

IExperiment* SimpleAllocExperiment::Create() noexcept {
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
    // Optional warmup phase: run a short phase with minimal operations to prime allocator and caches
    _params.seed = _seed + 1; // Use a different seed for warmup to avoid affecting measured run
    _params.operationCount = 128; // Small number of operations for warmup
    _params.sizeDistribution = core::bench::SizePresets::SmallObjects();
    _params.alignmentDistribution = core::bench::AlignmentPresets::Default();
    _params.lifetimeModel = core::bench::LifetimeModel::Fifo;
    _params.maxLiveObjects = 16;
    _params.allocFreeRatio = 1.0f;
    _params.tickInterval = 0;

    _phaseDesc = {};
    _phaseDesc.name = "Warmup";
    _phaseDesc.experimentName = Name();
    _phaseDesc.type = PhaseType::RampUp;
    _phaseDesc.repetitionId = 0;
    _phaseDesc.params = _params;
    _phaseDesc.reclaimMode = ReclaimMode::FreeAll;
    _phaseDesc.reclaimCallback = nullptr;
    _phaseDesc.customOperation = nullptr;
    _phaseDesc.completionCheck = nullptr;
    _phaseDesc.userData = nullptr;

    _phaseCtx = {};
    _phaseCtx.allocator = _allocator;
    _phaseCtx.rng = nullptr;
    _phaseCtx.eventSink = _eventSink;
    _phaseCtx.phaseName = _phaseDesc.name;
    _phaseCtx.experimentName = _phaseDesc.experimentName;
    _phaseCtx.phaseType = _phaseDesc.type;
    _phaseCtx.repetitionId = _phaseDesc.repetitionId;
    _phaseCtx.userData = nullptr;

    if (_phaseExecutor) {
        delete _phaseExecutor;
        _phaseExecutor = nullptr;
    }
    _phaseExecutor = new PhaseExecutor(_phaseDesc, _phaseCtx, _eventSink);
    _phaseExecutor->Execute();
    delete _phaseExecutor;
    _phaseExecutor = nullptr;
}

void SimpleAllocExperiment::RunPhases() {
    // Phase 1: RampUp with alloc-only operations
    _params.seed = _seed;
    _params.operationCount = 10000; // Example: 10k allocations
    _params.sizeDistribution = core::bench::SizePresets::SmallObjects();
    _params.alignmentDistribution = core::bench::AlignmentPresets::Default();
    _params.lifetimeModel = core::bench::LifetimeModel::Fifo;
    _params.maxLiveObjects = 10000; // All allocations live until end
    _params.allocFreeRatio = 1.0f; // Alloc-only
    _params.tickInterval = 0;

    _phaseDesc = {};
    _phaseDesc.name = "RampUp";
    _phaseDesc.experimentName = Name();
    _phaseDesc.type = PhaseType::RampUp;
    _phaseDesc.repetitionId = 0;
    _phaseDesc.params = _params;
    _phaseDesc.reclaimMode = ReclaimMode::None;
    _phaseDesc.reclaimCallback = nullptr;
    _phaseDesc.customOperation = nullptr;
    _phaseDesc.completionCheck = nullptr;
    _phaseDesc.userData = nullptr;

    _phaseCtx = {};
    _phaseCtx.allocator = _allocator;
    _phaseCtx.rng = nullptr;
    _phaseCtx.eventSink = _eventSink;
    _phaseCtx.phaseName = _phaseDesc.name;
    _phaseCtx.experimentName = _phaseDesc.experimentName;
    _phaseCtx.phaseType = _phaseDesc.type;
    _phaseCtx.repetitionId = _phaseDesc.repetitionId;
    _phaseCtx.userData = nullptr;

    if (_phaseExecutor) {
        delete _phaseExecutor;
        _phaseExecutor = nullptr;
    }
    _phaseExecutor = new PhaseExecutor(_phaseDesc, _phaseCtx, _eventSink);
    _phaseExecutor->Execute();
    delete _phaseExecutor;
    _phaseExecutor = nullptr;

    // Phase 2: Steady with mixed alloc/free and bounded live-set
    _params.seed = _seed + 100;
    _params.operationCount = 20000; // Example: 20k operations
    _params.sizeDistribution = core::bench::SizePresets::SmallObjects();
    _params.alignmentDistribution = core::bench::AlignmentPresets::Default();
    _params.lifetimeModel = core::bench::LifetimeModel::Bounded;
    _params.maxLiveObjects = 1000; // Bounded live-set
    _params.allocFreeRatio = 0.5f; // 50% alloc, 50% free
    _params.tickInterval = 1000; // Emit tick every 1000 ops (optional)

    _phaseDesc = {};
    _phaseDesc.name = "Steady";
    _phaseDesc.experimentName = Name();
    _phaseDesc.type = PhaseType::Steady;
    _phaseDesc.repetitionId = 0;
    _phaseDesc.params = _params;
    _phaseDesc.reclaimMode = ReclaimMode::None;
    _phaseDesc.reclaimCallback = nullptr;
    _phaseDesc.customOperation = nullptr;
    _phaseDesc.completionCheck = nullptr;
    _phaseDesc.userData = nullptr;

    _phaseCtx = {};
    _phaseCtx.allocator = _allocator;
    _phaseCtx.rng = nullptr;
    _phaseCtx.eventSink = _eventSink;
    _phaseCtx.phaseName = _phaseDesc.name;
    _phaseCtx.experimentName = _phaseDesc.experimentName;
    _phaseCtx.phaseType = _phaseDesc.type;
    _phaseCtx.repetitionId = _phaseDesc.repetitionId;
    _phaseCtx.userData = nullptr;

    if (_phaseExecutor) {
        delete _phaseExecutor;
        _phaseExecutor = nullptr;
    }
    _phaseExecutor = new PhaseExecutor(_phaseDesc, _phaseCtx, _eventSink);
    _phaseExecutor->Execute();
    delete _phaseExecutor;
    _phaseExecutor = nullptr;
}

void SimpleAllocExperiment::Teardown() noexcept {

}

} // namespace bench
} // namespace core
