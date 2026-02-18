#include "../runner/experiment_runner.hpp"
#include "../runner/cli_parser.hpp"
#include "test_helpers.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

// Test SUCCESS exit code
TEST(ExitCodeTest, Success) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kSuccess);
}

// Test FAILURE exit code
TEST(ExitCodeTest, Failure) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "failing";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Failing";
    desc.factory = &FailingExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kFailure);
}

// Test NO_EXPERIMENTS exit code
TEST(ExitCodeTest, NoExperiments) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.filter = "nonexistent*";
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kNoExperiments);
}

// Test INVALID_ARGS exit code
TEST(ExitCodeTest, InvalidArgs) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--invalid-flag"};
    
    bool result = parser.Parse(2, argv, config);
    
    EXPECT_FALSE(result);
}

// Test PARTIAL_FAILURE exit code
TEST(ExitCodeTest, PartialFailure) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc1;
    desc1.name = "null";
    desc1.category = "test";
    desc1.allocatorName = "none";
    desc1.description = "Null";
    desc1.factory = &NullExperiment::Create;
    registry.Register(desc1);

    ExperimentDescriptor desc2;
    desc2.name = "failing";
    desc2.category = "test";
    desc2.allocatorName = "none";
    desc2.description = "Failing";
    desc2.factory = &FailingExperiment::Create;
    registry.Register(desc2);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kPartialFailure);
}
