// =============================================================================
// article1_registry.cpp
// Static definition of all 31 Article 1 matrix scenarios + registration.
//
// This file is the CODE-LEVEL counterpart of:
//   benchmarks/config/article1_matrix.json  (source of truth / documentation)
//   benchmarks/docs/article1_matrix.md      (human-readable reference)
//
// Keep in sync with article1_matrix.json when parameters change.
// =============================================================================

#include "article1_registry.hpp"
#include "alloc_bench_experiment.hpp"
#include "../runner/experiment_registry.hpp"
#include "../runner/experiment_descriptor.hpp"

namespace core::bench {

// ============================================================================
// Workload profile parameters (must match article1_matrix.json)
// ============================================================================

namespace profiles {

struct Params {
    u32  sizeMin;
    u32  sizeMax;
    u64  operationCount;
    u32  maxLiveObjects;
    f32  allocFreeRatio;
};

static constexpr Params kFixedSmall   = {  32,  32, 100000, 1000, 0.5f };
static constexpr Params kVariableSize = {   8, 512, 100000, 1000, 0.5f };
static constexpr Params kChurn        = {  16,  64, 200000,  500, 0.6f };

} // namespace profiles

// ============================================================================
// Static matrix definition — 31 entries, ordered by allocator group
// ============================================================================

// Helper macro to build AllocBenchConfig from a profile struct
#define SCENARIO(sname, atype, life, prof) \
    AllocBenchConfig { \
        (sname), \
        AllocatorType::atype, \
        LifetimeModel::life, \
        WorkloadProfile::prof, \
        profiles::k##prof.operationCount, \
        profiles::k##prof.maxLiveObjects, \
        profiles::k##prof.allocFreeRatio, \
        profiles::k##prof.sizeMin, \
        profiles::k##prof.sizeMax, \
        0, 0 \
    }

static const AllocBenchConfig kArticle1Matrix[] = {

    // ------------------------------------------------------------------
    // malloc (baseline) — 12 scenarios
    // ------------------------------------------------------------------
    SCENARIO("article1/malloc/fifo/fixed_small",         Malloc, Fifo,      FixedSmall),
    SCENARIO("article1/malloc/fifo/variable_size",       Malloc, Fifo,      VariableSize),
    SCENARIO("article1/malloc/fifo/churn",               Malloc, Fifo,      Churn),

    SCENARIO("article1/malloc/lifo/fixed_small",         Malloc, Lifo,      FixedSmall),
    SCENARIO("article1/malloc/lifo/variable_size",       Malloc, Lifo,      VariableSize),
    SCENARIO("article1/malloc/lifo/churn",               Malloc, Lifo,      Churn),

    SCENARIO("article1/malloc/random/fixed_small",       Malloc, Random,    FixedSmall),
    SCENARIO("article1/malloc/random/variable_size",     Malloc, Random,    VariableSize),
    SCENARIO("article1/malloc/random/churn",             Malloc, Random,    Churn),

    SCENARIO("article1/malloc/long_lived/fixed_small",   Malloc, LongLived, FixedSmall),
    SCENARIO("article1/malloc/long_lived/variable_size", Malloc, LongLived, VariableSize),
    SCENARIO("article1/malloc/long_lived/churn",         Malloc, LongLived, Churn),

    // ------------------------------------------------------------------
    // monotonic_arena — 3 scenarios (LongLived only; bulk-reclaim semantics)
    // ------------------------------------------------------------------
    SCENARIO("article1/monotonic_arena/long_lived/fixed_small",   MonotonicArena, LongLived, FixedSmall),
    SCENARIO("article1/monotonic_arena/long_lived/variable_size", MonotonicArena, LongLived, VariableSize),
    SCENARIO("article1/monotonic_arena/long_lived/churn",         MonotonicArena, LongLived, Churn),

    // ------------------------------------------------------------------
    // pool_allocator — 4 scenarios (fixed_small only; requires fixed block size)
    // ------------------------------------------------------------------
    SCENARIO("article1/pool_allocator/fifo/fixed_small",       Pool, Fifo,      FixedSmall),
    SCENARIO("article1/pool_allocator/lifo/fixed_small",       Pool, Lifo,      FixedSmall),
    SCENARIO("article1/pool_allocator/random/fixed_small",     Pool, Random,    FixedSmall),
    SCENARIO("article1/pool_allocator/long_lived/fixed_small", Pool, LongLived, FixedSmall),

    // ------------------------------------------------------------------
    // segregated_list — 12 scenarios
    // ------------------------------------------------------------------
    SCENARIO("article1/segregated_list/fifo/fixed_small",         SegregatedList, Fifo,      FixedSmall),
    SCENARIO("article1/segregated_list/fifo/variable_size",       SegregatedList, Fifo,      VariableSize),
    SCENARIO("article1/segregated_list/fifo/churn",               SegregatedList, Fifo,      Churn),

    SCENARIO("article1/segregated_list/lifo/fixed_small",         SegregatedList, Lifo,      FixedSmall),
    SCENARIO("article1/segregated_list/lifo/variable_size",       SegregatedList, Lifo,      VariableSize),
    SCENARIO("article1/segregated_list/lifo/churn",               SegregatedList, Lifo,      Churn),

    SCENARIO("article1/segregated_list/random/fixed_small",       SegregatedList, Random,    FixedSmall),
    SCENARIO("article1/segregated_list/random/variable_size",     SegregatedList, Random,    VariableSize),
    SCENARIO("article1/segregated_list/random/churn",             SegregatedList, Random,    Churn),

    SCENARIO("article1/segregated_list/long_lived/fixed_small",   SegregatedList, LongLived, FixedSmall),
    SCENARIO("article1/segregated_list/long_lived/variable_size", SegregatedList, LongLived, VariableSize),
    SCENARIO("article1/segregated_list/long_lived/churn",         SegregatedList, LongLived, Churn),
};

#undef SCENARIO

static constexpr u32 kArticle1MatrixCount =
    static_cast<u32>(sizeof(kArticle1Matrix) / sizeof(kArticle1Matrix[0]));

// Static assert: must stay in sync with the JSON matrix (31 scenarios)
static_assert(kArticle1MatrixCount == 31,
    "article1_matrix: expected exactly 31 scenarios; update JSON and .md if changed");

// ============================================================================
// Factory shims — one per scenario index
// ExperimentFactory is a plain function pointer (no closure), so we use
// template shims indexed into the static kArticle1Matrix array.
// ============================================================================

namespace {

template<u32 N>
IExperiment* ArticleFactory() noexcept {
    return AllocBenchExperiment::Create(kArticle1Matrix[N]);
}

// Explicit factory table — avoids index_sequence complexity
static const ExperimentFactory kFactories[31] = {
    ArticleFactory< 0>, ArticleFactory< 1>, ArticleFactory< 2>,
    ArticleFactory< 3>, ArticleFactory< 4>, ArticleFactory< 5>,
    ArticleFactory< 6>, ArticleFactory< 7>, ArticleFactory< 8>,
    ArticleFactory< 9>, ArticleFactory<10>, ArticleFactory<11>,
    ArticleFactory<12>, ArticleFactory<13>, ArticleFactory<14>,
    ArticleFactory<15>, ArticleFactory<16>, ArticleFactory<17>,
    ArticleFactory<18>, ArticleFactory<19>, ArticleFactory<20>,
    ArticleFactory<21>, ArticleFactory<22>, ArticleFactory<23>,
    ArticleFactory<24>, ArticleFactory<25>, ArticleFactory<26>,
    ArticleFactory<27>, ArticleFactory<28>, ArticleFactory<29>,
    ArticleFactory<30>,
};

static_assert(sizeof(kFactories)/sizeof(kFactories[0]) == 31,
    "kFactories must have exactly 31 entries matching kArticle1Matrix");

} // namespace

// ============================================================================
// RegisterArticle1Matrix
// ============================================================================

void RegisterArticle1Matrix(ExperimentRegistry& registry) noexcept {
    for (u32 i = 0; i < kArticle1MatrixCount; ++i) {
        const AllocBenchConfig& cfg = kArticle1Matrix[i];

        ExperimentDescriptor desc{};
        desc.name          = cfg.scenarioName;
        desc.category      = "article1";

        // Derive allocatorName from AllocatorType
        switch (cfg.allocatorType) {
            case AllocatorType::Malloc:         desc.allocatorName = "malloc";          break;
            case AllocatorType::MonotonicArena: desc.allocatorName = "monotonic_arena"; break;
            case AllocatorType::Pool:           desc.allocatorName = "pool_allocator";  break;
            case AllocatorType::SegregatedList: desc.allocatorName = "segregated_list"; break;
        }

        desc.description = "Article 1 matrix scenario: allocator x lifetime x workload";
        desc.factory     = kFactories[i];

        desc.scenarioSeed        = cfg.seed;        // 0 = use global default
        desc.scenarioRepetitions = cfg.repetitions; // 0 = use global default (5)
        desc.paramsHash          = ComputeAllocBenchParamsHash(cfg);

        registry.Register(desc);
    }
}

const AllocBenchConfig* GetArticle1MatrixEntries(u32& count) noexcept {
    count = kArticle1MatrixCount;
    return kArticle1Matrix;
}

} // namespace core::bench




