// =============================================================================
// simple_alloc_experiment.cpp
// Implementation of SimpleAllocExperiment for allocation benchmarks
// =============================================================================

#include "simple_alloc_experiment.hpp"
#include "../runner/experiment_params.hpp"
#include "../runner/experiment_registry.hpp"
#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/workload_params.hpp"
#include "../workload/phase_context.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_types.hpp"
#include "core/memory/core_allocator.hpp"
#include "../common/seeded_rng.hpp"
#include "core/base/core_assert.hpp"

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
    ASSERT(&core::GetDefaultAllocator() != nullptr);
    _allocator = &core::GetDefaultAllocator();
    ASSERT(_allocator != nullptr);
    _seed = params.seed;
}

void SimpleAllocExperiment::Warmup() {
    ASSERT(_allocator != nullptr);
    _params.seed = _seed + 1;
    _params.operationCount = 128;
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

    SeededRNG rng(_params.seed);
    _phaseCtx = {};
    _phaseCtx.allocator = _allocator;
    _phaseCtx.rng = &rng;
    ASSERT(_phaseCtx.rng != nullptr);
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
    ASSERT(_phaseExecutor != nullptr);
    _phaseExecutor->Execute();
    delete _phaseExecutor;
    _phaseExecutor = nullptr;
}

namespace {
    void RunPhase(
        PhaseExecutor*& phaseExecutor,
        IAllocator* allocator,
        IEventSink* eventSink,
        const char* phaseName,
        const char* experimentName,
        PhaseType phaseType,
        u32 repetitionId,
        const WorkloadParams& params,
        ReclaimMode reclaimMode,
        ReclaimCallback reclaimCallback = nullptr,
        PhaseOperationCallback customOperation = nullptr,
        PhaseCompletionCallback completionCheck = nullptr,
        void* userData = nullptr,
        const PhaseContext* externalCtx = nullptr)
    {
        ASSERT(allocator != nullptr);
        PhaseDescriptor desc{};
        desc.name = phaseName;
        desc.experimentName = experimentName;
        desc.type = phaseType;
        desc.repetitionId = repetitionId;
        desc.params = params;
        desc.reclaimMode = reclaimMode;
        desc.reclaimCallback = reclaimCallback;
        desc.customOperation = customOperation;
        desc.completionCheck = completionCheck;
        desc.userData = userData;

        SeededRNG rng(params.seed);
        PhaseContext ctx{};
        ctx.allocator = allocator;
        ctx.rng = &rng;
        ASSERT(ctx.rng != nullptr);
        ctx.eventSink = eventSink;
        ctx.phaseName = phaseName;
        ctx.experimentName = experimentName;
        ctx.phaseType = phaseType;
        ctx.repetitionId = repetitionId;
        ctx.userData = userData;
        if (externalCtx) {
            ctx.externalLifetimeTracker = externalCtx->externalLifetimeTracker;
        }
        if (phaseExecutor) {
            delete phaseExecutor;
            phaseExecutor = nullptr;
        }
        phaseExecutor = new PhaseExecutor(desc, ctx, eventSink);
        ASSERT(phaseExecutor != nullptr);
        phaseExecutor->Execute();
        delete phaseExecutor;
        phaseExecutor = nullptr;
    }

}

void SimpleAllocExperiment::RunPhases() {
    // --- Shared tracker for Steady and BulkReclaim ---
    LifetimeTracker* sharedTracker = nullptr;

    // Phase 1: RampUp (alloc-only)
    WorkloadParams rampUpParams = _params;
    rampUpParams.seed = _seed;
    rampUpParams.operationCount = 10000;
    rampUpParams.sizeDistribution = core::bench::SizePresets::SmallObjects();
    rampUpParams.alignmentDistribution = core::bench::AlignmentPresets::Default();
    rampUpParams.lifetimeModel = core::bench::LifetimeModel::Fifo;
    rampUpParams.maxLiveObjects = 10000;
    rampUpParams.allocFreeRatio = 1.0f;
    rampUpParams.tickInterval = 0;
    RunPhase(_phaseExecutor, _allocator, _eventSink, "RampUp", Name(), PhaseType::RampUp, 0, rampUpParams, ReclaimMode::None);

    // Phase 2: Steady (mixed alloc/free, bounded live-set)
    WorkloadParams steadyParams = _params;
    steadyParams.seed = _seed + 100;
    steadyParams.operationCount = 20000;
    steadyParams.sizeDistribution = core::bench::SizePresets::SmallObjects();
    steadyParams.alignmentDistribution = core::bench::AlignmentPresets::Default();
    steadyParams.lifetimeModel = core::bench::LifetimeModel::Bounded;
    steadyParams.maxLiveObjects = 1000;
    steadyParams.allocFreeRatio = 0.5f;
    steadyParams.tickInterval = 1000;
    sharedTracker = new LifetimeTracker(steadyParams.lifetimeModel, steadyParams.maxLiveObjects, *_phaseCtx.rng, _allocator);
    PhaseContext steadyCtx = _phaseCtx;
    steadyCtx.externalLifetimeTracker = sharedTracker;
    RunPhase(_phaseExecutor, _allocator, _eventSink, "Steady", Name(), PhaseType::Steady, 0, steadyParams, ReclaimMode::None, nullptr, nullptr, nullptr, nullptr, &steadyCtx);

    // Phase 3: BulkReclaim (FreeAll)
    WorkloadParams reclaimParams = _params;
    reclaimParams.seed = _seed + 200;
    reclaimParams.operationCount = 0;
    reclaimParams.sizeDistribution = core::bench::SizePresets::SmallObjects();
    reclaimParams.alignmentDistribution = core::bench::AlignmentPresets::Default();
    reclaimParams.lifetimeModel = core::bench::LifetimeModel::Fifo;
    reclaimParams.maxLiveObjects = 0;
    reclaimParams.allocFreeRatio = 0.0f;
    reclaimParams.tickInterval = 0;
    PhaseContext reclaimCtx = _phaseCtx;
    reclaimCtx.externalLifetimeTracker = sharedTracker;
    RunPhase(_phaseExecutor, _allocator, _eventSink, "BulkReclaim", Name(), PhaseType::BulkReclaim, 0, reclaimParams, ReclaimMode::FreeAll, nullptr, nullptr, nullptr, nullptr, &reclaimCtx);

    if (sharedTracker) {
        delete sharedTracker;
        sharedTracker = nullptr;
    }
}

void SimpleAllocExperiment::Teardown() noexcept {
    // Clean up phase executor if still allocated
    if (_phaseExecutor) {
        delete _phaseExecutor;
        _phaseExecutor = nullptr;
    }
    // Reset pointers and state
    _allocator = nullptr;
    _eventSink = nullptr;
    _seed = 0;
    _params = {};
    _phaseCtx = {};
    _phaseDesc = {};
}

} // namespace bench
} // namespace core

extern "C" core::bench::IExperiment* CreateSimpleAllocExperiment() {
    return new core::bench::SimpleAllocExperiment();
}
