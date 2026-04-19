// =============================================================================
// static_matrix_registry.cpp
// Static definition of all 39 static matrix scenarios + registration.
//
// This file is the CODE-LEVEL counterpart of the experiment JSON config
// (e.g. workspace/config/article1_matrix.json).
//
// Keep in sync with the JSON config when parameters change.
// =============================================================================

#include "static_matrix_registry.hpp"
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

static constexpr Params kFixedSmall        = {  32,  32,  100000, 1000, 0.5f };
static constexpr Params kVariableSize      = {   8, 512,  100000, 1000, 0.5f };
static constexpr Params kChurn             = {  16,  64,  200000,  500, 0.6f };
static constexpr Params kFixedSmallLarge   = {  32,  32, 1000000, 10000, 0.5f };
static constexpr Params kVariableSizeLarge = {   8, 512, 1000000, 10000, 0.5f };
static constexpr Params kHeavyChurn        = {  16,  64,  200000,  200, 0.9f };
static constexpr Params kIdealFit32        = {  32,  32,  100000, 1000, 0.5f };

} // namespace profiles

// ============================================================================
// Static matrix definition — 39 entries, ordered by allocator group
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

static const AllocBenchConfig kStaticMatrix[] = {

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

    // ------------------------------------------------------------------
    // v2: scale tests — 5 scenarios (1M ops, 10k live)
    // ------------------------------------------------------------------
    SCENARIO("article1/malloc/random/fixed_small_large",            Malloc,         Random, FixedSmallLarge),
    SCENARIO("article1/malloc/random/variable_size_large",          Malloc,         Random, VariableSizeLarge),
    SCENARIO("article1/pool_allocator/random/fixed_small_large",    Pool,           Random, FixedSmallLarge),
    SCENARIO("article1/segregated_list/random/fixed_small_large",   SegregatedList, Random, FixedSmallLarge),
    SCENARIO("article1/segregated_list/random/variable_size_large", SegregatedList, Random, VariableSizeLarge),

    // ------------------------------------------------------------------
    // v2: heavy_churn — 2 scenarios (ratio 0.9, 200 live)
    // ------------------------------------------------------------------
    SCENARIO("article1/malloc/random/heavy_churn",          Malloc,         Random, HeavyChurn),
    SCENARIO("article1/segregated_list/random/heavy_churn", SegregatedList, Random, HeavyChurn),

    // ------------------------------------------------------------------
    // v2: ideal_fit_32 — 1 scenario (segregated best-case)
    // ------------------------------------------------------------------
    SCENARIO("article1/segregated_list/random/ideal_fit_32", SegregatedList, Random, IdealFit32),
};

#undef SCENARIO

static constexpr u32 kStaticMatrixCount =
    static_cast<u32>(sizeof(kStaticMatrix) / sizeof(kStaticMatrix[0]));

// Static assert: must stay in sync with the JSON matrix (39 scenarios)
static_assert(kStaticMatrixCount == 39,
    "static_matrix: expected exactly 39 scenarios; update JSON and .md if changed");

// ============================================================================
// Factory shims — one per scenario index
// ExperimentFactory is a plain function pointer (no closure), so we use
// template shims indexed into the static kStaticMatrix array.
// ============================================================================

namespace {

template<u32 N>
IExperiment* ArticleFactory() noexcept {
    return AllocBenchExperiment::Create(kStaticMatrix[N]);
}

// Explicit factory table — avoids index_sequence complexity
static const ExperimentFactory kFactories[39] = {
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
    ArticleFactory<30>, ArticleFactory<31>, ArticleFactory<32>,
    ArticleFactory<33>, ArticleFactory<34>, ArticleFactory<35>,
    ArticleFactory<36>, ArticleFactory<37>, ArticleFactory<38>,
};

static_assert(sizeof(kFactories)/sizeof(kFactories[0]) == 39,
    "kFactories must have exactly 39 entries matching kStaticMatrix");

} // namespace

// ============================================================================
// RegisterStaticMatrix
// ============================================================================

void RegisterStaticMatrix(ExperimentRegistry& registry) noexcept {
    for (u32 i = 0; i < kStaticMatrixCount; ++i) {
        const AllocBenchConfig& cfg = kStaticMatrix[i];

        ExperimentDescriptor desc{};
        desc.name          = cfg.scenarioName;

        // Derive category from first path segment of scenario name (e.g. "article1/..." → "article1")
        static thread_local char categoryBuf[64];
        const char* slash = nullptr;
        for (const char* p = cfg.scenarioName; *p; ++p) {
            if (*p == '/') { slash = p; break; }
        }
        if (slash && (slash - cfg.scenarioName) < 63) {
            const auto len = static_cast<u32>(slash - cfg.scenarioName);
            for (u32 c = 0; c < len; ++c) categoryBuf[c] = cfg.scenarioName[c];
            categoryBuf[len] = '\0';
            desc.category = categoryBuf;
        } else {
            desc.category = cfg.scenarioName;
        }

        // Derive allocatorName from AllocatorType
        switch (cfg.allocatorType) {
            case AllocatorType::Malloc:         desc.allocatorName = "malloc";          break;
            case AllocatorType::MonotonicArena: desc.allocatorName = "monotonic_arena"; break;
            case AllocatorType::Pool:           desc.allocatorName = "pool_allocator";  break;
            case AllocatorType::SegregatedList: desc.allocatorName = "segregated_list"; break;
        }

        desc.description = "Allocator benchmark scenario: allocator x lifetime x workload";
        desc.factory     = kFactories[i];

        desc.scenarioSeed        = cfg.seed;        // 0 = use global default
        desc.scenarioRepetitions = cfg.repetitions; // 0 = use global default (5)
        desc.scenarioWarmup      = cfg.warmup;      // 0 = use global default (3)
        desc.paramsHash          = ComputeAllocBenchParamsHash(cfg);

        registry.Register(desc);
    }
}

const AllocBenchConfig* GetStaticMatrixEntries(u32& count) noexcept {
    count = kStaticMatrixCount;
    return kStaticMatrix;
}

} // namespace core::bench




