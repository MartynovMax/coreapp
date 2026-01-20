#pragma once

// =============================================================================
// cli_parser.hpp
// Command-line argument parser for experiment runner.
// =============================================================================

#include "run_config.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// CLIParser - Parse argc/argv into RunConfig
// ----------------------------------------------------------------------------

class CLIParser {
public:
    CLIParser() noexcept = default;
    ~CLIParser() noexcept = default;

    // Parse command-line arguments into configuration
    // Returns true on success, false on error
    bool Parse(int argc, char** argv, RunConfig& outConfig) noexcept;

    // Get last error message (valid after Parse() returns false)
    const char* GetError() const noexcept { return _errorMessage; }

private:
    const char* _errorMessage = nullptr;
};

} // namespace bench
} // namespace core
