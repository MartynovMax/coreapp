// =============================================================================
// alloc_bench_experiment.cpp
// Parametric experiment for Article 1 matrix.
// =============================================================================

#include "alloc_bench_experiment.hpp"

#include <new>

#include "../runner/experiment_params.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/phase_executor.hpp"
#include "../workload/lifetime_tracker.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/base/core_assert.hpp"

namespace core::bench {

// ============================================================================
// Internal helpers (anonymous namespace)
// ============================================================================

namespace {

// Default size classes for SegregatedListAllocator
static const core::SizeClassConfig kSegSizeClasses[] = {
    {  16, 2048 },
    {  32, 2048 },
    {  64, 2048 },
    { 128, 2048 },
    { 256, 2048 },
    { 512, 2048 },
};
static constexpr u32 kSegClassCount = 6;

bool ImmediateCompletion(const PhaseContext& /*ctx*/) noexcept {
    return true;
}

void NoOpOperation(PhaseContext& /*ctx*/, const Operation& /*op*/) noexcept {
    // No-op: BulkReclaim phase has zero operationCount
}

// Execute one phase of the experiment
void RunPhaseOnce(
    IAllocator*             allocator,
    IEventSink*             eventSink,
    const char*             phaseName,
    const char*             experimentName,
    PhaseType               phaseType,
    u32                     repetitionId,
    const WorkloadParams&   params,
    ReclaimMode             reclaimMode,
    LifetimeTracker*        externalTracker     = nullptr,
    ReclaimCallback         reclaimCallback     = nullptr,
    PhaseOperationCallback  customOperation     = nullptr,
    PhaseCompletionCallback completionCheck     = nullptr,
    void*                   userData            = nullptr) noexcept
{
    ASSERT(allocator != nullptr);

    PhaseDescriptor desc{};
    desc.name           = phaseName;
    desc.experimentName = experimentName;
    desc.type           = phaseType;
    desc.repetitionId   = repetitionId;
    desc.params         = params;
    desc.reclaimMode    = reclaimMode;
    desc.reclaimCallback  = reclaimCallback;
    desc.customOperation  = customOperation;
    desc.completionCheck  = completionCheck;
    desc.userData         = userData;

    SeededRNG rng(params.seed);
    PhaseContext ctx{};
    ctx.allocator      = allocator;
    ctx.callbackRng    = &rng;
    ctx.eventSink      = eventSink;
    ctx.phaseName      = phaseName;
    ctx.experimentName = experimentName;
    ctx.phaseType      = phaseType;
    ctx.repetitionId   = repetitionId;
    ctx.userData       = userData;
    ctx.liveSetTracker = externalTracker;

    PhaseExecutor exec(desc, ctx, eventSink);
    exec.Execute();
}

// Compute arena backing capacity for MonotonicArena scenarios.
// All operations are allocs (ratio forced to 1.0), so total bytes =
// (rampUpOps + steadyOps) * avgSize * 1.25 safety factor.
core::memory_size ComputeArenaCapacity(const AllocBenchConfig& cfg) noexcept {
    const u64 avgSize     = ((u64)cfg.sizeMin + cfg.sizeMax) / 2;
    const u64 totalAllocs = (u64)cfg.maxLiveObjects + cfg.operationCount;
    const u64 bytes       = totalAllocs * avgSize * 5 / 4; // 1.25x safety
    const u64 minimum     = 1024 * 1024; // at least 1 MB
    return static_cast<core::memory_size>(bytes < minimum ? minimum : bytes);
}

} // namespace

// ============================================================================
// AllocBenchExperiment
// ============================================================================

AllocBenchExperiment::AllocBenchExperiment(const AllocBenchConfig& config) noexcept
    : _config(config)
{}

IExperiment* AllocBenchExperiment::Create(const AllocBenchConfig& config) noexcept {
    return new AllocBenchExperiment(config);
}

// Metadata -------------------------------------------------------------------

const char* AllocBenchExperiment::Name() const noexcept {
    return _config.scenarioName ? _config.scenarioName : "alloc_bench";
}

const char* AllocBenchExperiment::Category() const noexcept {
    return "article1";
}

const char* AllocBenchExperiment::Description() const noexcept {
    return "Article 1 matrix scenario: allocator x lifetime x workload";
}

const char* AllocBenchExperiment::AllocatorName() const noexcept {
    switch (_config.allocatorType) {
        case AllocatorType::Malloc:         return "malloc";
        case AllocatorType::MonotonicArena: return "monotonic_arena";
        case AllocatorType::Pool:           return "pool_allocator";
        case AllocatorType::SegregatedList: return "segregated_list";
    }
    return "unknown";
}

// Setup / Teardown helpers ---------------------------------------------------

void AllocBenchExperiment::SetupAllocator() noexcept {
    IAllocator& bk = core::GetDefaultAllocator();

    switch (_config.allocatorType) {

        case AllocatorType::Malloc:
            _allocator = &core::GetDefaultAllocator();
            break;

        case AllocatorType::MonotonicArena: {
            const core::memory_size cap = ComputeArenaCapacity(_config);
            _arena        = new core::BumpArena(cap, bk);
            _arenaAdapter = new core::ArenaAllocatorAdapter(*_arena);
            _allocator    = _arenaAdapter;
            break;
        }

        case AllocatorType::Pool: {
            const u32 blockSize  = _config.poolBlockSize  ? _config.poolBlockSize  : _config.sizeMax;
            const u32 blockCount = _config.poolBlockCount ? _config.poolBlockCount : _config.maxLiveObjects + 512;
            _pool      = new core::PoolAllocator(blockSize, blockCount, bk);
            _allocator = _pool;
            break;
        }

        case AllocatorType::SegregatedList:
            _segregated = new core::SegregatedListAllocator(kSegSizeClasses, kSegClassCount, bk, bk);
            _allocator  = _segregated;
            break;
    }

    ASSERT(_allocator != nullptr);
}

void AllocBenchExperiment::TeardownAllocator() noexcept {
    // Destroy in reverse construction order; each owns its backing buffer.
    delete _segregated;  _segregated  = nullptr;
    delete _pool;        _pool        = nullptr;
    delete _arenaAdapter; _arenaAdapter = nullptr;
    delete _arena;       _arena       = nullptr;
    _allocator = nullptr;
}

// Lifecycle ------------------------------------------------------------------

void AllocBenchExperiment::Setup(const ExperimentParams& params) {
    _seed             = params.seed;
    _rng              = SeededRNG(_seed);
    _trackerRng       = SeededRNG(_seed + 100);
    _warmupIterations = params.warmupIterations;

    SetupAllocator();

    const bool _isLongLivedOrArena =
        (_config.lifetime == LifetimeModel::LongLived) ||
        (_config.allocatorType == AllocatorType::MonotonicArena);
    const u64 _totalAllocCapacity = (u64)_config.maxLiveObjects + _config.operationCount + 512;
    const u32 trackerCapacity = _isLongLivedOrArena
        ? static_cast<u32>(_totalAllocCapacity > 0x7FFFFFFFu ? 0x7FFFFFFFu : _totalAllocCapacity)
        : _config.maxLiveObjects + (_config.maxLiveObjects / 2) + 512;
    _trackerMem = core::GetDefaultAllocator().Allocate(core::AllocationRequest{
        .size      = sizeof(LifetimeTracker),
        .alignment = static_cast<core::memory_alignment>(alignof(LifetimeTracker)),
    });
    if (!_trackerMem) { FATAL("AllocBenchExperiment: failed to allocate LifetimeTracker"); }

    _sharedTracker = new(_trackerMem) LifetimeTracker(
        trackerCapacity,
        _config.lifetime,
        _trackerRng,
        _allocator,
        &core::GetDefaultAllocator());

    if (!_sharedTracker->isValid()) { FATAL("AllocBenchExperiment: LifetimeTracker init failed"); }
}

void AllocBenchExperiment::Warmup() {
    ASSERT(_allocator != nullptr);
    if (_warmupIterations == 0) return;

    for (u32 i = 0; i < _warmupIterations; ++i) {
        WorkloadParams warm{};
        warm.seed              = _seed + 1 + i;
        warm.operationCount    = 128;
        warm.maxLiveObjects    = 128;
        warm.allocFreeRatio    = 1.0f;
        warm.sizeDistribution  = SizeDistribution{
            .type    = DistributionType::Uniform,
            .minSize = _config.sizeMin,
            .maxSize = _config.sizeMax,
        };
        warm.alignmentDistribution = AlignmentDistribution{};
        warm.lifetimeModel  = LifetimeModel::Fifo;
        warm.tickInterval   = 0;

        RunPhaseOnce(_allocator, _eventSink, "Warmup", Name(),
                     PhaseType::RampUp, 0, warm, ReclaimMode::FreeAll);
    }
}

void AllocBenchExperiment::RunPhases(u32 repetitionIndex) {
    ASSERT(_allocator != nullptr);
    ASSERT(_sharedTracker != nullptr && _sharedTracker->isValid());

    const bool isArena    = (_config.allocatorType == AllocatorType::MonotonicArena);
    const bool isLongLived = (_config.lifetime == LifetimeModel::LongLived);
    const f32  steadyRatio = (isArena || isLongLived) ? 1.0f : _config.allocFreeRatio;

    // --- Phase 1: RampUp — fill the live set ---
    WorkloadParams ramp{};
    ramp.seed           = _seed;
    ramp.operationCount = _config.maxLiveObjects;
    ramp.maxLiveObjects = _config.maxLiveObjects;
    ramp.allocFreeRatio = 1.0f; // alloc-only during ramp
    ramp.lifetimeModel  = _config.lifetime;
    ramp.sizeDistribution = SizeDistribution{
        .type    = DistributionType::Uniform,
        .minSize = _config.sizeMin,
        .maxSize = _config.sizeMax,
    };
    ramp.alignmentDistribution = AlignmentDistribution{};
    ramp.tickInterval = 0;

    // --- Phase 2: Steady — main measurement phase ---
    WorkloadParams steady{};
    steady.seed           = _seed + 100;
    steady.operationCount = _config.operationCount;
    steady.maxLiveObjects = (isLongLived || isArena) ? 0u : _config.maxLiveObjects;
    steady.allocFreeRatio = steadyRatio;
    steady.lifetimeModel  = _config.lifetime;
    steady.sizeDistribution = SizeDistribution{
        .type    = DistributionType::Uniform,
        .minSize = _config.sizeMin,
        .maxSize = _config.sizeMax,
    };
    steady.alignmentDistribution = AlignmentDistribution{};
    steady.tickInterval = _config.operationCount / 10; // 10 tick points

    // --- Phase 3: BulkReclaim ---
    WorkloadParams bulk{};
    bulk.seed           = _seed + 200;
    bulk.operationCount = 0; // do-while mode; completionCheck fires immediately
    bulk.tickInterval   = 0;

    _sharedTracker->Clear();

    RunPhaseOnce(_allocator, _eventSink, "RampUp", Name(),
                 PhaseType::RampUp, repetitionIndex,
                 ramp, ReclaimMode::None, _sharedTracker);

    RunPhaseOnce(_allocator, _eventSink, "Steady", Name(),
                 PhaseType::Steady, repetitionIndex,
                 steady, ReclaimMode::None, _sharedTracker);

    if (isArena) {
        // Arena reclaim: capture metrics, clear tracker, then Reset()
        RunPhaseOnce(_allocator, _eventSink, "BulkReclaim", Name(),
                     PhaseType::BulkReclaim, repetitionIndex,
                     bulk, ReclaimMode::Custom,
                     /*externalTracker=*/nullptr,
                     ArenaReclaimCallback,
                     NoOpOperation,
                     ImmediateCompletion,
                     /*userData=*/this);
    } else {
        // Standard reclaim: FreeAll via LifetimeTracker
        RunPhaseOnce(_allocator, _eventSink, "BulkReclaim", Name(),
                     PhaseType::BulkReclaim, repetitionIndex,
                     bulk, ReclaimMode::Custom,
                     /*externalTracker=*/nullptr,
                     StandardReclaimCallback,
                     NoOpOperation,
                     ImmediateCompletion,
                      /*userData=*/_sharedTracker);
    }
}

void AllocBenchExperiment::Teardown() noexcept {
    // Destroy tracker first (it may reference _allocator for FreeAll)
    if (_sharedTracker) {
        _sharedTracker->~LifetimeTracker();
        _sharedTracker = nullptr;
    }
    if (_trackerMem) {
        core::GetDefaultAllocator().Deallocate(core::AllocationInfo{
            .ptr       = _trackerMem,
            .size      = sizeof(LifetimeTracker),
            .alignment = static_cast<core::memory_alignment>(alignof(LifetimeTracker)),
        });
        _trackerMem = nullptr;
    }

    TeardownAllocator();

    _eventSink  = nullptr;
    _seed       = 0;
    _rng        = SeededRNG{0};
    _trackerRng = SeededRNG{0};
    _warmupIterations = 0;
}

// Reclaim callbacks ----------------------------------------------------------

void AllocBenchExperiment::ArenaReclaimCallback(PhaseContext& ctx) noexcept {
    auto* self = static_cast<AllocBenchExperiment*>(ctx.userData);
    ASSERT(self != nullptr);
    ASSERT(self->_arena != nullptr);

    // Capture live-set metrics before clearing
    if (self->_sharedTracker) {
        ctx.reclaimFreeCount  += self->_sharedTracker->GetLiveCount();
        ctx.reclaimBytesFreed += self->_sharedTracker->GetLiveBytes();
        // Clear tracker WITHOUT calling Deallocate (no-op for arena anyway)
        self->_sharedTracker->Clear();
    }

    // Actually free all arena memory in one shot
    self->_arena->Reset();
}

void AllocBenchExperiment::StandardReclaimCallback(PhaseContext& ctx) noexcept {
    auto* tracker = static_cast<LifetimeTracker*>(ctx.userData);
    ASSERT(tracker != nullptr);
    ASSERT(tracker->isValid());

    u64 count = 0, bytes = 0;
    tracker->FreeAll(&count, &bytes);

    ctx.reclaimFreeCount  += count;
    ctx.reclaimBytesFreed += bytes;
}

} // namespace core::bench

