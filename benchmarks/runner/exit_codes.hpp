#pragma once

// =============================================================================
// exit_codes.hpp
// Stable exit codes for experiment runner.
// =============================================================================

namespace core {
namespace bench {

// Exit codes for runner process
enum ExitCode {
    kSuccess = 0,           // All experiments passed
    kFailure = 1,           // All experiments failed
    kNoExperiments = 2,     // Filter matched no experiments
    kInvalidArgs = 3,       // Invalid CLI arguments
    kPartialFailure = 4,    // Some experiments failed
};

} // namespace bench
} // namespace core
