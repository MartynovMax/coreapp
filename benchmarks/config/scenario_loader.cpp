// =============================================================================
// scenario_loader.cpp
// Parses a JSON scenario matrix → AllocBenchConfig[].
// =============================================================================

#include "scenario_loader.hpp"
#include "../common/allocator_capabilities.hpp"
#include "../runner/experiment_registry.hpp"
#include "../runner/experiment_descriptor.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <cstdio>

namespace core::bench {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

bool ParseAllocatorType(const std::string& s, AllocatorType& out) noexcept {
    if (s == "malloc")          { out = AllocatorType::Malloc;         return true; }
    if (s == "monotonic_arena") { out = AllocatorType::MonotonicArena; return true; }
    if (s == "pool_allocator")  { out = AllocatorType::Pool;           return true; }
    if (s == "segregated_list") { out = AllocatorType::SegregatedList; return true; }
    return false;
}

bool ParseLifetimeModel(const std::string& s, LifetimeModel& out) noexcept {
    if (s == "fifo")       { out = LifetimeModel::Fifo;      return true; }
    if (s == "lifo")       { out = LifetimeModel::Lifo;      return true; }
    if (s == "random")     { out = LifetimeModel::Random;    return true; }
    if (s == "long_lived") { out = LifetimeModel::LongLived; return true; }
    if (s == "bounded")    { out = LifetimeModel::Bounded;   return true; }
    return false;
}

bool ParseWorkloadProfile(const std::string& s, WorkloadProfile& out) noexcept {
    if (s == "fixed_small")   { out = WorkloadProfile::FixedSmall;   return true; }
    if (s == "variable_size") { out = WorkloadProfile::VariableSize;  return true; }
    if (s == "churn")         { out = WorkloadProfile::Churn;         return true; }
    return false;
}

const char* AllocatorTypeName(AllocatorType t) noexcept {
    switch (t) {
        case AllocatorType::Malloc:         return "malloc";
        case AllocatorType::MonotonicArena: return "monotonic_arena";
        case AllocatorType::Pool:           return "pool_allocator";
        case AllocatorType::SegregatedList: return "segregated_list";
        default:                            return "unknown";
    }
}

// Persistent storage for scenario name strings (loaded scenarios live until program exit).
// Simple bump allocator: enough for 256 scenarios × 128 chars each.
static constexpr usize kNameStorageSize = kMaxLoadedScenarios * 128;
static char   s_nameStorage[kNameStorageSize];
static usize  s_nameStorageUsed = 0;

// Allocate a copy of str in the persistent name storage.
// Returns nullptr if storage is exhausted.
const char* StoreName(const std::string& str) noexcept {
    const usize len = str.size() + 1; // +1 for null terminator
    if (s_nameStorageUsed + len > kNameStorageSize) {
        return nullptr;
    }
    char* dest = s_nameStorage + s_nameStorageUsed;
    for (usize i = 0; i < str.size(); ++i) {
        dest[i] = str[i];
    }
    dest[str.size()] = '\0';
    s_nameStorageUsed += len;
    return dest;
}

} // anonymous namespace

// ============================================================================
// LoadScenariosFromJson
// ============================================================================

ScenarioLoadResult LoadScenariosFromJson(const char* path) noexcept {
    ScenarioLoadResult result{};

    if (path == nullptr) {
        snprintf(result.errorMessage, sizeof(result.errorMessage),
                 "LoadScenariosFromJson: path is null");
        return result;
    }

    try {
        std::ifstream f(path);
        if (!f.is_open()) {
            snprintf(result.errorMessage, sizeof(result.errorMessage),
                     "Cannot open file: %s", path);
            return result;
        }

        const nlohmann::json root = nlohmann::json::parse(f, /*cb=*/nullptr,
                                                           /*throw_exceptions=*/true,
                                                           /*ignore_comments=*/true);

        // ----------------------------------------------------------------
        // 1. Parse workload_profiles → lookup table
        // ----------------------------------------------------------------

        struct ProfileParams {
            u32  sizeMin        = 32;
            u32  sizeMax        = 32;
            u64  operationCount = 100000;
            u32  maxLiveObjects = 1000;
            f32  allocFreeRatio = 0.5f;
        };

        static constexpr usize kMaxProfiles = 16;
        struct ProfileEntry { std::string name; ProfileParams params; };
        ProfileEntry profiles[kMaxProfiles];
        usize profileCount = 0;

        if (root.contains("workload_profiles") && root["workload_profiles"].is_object()) {
            for (auto& [key, val] : root["workload_profiles"].items()) {
                if (profileCount >= kMaxProfiles) break;
                ProfileParams& p = profiles[profileCount].params;
                profiles[profileCount].name = key;
                if (val.contains("size_min"))         p.sizeMin        = val["size_min"].get<u32>();
                if (val.contains("size_max"))         p.sizeMax        = val["size_max"].get<u32>();
                if (val.contains("operation_count"))  p.operationCount = val["operation_count"].get<u64>();
                if (val.contains("max_live_objects")) p.maxLiveObjects = val["max_live_objects"].get<u32>();
                if (val.contains("alloc_free_ratio")) p.allocFreeRatio = val["alloc_free_ratio"].get<f32>();
                ++profileCount;
            }
        }

        auto findProfile = [&](const std::string& name) -> const ProfileParams* {
            for (usize i = 0; i < profileCount; ++i) {
                if (profiles[i].name == name) return &profiles[i].params;
            }
            return nullptr;
        };

        // ----------------------------------------------------------------
        // 2a. Parse run_prefix (used for output dirs and category)
        // ----------------------------------------------------------------
        {
            std::string runPrefixStr;
            if (root.contains("run_prefix") && root["run_prefix"].is_string()) {
                runPrefixStr = root["run_prefix"].get<std::string>();
            } else {
                // Derive from filename: "config/article1_matrix.json" -> "article1_matrix"
                runPrefixStr = path;
                auto lastSlash = runPrefixStr.find_last_of("/\\");
                if (lastSlash != std::string::npos) runPrefixStr = runPrefixStr.substr(lastSlash + 1);
                auto dot = runPrefixStr.find_last_of('.');
                if (dot != std::string::npos) runPrefixStr = runPrefixStr.substr(0, dot);
            }
            snprintf(result.runPrefix, sizeof(result.runPrefix), "%s", runPrefixStr.c_str());
            snprintf(result.category, sizeof(result.category), "%s", runPrefixStr.c_str());
        }

        // ----------------------------------------------------------------
        // 2b. Parse top-level defaults (seed, repetitions)
        // ----------------------------------------------------------------
        // NOTE: seed=0 is treated as "not set" throughout the loader.
        // A JSON value of "default_seed": 0 (or per-scenario "seed": 0) will be
        // ignored and the runner's built-in default (or CLI --seed) will be used.
        // Use any non-zero seed value (e.g. 1, 42, 0xDEAD) to pin reproducibility.

        u64 defaultSeed        = 0;
        u32 defaultRepetitions = 0;
        u32 defaultWarmup      = 0;

        if (root.contains("default_seed") && root["default_seed"].is_number_unsigned()) {
            defaultSeed = root["default_seed"].get<u64>();
        }
        if (root.contains("default_repetitions") && root["default_repetitions"].is_number_unsigned()) {
            defaultRepetitions = root["default_repetitions"].get<u32>();
        }
        if (root.contains("default_warmup") && root["default_warmup"].is_number_unsigned()) {
            defaultWarmup = root["default_warmup"].get<u32>();
        }

        // ----------------------------------------------------------------
        // 3. Parse scenarios array
        // ----------------------------------------------------------------

        if (!root.contains("scenarios") || !root["scenarios"].is_array()) {
            snprintf(result.errorMessage, sizeof(result.errorMessage),
                     "JSON missing 'scenarios' array: %s", path);
            return result;
        }

        for (const auto& item : root["scenarios"]) {
            if (result.count >= kMaxLoadedScenarios) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Exceeded kMaxLoadedScenarios (%u)", kMaxLoadedScenarios);
                return result;
            }

            AllocBenchConfig& cfg = result.scenarios[result.count];

            // --- name ---
            if (!item.contains("name") || !item["name"].is_string()) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario %u missing 'name'", result.count);
                return result;
            }
            const std::string nameStr = item["name"].get<std::string>();
            cfg.scenarioName = StoreName(nameStr);
            if (cfg.scenarioName == nullptr) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Name storage exhausted at scenario %u", result.count);
                return result;
            }

            // --- allocator ---
            if (!item.contains("allocator") || !item["allocator"].is_string()) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s' missing 'allocator'", cfg.scenarioName);
                return result;
            }
            if (!ParseAllocatorType(item["allocator"].get<std::string>(), cfg.allocatorType)) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': unknown allocator '%s'",
                         cfg.scenarioName, item["allocator"].get<std::string>().c_str());
                return result;
            }

            // --- lifetime ---
            if (!item.contains("lifetime") || !item["lifetime"].is_string()) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s' missing 'lifetime'", cfg.scenarioName);
                return result;
            }
            if (!ParseLifetimeModel(item["lifetime"].get<std::string>(), cfg.lifetime)) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': unknown lifetime '%s'",
                         cfg.scenarioName, item["lifetime"].get<std::string>().c_str());
                return result;
            }

            // --- workload profile ---
            if (!item.contains("workload") || !item["workload"].is_string()) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s' missing 'workload'", cfg.scenarioName);
                return result;
            }
            const std::string workloadName = item["workload"].get<std::string>();
            const ProfileParams* prof = findProfile(workloadName);
            if (prof == nullptr) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': unknown workload profile '%s'",
                         cfg.scenarioName, workloadName.c_str());
                return result;
            }
            cfg.sizeMin        = prof->sizeMin;
            cfg.sizeMax        = prof->sizeMax;
            cfg.operationCount = prof->operationCount;
            cfg.maxLiveObjects = prof->maxLiveObjects;
            cfg.allocFreeRatio = prof->allocFreeRatio;

            // --- numeric sanity validation ---
            if (cfg.sizeMin < 1) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': sizeMin must be >= 1 (got %u)",
                         cfg.scenarioName, cfg.sizeMin);
                return result;
            }
            if (cfg.sizeMax < cfg.sizeMin) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': sizeMax (%u) must be >= sizeMin (%u)",
                         cfg.scenarioName, cfg.sizeMax, cfg.sizeMin);
                return result;
            }
            if (cfg.operationCount == 0) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': operation_count must be > 0",
                         cfg.scenarioName);
                return result;
            }
            if (cfg.maxLiveObjects == 0) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': max_live_objects must be > 0",
                         cfg.scenarioName);
                return result;
            }
            if (!(cfg.allocFreeRatio >= 0.0f && cfg.allocFreeRatio <= 1.0f)) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': alloc_free_ratio must be in [0.0, 1.0]",
                         cfg.scenarioName);
                return result;
            }
            if (!ParseWorkloadProfile(workloadName, cfg.workload)) {
                // findProfile succeeded but ParseWorkloadProfile didn't — should never happen
                // if both tables are in sync; treat as internal error
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': workload profile '%s' has no enum mapping",
                         cfg.scenarioName, workloadName.c_str());
                return result;
            }

            // --- capability-based compatibility validation ---
            const char* compatError = ValidateScenarioCompatibility(
                cfg.allocatorType, cfg.workload, cfg.lifetime);
            if (compatError != nullptr) {
                snprintf(result.errorMessage, sizeof(result.errorMessage),
                         "Scenario '%s': %s", cfg.scenarioName, compatError);
                return result;
            }

            // --- optional overrides ---
            // NOTE: seed=0 is treated as "not set" — a per-scenario "seed": 0 falls back
            // to default_seed (if non-zero), then to the runner's global default/CLI value.
            if (item.contains("seed") && item["seed"].is_number_unsigned()) {
                cfg.seed = item["seed"].get<u64>();
            } else if (defaultSeed != 0) {
                cfg.seed = defaultSeed;
            }
            if (item.contains("repetitions") && item["repetitions"].is_number_unsigned()) {
                cfg.repetitions = item["repetitions"].get<u32>();
            } else if (defaultRepetitions != 0) {
                cfg.repetitions = defaultRepetitions;
            }
            if (item.contains("warmup") && item["warmup"].is_number_unsigned()) {
                cfg.warmup = item["warmup"].get<u32>();
            } else if (defaultWarmup != 0) {
                cfg.warmup = defaultWarmup;
            }
            if (item.contains("pool_block_size") && item["pool_block_size"].is_number_unsigned()) {
                cfg.poolBlockSize = item["pool_block_size"].get<u32>();
            }
            if (item.contains("pool_block_count") && item["pool_block_count"].is_number_unsigned()) {
                cfg.poolBlockCount = item["pool_block_count"].get<u32>();
            }

            ++result.count;
        }

        result.ok = true;

    } catch (const std::exception& ex) {
        snprintf(result.errorMessage, sizeof(result.errorMessage),
                 "JSON parse error in '%s': %s", path, ex.what());
    } catch (...) {
        snprintf(result.errorMessage, sizeof(result.errorMessage),
                 "Unknown error while loading '%s'", path);
    }

    return result;
}

// ============================================================================
// RegisterLoadedScenario
// ============================================================================

namespace {

// Dynamic factory table for loaded scenarios.
// Stores AllocBenchConfig by value so factories remain valid after LoadScenariosFromJson returns.
static constexpr u32 kMaxDynamicFactories = kMaxLoadedScenarios;
static_assert(kMaxLoadedScenarios <= 256,
              "kMaxLoadedScenarios exceeds the factory shim table size (256). "
              "Expand kDynamicFactories or lower kMaxLoadedScenarios.");
static AllocBenchConfig s_dynamicConfigs[kMaxDynamicFactories];
static u32              s_dynamicCount = 0;

template<u32 N>
IExperiment* DynamicFactory() noexcept {
    return AllocBenchExperiment::Create(s_dynamicConfigs[N]);
}

// Hand-written table of 256 factory shims (same pattern as static_matrix_registry.cpp).
// Only entries [0 .. s_dynamicCount) are ever called.
#define F(n) DynamicFactory<n>
static const ExperimentFactory kDynamicFactories[kMaxDynamicFactories] = {
    F(  0),F(  1),F(  2),F(  3),F(  4),F(  5),F(  6),F(  7),
    F(  8),F(  9),F( 10),F( 11),F( 12),F( 13),F( 14),F( 15),
    F( 16),F( 17),F( 18),F( 19),F( 20),F( 21),F( 22),F( 23),
    F( 24),F( 25),F( 26),F( 27),F( 28),F( 29),F( 30),F( 31),
    F( 32),F( 33),F( 34),F( 35),F( 36),F( 37),F( 38),F( 39),
    F( 40),F( 41),F( 42),F( 43),F( 44),F( 45),F( 46),F( 47),
    F( 48),F( 49),F( 50),F( 51),F( 52),F( 53),F( 54),F( 55),
    F( 56),F( 57),F( 58),F( 59),F( 60),F( 61),F( 62),F( 63),
    F( 64),F( 65),F( 66),F( 67),F( 68),F( 69),F( 70),F( 71),
    F( 72),F( 73),F( 74),F( 75),F( 76),F( 77),F( 78),F( 79),
    F( 80),F( 81),F( 82),F( 83),F( 84),F( 85),F( 86),F( 87),
    F( 88),F( 89),F( 90),F( 91),F( 92),F( 93),F( 94),F( 95),
    F( 96),F( 97),F( 98),F( 99),F(100),F(101),F(102),F(103),
    F(104),F(105),F(106),F(107),F(108),F(109),F(110),F(111),
    F(112),F(113),F(114),F(115),F(116),F(117),F(118),F(119),
    F(120),F(121),F(122),F(123),F(124),F(125),F(126),F(127),
    F(128),F(129),F(130),F(131),F(132),F(133),F(134),F(135),
    F(136),F(137),F(138),F(139),F(140),F(141),F(142),F(143),
    F(144),F(145),F(146),F(147),F(148),F(149),F(150),F(151),
    F(152),F(153),F(154),F(155),F(156),F(157),F(158),F(159),
    F(160),F(161),F(162),F(163),F(164),F(165),F(166),F(167),
    F(168),F(169),F(170),F(171),F(172),F(173),F(174),F(175),
    F(176),F(177),F(178),F(179),F(180),F(181),F(182),F(183),
    F(184),F(185),F(186),F(187),F(188),F(189),F(190),F(191),
    F(192),F(193),F(194),F(195),F(196),F(197),F(198),F(199),
    F(200),F(201),F(202),F(203),F(204),F(205),F(206),F(207),
    F(208),F(209),F(210),F(211),F(212),F(213),F(214),F(215),
    F(216),F(217),F(218),F(219),F(220),F(221),F(222),F(223),
    F(224),F(225),F(226),F(227),F(228),F(229),F(230),F(231),
    F(232),F(233),F(234),F(235),F(236),F(237),F(238),F(239),
    F(240),F(241),F(242),F(243),F(244),F(245),F(246),F(247),
    F(248),F(249),F(250),F(251),F(252),F(253),F(254),F(255),
};
#undef F

} // anonymous namespace

void RegisterLoadedScenario(ExperimentRegistry& registry,
                             const AllocBenchConfig& cfg,
                             const char* category) noexcept {
    if (s_dynamicCount >= kMaxDynamicFactories) {
        return; // silently ignore overflow; caller should check result.count
    }

    const u32 idx = s_dynamicCount;
    s_dynamicConfigs[idx] = cfg;
    ++s_dynamicCount;

    ExperimentDescriptor desc{};
    desc.name            = cfg.scenarioName;
    desc.category        = (category != nullptr) ? category : "experiment";
    desc.allocatorName   = AllocatorTypeName(cfg.allocatorType);
    desc.description     = "Loaded from JSON scenario matrix";
    desc.factory         = kDynamicFactories[idx];
    desc.scenarioSeed        = cfg.seed;
    desc.scenarioRepetitions = cfg.repetitions;
    desc.scenarioWarmup      = cfg.warmup;
    desc.paramsHash          = ComputeAllocBenchParamsHash(cfg);

    registry.Register(desc);
}

} // namespace core::bench


