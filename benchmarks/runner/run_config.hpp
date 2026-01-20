#pragma once

// =============================================================================
// run_config.hpp
// Configuration parsed from CLI arguments.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// Output format modes
enum class OutputFormat {
    None,       // Silent (no output)
    Text,       // Human-readable text
    Jsonl,      // JSONL time-series
    Summary,    // Summary table (CSV/JSON)
    All,        // All formats enabled
};

// ----------------------------------------------------------------------------
// RunConfig - Parsed CLI configuration
// ----------------------------------------------------------------------------

struct RunConfig {
    // Actions
    bool showList = false;              // --list flag
    bool showHelp = false;              // --help flag
    bool verbose = false;               // --verbose flag
    
    // Filtering
    const char* filter = nullptr;       // --filter=<pattern>
    
    // Experiment parameters
    u64 seed = 0;                       // --seed=<u64>
    u32 warmupIterations = 0;           // --warmup=<n>
    u32 measuredRepetitions = 1;        // --repetitions=<n>
    
    // Output control
    OutputFormat format = OutputFormat::Text;   // --format=<mode>
    const char* outputPath = nullptr;           // --out=<path>
};

} // namespace bench
} // namespace core
