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

    const char* filter = nullptr;

    u64 seed = 0;
    u32 warmupIterations = 0;
    u32 measuredRepetitions = 1;

    OutputFormat format = OutputFormat::Text;
    const char* outputPath = nullptr;
    const char* measurements = nullptr;

    bool enableTextOutput = true;
    bool enableTimeSeriesOutput = false;
    bool enableSummaryOutput = false;
};

} // namespace bench
} // namespace core
