// =============================================================================
// main_benchmarks.cpp
// Benchmark runner entry point.
// =============================================================================

#include "runner/experiment_registry.hpp"
#include "runner/experiment_runner.hpp"
#include "runner/cli_parser.hpp"
#include "experiments/null_experiment.hpp"
#include <stdio.h>

using namespace core::bench;

int main(int argc, char** argv) {
    // Parse CLI arguments
    CLIParser parser;
    RunConfig config;
    
    if (!parser.Parse(argc, argv, config)) {
        printf("Error: %s\n", parser.GetError());
        CLIParser::PrintHelp();
        return kInvalidArgs;
    }

    // Show help if requested
    if (config.showHelp) {
        CLIParser::PrintHelp();
        return kSuccess;
    }

    // Create registry
    ExperimentRegistry registry;

    // Register experiments
    ExperimentDescriptor nullDesc;
    nullDesc.name = "null";
    nullDesc.category = "test";
    nullDesc.allocatorName = "none";
    nullDesc.description = "No-op experiment for testing runner";
    nullDesc.factory = &NullExperiment::Create;
    registry.Register(nullDesc);

    // Show list if requested
    if (config.showList) {
        u32 count = 0;
        const ExperimentDescriptor* experiments = registry.GetAll(count);
        
        printf("Available experiments: %u\n", count);
        for (u32 i = 0; i < count; ++i) {
            printf("  %s - %s\n", experiments[i].name, experiments[i].description);
        }
        
        return kSuccess;
    }

    // Create and run experiments
    ExperimentRunner runner(registry);
    ExitCode exitCode = runner.Run(config);

    return exitCode;
}
