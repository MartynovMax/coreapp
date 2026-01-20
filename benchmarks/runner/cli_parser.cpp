#include "cli_parser.hpp"
#include "../common/string_utils.hpp"

namespace core {
namespace bench {

bool CLIParser::Parse(int argc, char** argv, RunConfig& outConfig) noexcept {
    _errorMessage = nullptr;

    // Skip program name (argv[0])
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        if (arg == nullptr) {
            continue;
        }

        // --list
        if (StringsEqual(arg, "--list")) {
            outConfig.showList = true;
            continue;
        }

        // --filter=<pattern>
        if (StartsWith(arg, "--filter")) {
            const char* value = ExtractValue(arg, "--filter");
            if (value == nullptr || *value == '\0') {
                _errorMessage = "--filter requires a pattern value";
                return false;
            }
            outConfig.filter = value;
            continue;
        }

        // --seed=<u64>
        if (StartsWith(arg, "--seed")) {
            const char* value = ExtractValue(arg, "--seed");
            if (value == nullptr || *value == '\0') {
                _errorMessage = "--seed requires a numeric value";
                return false;
            }
            if (!ParseU64(value, outConfig.seed)) {
                _errorMessage = "--seed: invalid numeric value";
                return false;
            }
            continue;
        }

        // --warmup=<n>
        if (StartsWith(arg, "--warmup")) {
            const char* value = ExtractValue(arg, "--warmup");
            if (value == nullptr || *value == '\0') {
                _errorMessage = "--warmup requires a numeric value";
                return false;
            }
            if (!ParseU32(value, outConfig.warmupIterations)) {
                _errorMessage = "--warmup: invalid numeric value";
                return false;
            }
            continue;
        }

        // --repetitions=<n>
        if (StartsWith(arg, "--repetitions")) {
            const char* value = ExtractValue(arg, "--repetitions");
            if (value == nullptr || *value == '\0') {
                _errorMessage = "--repetitions requires a numeric value";
                return false;
            }
            if (!ParseU32(value, outConfig.measuredRepetitions)) {
                _errorMessage = "--repetitions: invalid numeric value";
                return false;
            }
            continue;
        }

        // Unknown flag
        _errorMessage = "Unknown flag";
        return false;
    }

    return true;
}

} // namespace bench
} // namespace core
