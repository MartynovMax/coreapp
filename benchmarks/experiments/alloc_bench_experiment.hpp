#pragma once

// =============================================================================
// alloc_bench_experiment.hpp
// Parametric experiment covering all Article 1 matrix scenarios.
// =============================================================================

#include "../runner/experiment_interface.hpp"
#include "../workload/workload_params.hpp"
#include "../common/seeded_rng.hpp"
#include "core/memory/bump_arena.hpp"
#include "core/memory/arena.hpp"
#include "core/memory/pool_allocator.hpp"
#include "core/memory/segregated_list_allocator.hpp"

namespace core::bench {

// ----------------------------------------------------------------------------
// AllocatorType — which allocator is under test
// ----------------------------------------------------------------------------

enum class AllocatorType {
    Malloc,
    MonotonicArena,
    Pool,
    SegregatedList,
};

// ----------------------------------------------------------------------------
// WorkloadProfile — one of the three matrix workload shapes
// ----------------------------------------------------------------------------

enum class WorkloadProfile {
    FixedSmall,    // 32×32 bytes, 100k ops, 1000 live, ratio 0.5
    VariableSize,  // 8–512 bytes, 100k ops, 1000 live, ratio 0.5
    Churn,         // 16–64 bytes, 200k ops, 500 live, ratio 0.6
};

// ----------------------------------------------------------------------------
// AllocBenchConfig — full description of one matrix scenario
// ----------------------------------------------------------------------------

struct AllocBenchConfig {
    const char*     scenarioName   = nullptr;  // e.g. "article1/malloc/fifo/fixed_small"
    AllocatorType   allocatorType  = AllocatorType::Malloc;
    LifetimeModel   lifetime       = LifetimeModel::Fifo;
    WorkloadProfile workload       = WorkloadProfile::FixedSmall;
    u64             operationCount = 100000;
    u32             maxLiveObjects = 1000;
    f32             allocFreeRatio = 0.5f;
    u32             sizeMin        = 32;
    u32             sizeMax        = 32;

    // PoolAllocator: 0 = derive from sizeMax / maxLiveObjects respectively
    u32 poolBlockSize  = 0;
    u32 poolBlockCount = 0;
};

// ----------------------------------------------------------------------------
// AllocBenchExperiment — parametric experiment for all Article 1 scenarios
// ----------------------------------------------------------------------------

class LifetimeTracker;

class AllocBenchExperiment final : public IExperiment {
public:
    explicit AllocBenchExperiment(const AllocBenchConfig& config) noexcept;
    ~AllocBenchExperiment() noexcept override = default;

    static IExperiment* Create(const AllocBenchConfig& config) noexcept;

    void Setup(const ExperimentParams& params) override;
    void Warmup() override;
    void RunPhases() override;
    void Teardown() noexcept override;

    [[nodiscard]] const char* Name()          const noexcept override;
    [[nodiscard]] const char* Category()      const noexcept override;
    [[nodiscard]] const char* Description()   const noexcept override;
    [[nodiscard]] const char* AllocatorName() const noexcept override;

    void AttachEventSink(IEventSink* sink) noexcept override { _eventSink = sink; }

private:
    AllocBenchConfig _config;

    core::IAllocator* _allocator = nullptr;
    IEventSink*       _eventSink = nullptr;

    u64       _seed             = 0;
    SeededRNG _rng{0};
    SeededRNG _trackerRng{0};
    u32       _warmupIterations = 0;

    // MonotonicArena storage (owned)
    core::BumpArena*             _arena        = nullptr;
    core::ArenaAllocatorAdapter* _arenaAdapter = nullptr;

    // PoolAllocator storage (owned)
    core::PoolAllocator* _pool = nullptr;

    // SegregatedListAllocator storage (owned)
    core::SegregatedListAllocator* _segregated = nullptr;

    // LifetimeTracker (placement-new into _trackerMem, allocated from default allocator)
    void*            _trackerMem    = nullptr;
    LifetimeTracker* _sharedTracker = nullptr;

    void SetupAllocator()    noexcept;
    void TeardownAllocator() noexcept;

    static void ArenaReclaimCallback(PhaseContext& ctx)    noexcept;
    static void StandardReclaimCallback(PhaseContext& ctx) noexcept;
};

} // namespace core::bench

