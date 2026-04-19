#pragma once

// =============================================================================
// run_config.hpp
// Configuration parsed from CLI arguments.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

enum class OutputFormat {
    None,
    Text,
    Jsonl,
    Summary,
    All,
};

struct RunConfig {
    bool showList = false;
    bool showHelp = false;
    bool verbose = false;
    bool batchMode = false;
    bool dryRun = false;

    const char* filter = nullptr;

    u64 seed = 0;
    u32 warmupIterations = 3;
    u32 measuredRepetitions = 5;
    u32 minRepetitions = 5;

    bool hasExplicitSeed        = false;
    bool hasExplicitRepetitions = false;
    bool hasExplicitWarmup      = false;

    const char* scenarioConfigPath = nullptr;  // path passed via --config=<path>
    bool hasExplicitConfig         = false;

    OutputFormat format = OutputFormat::Text;
    const char* outputPath = nullptr;
    const char* measurements = nullptr;

    bool enableTextOutput = true;
    bool enableTimeSeriesOutput = false;
    bool enableSummaryOutput = false;

    const char* commandLine = "";              // Reconstructed command line for manifest

    const char* runPrefix = nullptr;           // Subdirectory under runs/ — set from JSON "run_prefix" or --run-prefix
    bool hasExplicitRunPrefix = false;         // True if set via --run-prefix CLI flag
};

} // namespace bench
} // namespace core
