// =============================================================================
// simple_alloc_experiment.cpp
// Implementation of SimpleAllocExperiment for allocation benchmarks
// =============================================================================

#include "simple_alloc_experiment.hpp"

#include <new> // For placement new

#include "../runner/experiment_params.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/phase_executor.hpp"
#include "../common/seeded_rng.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/base/core_assert.hpp"
#include "../workload/lifetime_tracker.hpp"

namespace core::bench {

namespace {

// Helper completion check for operationCount=0 phases
bool ImmediateCompletion(const PhaseContext& /*ctx*/) noexcept {
    return true;  // Immediately complete
}

// Helper no-op operation for operationCount=0 phases
void NoOpOperation(PhaseContext& /*ctx*/, const Operation& /*op*/) noexcept {
    // No-op: used when phase logic is handled elsewhere (e.g., reclaim callback)
}

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
    ctx.callbackRng = &rng;
    ctx.eventSink = eventSink;

    ctx.phaseName = phaseName;
    ctx.experimentName = experimentName;
    ctx.phaseType = phaseType;
    ctx.repetitionId = repetitionId;
    ctx.userData = userData;

    // Use liveSetTracker for external tracker (source of truth for metrics)
    ctx.liveSetTracker = externalTracker;

    PhaseExecutor exec(desc, ctx, eventSink);
    exec.Execute();
}

void BulkReclaimFromUserData(PhaseContext& ctx) noexcept {
    auto* tracker = static_cast<LifetimeTracker*>(ctx.userData);
    ASSERT(tracker != nullptr);
    ASSERT(tracker->isValid());

    u64 freedCount = 0;
    u64 freedBytes = 0;
    tracker->FreeAll(&freedCount, &freedBytes);

    ctx.reclaimFreeCount += freedCount;
    ctx.reclaimBytesFreed += freedBytes;

    ASSERT(tracker->GetLiveCount() == 0);
    ASSERT(tracker->GetLiveBytes() == 0);
}

WorkloadParams MakeBaseParams(u64 seed) noexcept {
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
    _allocator = _allocatorOverride ? _allocatorOverride : &core::GetDefaultAllocator();
    ASSERT(_allocator != nullptr);

    _seed = params.seed;
    _rng = core::bench::SeededRNG(_seed);

    _params = MakeBaseParams(_seed);
    _phaseCtx = {};
    _phaseCtx.allocator = _allocator;
    _phaseCtx.callbackRng = &_rng;
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
    steady.maxLiveObjects = 20000;
    steady.allocFreeRatio = 0.5f;
    steady.tickInterval = 1000;

    SeededRNG sharedRng(_seed + 100);
    
    u32 rampMaxLive = ramp.maxLiveObjects;
    u32 steadyMaxLive = steady.maxLiveObjects;
    u32 maxLiveAcrossPhases = (rampMaxLive > steadyMaxLive) ? rampMaxLive : steadyMaxLive;
    u32 safetyMargin = maxLiveAcrossPhases / 2;
    const u32 sharedCapacity = maxLiveAcrossPhases + safetyMargin;

    void* trackerMem = _allocator->Allocate(core::AllocationRequest{
        .size = sizeof(LifetimeTracker),
        .alignment = static_cast<memory_alignment>(alignof(LifetimeTracker))
    });
    
    if (!trackerMem) {
        FATAL("Failed to allocate LifetimeTracker");
    }
    
    LifetimeTracker* sharedTracker = new (trackerMem) LifetimeTracker(
        sharedCapacity,
        LifetimeModel::Fifo,
        sharedRng,
        _allocator);
    
    if (!sharedTracker->isValid()) {
        FATAL("Failed to initialize shared LifetimeTracker - insufficient capacity or allocation failure");
    }
    
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
        /*externalTracker=*/sharedTracker);

    RunPhaseOnce(
        _allocator,
        _eventSink,
        "Steady",
        Name(),
        PhaseType::Steady,
        /*repetitionId=*/0,
        steady,
        ReclaimMode::None,
        /*externalTracker=*/sharedTracker);

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
        ReclaimMode::Custom,
        /*externalTracker=*/nullptr,
        /*reclaimCallback=*/BulkReclaimFromUserData,
        /*customOperation=*/NoOpOperation,          // Required for operationCount=0
        /*completionCheck=*/ImmediateCompletion,  // Required for operationCount=0
        /*userData=*/sharedTracker);
    
    // Manual cleanup
    sharedTracker->~LifetimeTracker();
    _allocator->Deallocate(core::AllocationInfo{
        .ptr = trackerMem,
        .size = sizeof(LifetimeTracker),
        .alignment = static_cast<memory_alignment>(alignof(LifetimeTracker))
    });
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
