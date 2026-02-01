// =============================================================================
// simple_alloc_experiment.cpp
// Implementation of SimpleAllocExperiment for allocation benchmarks
// =============================================================================

#include "simple_alloc_experiment.hpp"

#include <memory> // Ensure make_unique is available

#include "../runner/experiment_params.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/phase_executor.hpp"
#include "../common/seeded_rng.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/base/core_assert.hpp"

namespace core::bench {

namespace {

void RunPhaseOnce(
    IAllocator* allocator,
    IEventSink* eventSink,
    const char* phaseName,
    const char* experimentName,
    PhaseType phaseType,
    u32 repetitionId,
    const WorkloadParams& params,
    ReclaimMode reclaimMode,
    LifetimeTracker* externalTracker = nullptr,
    ReclaimCallback reclaimCallback = nullptr,
    PhaseOperationCallback customOperation = nullptr,
    PhaseCompletionCallback completionCheck = nullptr,
    void* userData = nullptr) noexcept {
    ASSERT(allocator != nullptr);
    ASSERT(eventSink != nullptr);

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
    ctx.eventSink = eventSink;

    ctx.phaseName = phaseName;
    ctx.experimentName = experimentName;
    ctx.phaseType = phaseType;
    ctx.repetitionId = repetitionId;
    ctx.userData = userData;

    ctx.externalLifetimeTracker = externalTracker;

    PhaseExecutor exec(desc, ctx, eventSink);
    exec.Execute();
}

WorkloadParams MakeBaseParams(u32 seed) noexcept {
    WorkloadParams p{};
    p.seed = seed;
    p.sizeDistribution = SizePresets::SmallObjects();
    p.alignmentDistribution = AlignmentPresets::Default();
    p.lifetimeModel = LifetimeModel::Fifo;
    p.maxLiveObjects = 0;
    p.operationCount = 0;
    p.allocFreeRatio = 0.0f;
    p.tickInterval = 0;
    return p;
}

} // namespace

IExperiment* SimpleAllocExperiment::Create() noexcept {
    return new SimpleAllocExperiment();
}

const char* SimpleAllocExperiment::Name() const noexcept { return "SimpleAlloc"; }
const char* SimpleAllocExperiment::Category() const noexcept { return "Allocation"; }

const char* SimpleAllocExperiment::Description() const noexcept {
    return "A simple allocation/free experiment with phase-based workload model.";
}

const char* SimpleAllocExperiment::AllocatorName() const noexcept { return "DefaultAllocator"; }

void SimpleAllocExperiment::Setup(const ExperimentParams& params) {
    _allocator = &core::GetDefaultAllocator();
    ASSERT(_allocator != nullptr);

    _seed = params.seed;
    _rng = core::bench::SeededRNG(_seed);

    _params = MakeBaseParams(_seed);
    _phaseCtx = {};
    _phaseCtx.allocator = _allocator;
    _phaseCtx.rng = &_rng;
    _phaseCtx.eventSink = _eventSink;
    _phaseCtx.experimentName = Name();

    _warmupIterations = params.warmupIterations;
}

void SimpleAllocExperiment::Warmup() {
    ASSERT(_allocator != nullptr);

    if (_warmupIterations == 0) {
        return; // Skip Warmup if iterations are zero
    }

    for (u32 i = 0; i < _warmupIterations; ++i) {
        WorkloadParams warm = MakeBaseParams(_seed + 1 + i);
        warm.operationCount = 128;
        warm.maxLiveObjects = warm.operationCount; // Ensure FIFO does not overflow
        warm.allocFreeRatio = 1.0f;
        warm.tickInterval = 0;

        RunPhaseOnce(
            _allocator,
            _eventSink,
            "Warmup",
            Name(),
            PhaseType::RampUp,
            /*repetitionId=*/0,
            warm,
            ReclaimMode::FreeAll);
    }
}

void SimpleAllocExperiment::RunPhases() {
    ASSERT(_allocator != nullptr);

    WorkloadParams ramp = MakeBaseParams(_seed);
    ramp.operationCount = 10000;
    ramp.lifetimeModel = LifetimeModel::Fifo;
    ramp.maxLiveObjects = 10000;
    ramp.allocFreeRatio = 1.0f;
    ramp.tickInterval = 0;

    WorkloadParams steady = MakeBaseParams(_seed + 100);
    steady.operationCount = 20000;
    steady.lifetimeModel = LifetimeModel::Fifo;
    steady.maxLiveObjects = 20000; // Allow headroom for allocs
    steady.allocFreeRatio = 0.5f;
    steady.tickInterval = 1000;

    SeededRNG sharedRng(_seed + 100);
    const u32 sharedCapacity = steady.maxLiveObjects;

    auto sharedTracker = std::make_unique<LifetimeTracker>(
        sharedCapacity,
        LifetimeModel::Fifo,
        sharedRng,
        _allocator);
    sharedTracker->Clear();

    RunPhaseOnce(
        _allocator,
        _eventSink,
        "RampUp",
        Name(),
        PhaseType::RampUp,
        /*repetitionId=*/0,
        ramp,
        ReclaimMode::None,
        /*externalTracker=*/sharedTracker.get());

    RunPhaseOnce(
        _allocator,
        _eventSink,
        "Steady",
        Name(),
        PhaseType::Steady,
        /*repetitionId=*/0,
        steady,
        ReclaimMode::None,
        /*externalTracker=*/sharedTracker.get());

    WorkloadParams bulk = MakeBaseParams(_seed + 200);
    bulk.operationCount = 0;
    bulk.tickInterval = 0;

    RunPhaseOnce(
        _allocator,
        _eventSink,
        "BulkReclaim",
        Name(),
        PhaseType::BulkReclaim,
        /*repetitionId=*/0,
        bulk,
        ReclaimMode::FreeAll,
        /*externalTracker=*/sharedTracker.get());
}

void SimpleAllocExperiment::Teardown() noexcept {
    if (_resetCallback) {
        _resetCallback(_resetUserData);
    }

    _allocator = nullptr;
    _allocatorOverride = nullptr;
    _eventSink = nullptr;

    _seed = 0;
    _params = {};
    _phaseCtx = {};
    _phaseDesc = {};

    _resetCallback = nullptr;
    _resetUserData = nullptr;
}

} // namespace core::bench

extern "C" core::bench::IExperiment* CreateSimpleAllocExperiment() {
    return new core::bench::SimpleAllocExperiment();
}
