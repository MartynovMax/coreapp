#pragma once

// =============================================================================
// output_config.hpp
// Configuration for structured output writers.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// Independent output mode toggles
struct OutputConfig {
    bool enableTextOutput = false;
    bool enableTimeSeriesOutput = false;
    bool enableSummaryOutput = false;

    const char* outputPath = nullptr;
    bool verbose = false;
};

} // namespace bench
} // namespace core
