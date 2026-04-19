// =============================================================================
// main_benchmarks.cpp
// Benchmark runner entry point.
// =============================================================================

#include "core/base/core_types.hpp"
#include "runner/experiment_registry.hpp"
#include "runner/experiment_runner.hpp"
#include "runner/cli_parser.hpp"
#include "experiments/null_experiment.hpp"
#include "experiments/simple_alloc_experiment.hpp"
#include "config/scenario_loader.hpp"
#include "runner/batch_runner.hpp"
#include "measurement/measurement_factory.hpp"
#include <stdio.h>

using namespace core;
using namespace core::bench;

extern "C" core::bench::IExperiment* CreateSimpleAllocExperiment();

int main(int argc, char** argv) {
    printf("[main] Starting up...\n");

    // Build command line string for manifest (quote args containing spaces)
    static char cmdLineBuffer[2048];
    {
        size_t pos = 0;
        for (int i = 0; i < argc && pos < sizeof(cmdLineBuffer) - 1; ++i) {
            if (i > 0 && pos < sizeof(cmdLineBuffer) - 1) cmdLineBuffer[pos++] = ' ';

            // Check if argument contains spaces and needs quoting
            bool needsQuote = false;
            for (const char* scan = argv[i]; *scan != '\0'; ++scan) {
                if (*scan == ' ') { needsQuote = true; break; }
            }

            if (needsQuote && pos < sizeof(cmdLineBuffer) - 1) cmdLineBuffer[pos++] = '"';
            for (const char* p = argv[i]; *p != '\0' && pos < sizeof(cmdLineBuffer) - 1; ++p) {
                cmdLineBuffer[pos++] = *p;
            }
            if (needsQuote && pos < sizeof(cmdLineBuffer) - 1) cmdLineBuffer[pos++] = '"';
        }
        cmdLineBuffer[pos] = '\0';
    }

    // Parse CLI arguments
    CLIParser parser;
    RunConfig config;

    if (!parser.Parse(argc, argv, config)) {
        printf("Error: %s\n", parser.GetError());
        CLIParser::PrintHelp();
        return kInvalidArgs;
    }

    config.commandLine = cmdLineBuffer;

    // Show help if requested
    if (config.showHelp) {
        CLIParser::PrintHelp();
        return kSuccess;
    }

    // Create registry
    ExperimentRegistry registry;

    // Register built-in experiments (always available)
    ExperimentDescriptor nullDesc;
    nullDesc.name = "null";
    nullDesc.category = "test";
    nullDesc.allocatorName = "none";
    nullDesc.description = "No-op experiment for testing runner";
    nullDesc.factory = &NullExperiment::Create;
    registry.Register(nullDesc);

    ExperimentDescriptor simpleAllocDesc;
    simpleAllocDesc.name = "simple_alloc";
    simpleAllocDesc.category = "allocation";
    simpleAllocDesc.allocatorName = "DefaultAllocator";
    simpleAllocDesc.description = "A simple allocation/free experiment with phase-based workload model.";
    simpleAllocDesc.factory = &CreateSimpleAllocExperiment;
    registry.Register(simpleAllocDesc);

    // Load scenario matrix from JSON config (if provided)
    //
    // --config=<path>  is the single entry point for loading experiment matrices.
    // The JSON file's "run_prefix" field determines the output directory prefix
    // and experiment category.  CLI --run-prefix overrides it.
    if (config.scenarioConfigPath != nullptr) {
        ScenarioLoadResult loaded = LoadScenariosFromJson(config.scenarioConfigPath);
        if (!loaded.ok) {
            printf("Error: failed to load scenario config '%s': %s\n",
                   config.scenarioConfigPath, loaded.errorMessage);
            return kInvalidArgs;
        }
        for (u32 i = 0; i < loaded.count; ++i) {
            RegisterLoadedScenario(registry, loaded.scenarios[i], loaded.category);
        }
        printf("[main] Loaded %u scenario(s) from %s (prefix: %s)\n",
               loaded.count, config.scenarioConfigPath, loaded.runPrefix);

        // Auto-set runPrefix from config if CLI didn't override
        if (!config.hasExplicitRunPrefix && loaded.runPrefix[0] != '\0') {
            config.runPrefix = loaded.runPrefix;
        }
    }

    if (config.showList) {
        u32 count = 0;
        const ExperimentDescriptor* experiments = registry.GetAll(count);

        printf("Available experiments: %u\n", count);
        for (u32 i = 0; i < count; ++i) {
            printf("  %s - %s\n", experiments[i].name, experiments[i].description);
        }
        return kSuccess;
    }

    printf("[main] Creating and running experiments...\n");

    // Batch mode: run all matched scenarios with auto-generated output dirs
    if (config.batchMode) {
        BatchRunner batch(registry, config);
        ExitCode batchExit = batch.Run();
        printf("[main] Batch done. Exit code: %d\n", batchExit);
        return batchExit;
    }

    ExperimentRunner runner(registry);

    MeasurementFactory measurementFactory;
    if (config.measurements != nullptr) {
        u32 systemCount = measurementFactory.ParseAndCreate(config.measurements);
        printf("[main] Enabled %u measurement system(s)\n", systemCount);

        IMeasurementSystem** systems = measurementFactory.GetSystems();
        for (u32 i = 0; i < systemCount; ++i) {
            if (systems[i] != nullptr) {
                runner.RegisterMeasurementSystem(systems[i]);
                printf("  - %s: %s\n", systems[i]->Name(), systems[i]->Description());
            }
        }
    }

    ExitCode exitCode = runner.Run(config);

    printf("[main] Done. Exit code: %d\n", exitCode);
    return exitCode;
}
