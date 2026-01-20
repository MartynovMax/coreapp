#include "cli_parser.hpp"
#include "../common/string_utils.hpp"
#include <stdio.h>

namespace core {
namespace bench {

namespace {

// Parse OutputFormat from string
bool ParseOutputFormat(const char* str, OutputFormat& outFormat) noexcept {
    if (str == nullptr) {
        return false;
    }
    if (StringsEqual(str, "none")) {
        outFormat = OutputFormat::None;
        return true;
    }
    if (StringsEqual(str, "text")) {
        outFormat = OutputFormat::Text;
        return true;
    }
    if (StringsEqual(str, "jsonl")) {
        outFormat = OutputFormat::Jsonl;
        return true;
    }
    if (StringsEqual(str, "summary")) {
        outFormat = OutputFormat::Summary;
        return true;
    }
    if (StringsEqual(str, "all")) {
        outFormat = OutputFormat::All;
        return true;
    }
    return false;
}

} // anonymous namespace

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

        // --format=<mode>
        if (StartsWith(arg, "--format")) {
            const char* value = ExtractValue(arg, "--format");
            if (value == nullptr || *value == '\0') {
                _errorMessage = "--format requires a mode value";
                return false;
            }
            if (!ParseOutputFormat(value, outConfig.format)) {
                _errorMessage = "--format: invalid mode (use: none, text, jsonl, summary, all)";
                return false;
            }
            continue;
        }

        // --out=<path>
        if (StartsWith(arg, "--out")) {
            const char* value = ExtractValue(arg, "--out");
            if (value == nullptr || *value == '\0') {
                _errorMessage = "--out requires a path value";
                return false;
            }
            outConfig.outputPath = value;
            continue;
        }

        // --help or -h
        if (StringsEqual(arg, "--help") || StringsEqual(arg, "-h")) {
            outConfig.showHelp = true;
            continue;
        }

        // --verbose or -v
        if (StringsEqual(arg, "--verbose") || StringsEqual(arg, "-v")) {
            outConfig.verbose = true;
            continue;
        }

        // Unknown flag
        _errorMessage = "Unknown flag";
        return false;
    }

    // Validate configuration
    if (outConfig.measuredRepetitions == 0) {
        _errorMessage = "--repetitions must be at least 1";
        return false;
    }

    return true;
}

void CLIParser::PrintHelp() noexcept {
    printf("Usage: coreapp_benchmarks [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  --list                    List all available experiments\n");
    printf("  --filter=<pattern>        Run experiments matching pattern (wildcards: *, ?)\n");
    printf("  --seed=<u64>              Set deterministic seed (default: 0)\n");
    printf("  --warmup=<n>              Number of warmup iterations (default: 0)\n");
    printf("  --repetitions=<n>         Number of measured repetitions (default: 1)\n");
    printf("  --format=<mode>           Output format: none, text, jsonl, summary, all (default: text)\n");
    printf("  --out=<path>              Output file path\n");
    printf("  --help, -h                Show this help message\n");
    printf("  --verbose, -v             Enable verbose output\n");
}

} // namespace bench
} // namespace core
