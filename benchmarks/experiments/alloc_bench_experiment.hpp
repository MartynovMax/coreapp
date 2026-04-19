#pragma once

// =============================================================================
// alloc_bench_experiment.hpp
// Parametric experiment covering all Article 1 matrix scenarios.
// =============================================================================

#include "../runner/experiment_interface.hpp"
#include "../workload/workload_params.hpp"
#include "../common/seeded_rng.hpp"
#include "../events/event_bus.hpp"
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
    // scenario_id: this field is the stable unique key used as a join key across runs.
    // Do NOT rename scenarios between runs — downstream analysis uses this name.
    // Format: "article1/{allocator}/{lifetime}/{workload}"
    const char*     scenarioName   = nullptr;  // e.g. "article1/malloc/fifo/fixed_small"
    AllocatorType   allocatorType  = AllocatorType::Malloc;
    LifetimeModel   lifetime       = LifetimeModel::Fifo;
    WorkloadProfile workload       = WorkloadProfile::FixedSmall;
    u64             operationCount = 100000;
    u32             maxLiveObjects = 1000;
    f32             allocFreeRatio = 0.5f;
    u32             sizeMin        = 32;
    u32             sizeMax        = 32;

    // Per-scenario reproducibility (0 = not set; fallback to global default or CLI override)
    u64 seed        = 0;   // Deterministic seed for this scenario
    u32 repetitions = 0;   // Number of measured repetitions for this scenario
    u32 warmup      = 0;   // Number of warmup iterations for this scenario

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
    void RunPhases(u32 repetitionIndex) override;
    void Teardown() noexcept override;

    [[nodiscard]] const char* Name()          const noexcept override;
    [[nodiscard]] const char* Category()      const noexcept override;
    [[nodiscard]] const char* Description()   const noexcept override;
    [[nodiscard]] const char* AllocatorName() const noexcept override;

    void AttachEventSink(IEventSink* sink) noexcept override {
        _eventBus.Attach(sink);
        _hasEventSinks = true;
    }

    // Returns allocator-specific reserved/capacity bytes; 0 if not available.
    [[nodiscard]] u64 QueryFootprint() const noexcept;

    // Returns segregated_list fallback allocation count since last reset; 0 for other allocators.
    [[nodiscard]] u64 QueryFallbackCount() const noexcept;

    // FootprintCallback trampoline: userData must be AllocBenchExperiment*.
    static u64 FootprintQueryCallback(void* userData) noexcept;

    // FallbackCountCallback trampoline: userData must be AllocBenchExperiment*.
    static u64 FallbackCountQueryCallback(void* userData) noexcept;

private:
    AllocBenchConfig _config;

    core::IAllocator* _allocator = nullptr;
    EventBus          _eventBus;
    bool              _hasEventSinks = false;

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

// Compute a stable 32-bit FNV-1a hash of scenario workload parameters.
// Use this to detect silent drift when scenario names stay the same but
// parameters change between runs.
inline constexpr u32 ComputeAllocBenchParamsHash(const AllocBenchConfig& cfg) noexcept {
    u32 h = 2166136261u;
    auto mix = [&](u32 v) noexcept { h = (h ^ v) * 16777619u; };
    mix(static_cast<u32>(cfg.allocatorType));
    mix(static_cast<u32>(cfg.lifetime));
    mix(static_cast<u32>(cfg.workload));
    mix(cfg.sizeMin);
    mix(cfg.sizeMax);
    mix(static_cast<u32>(cfg.operationCount & 0xFFFFFFFFu));
    mix(static_cast<u32>(cfg.operationCount >> 32));
    mix(cfg.maxLiveObjects);
    // Encode allocFreeRatio as fixed-point (4 decimal places) to avoid FP reinterpret_cast
    mix(static_cast<u32>(cfg.allocFreeRatio * 10000.0f + 0.5f));
    return h;
}

} // namespace core::bench

