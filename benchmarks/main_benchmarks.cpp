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
#include "experiments/article1_registry.hpp"
#include "config/scenario_loader.hpp"
#include "measurement/measurement_factory.hpp"
#include <stdio.h>

using namespace core;
using namespace core::bench;

extern "C" core::bench::IExperiment* CreateSimpleAllocExperiment();

int main(int argc, char** argv) {
    printf("[main] Starting up...\n");

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

    // Register SimpleAllocExperiment
    ExperimentDescriptor simpleAllocDesc;
    simpleAllocDesc.name = "simple_alloc";
    simpleAllocDesc.category = "allocation";
    simpleAllocDesc.allocatorName = "DefaultAllocator";
    simpleAllocDesc.description = "A simple allocation/free experiment with phase-based workload model.";
    simpleAllocDesc.factory = &CreateSimpleAllocExperiment;
    registry.Register(simpleAllocDesc);

    // Register Article 1 matrix — 31 scenarios: article1/{allocator}/{lifetime}/{workload}
    // If --config is given the JSON file replaces (not extends) the built-in table.
    if (config.hasExplicitConfig) {
        ScenarioLoadResult loaded = LoadScenariosFromJson(config.scenarioConfigPath);
        if (!loaded.ok) {
            printf("Error: failed to load scenario config '%s': %s\n",
                   config.scenarioConfigPath, loaded.errorMessage);
            return kInvalidArgs;
        }
        for (u32 i = 0; i < loaded.count; ++i) {
            RegisterLoadedScenario(registry, loaded.scenarios[i]);
        }
        printf("[main] Loaded %u scenario(s) from %s\n",
               loaded.count, config.scenarioConfigPath);
    } else {
        // Default: use the built-in C++ table
        // Run all: coreapp_benchmarks --filter=article1/*
        RegisterArticle1Matrix(registry);
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
