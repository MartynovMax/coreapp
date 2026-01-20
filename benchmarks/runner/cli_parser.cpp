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

        // Unknown flag
        _errorMessage = "Unknown flag";
        return false;
    }

    return true;
}

} // namespace bench
} // namespace core
