#include "../runner/experiment_runner.hpp"
#include "../runner/cli_parser.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;

// Mock experiments for testing
class SuccessExperiment : public IExperiment {
public:
    void Setup(const ExperimentParams& params) override { (void)params; }
    void Warmup() override {}
    void RunPhases() override {}
    void Teardown() noexcept override {}
    const char* Name() const noexcept override { return "success"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Success"; }
    const char* AllocatorName() const noexcept override { return "none"; }
    static IExperiment* Create() noexcept { return new SuccessExperiment(); }
};

class FailingExperiment : public IExperiment {
public:
    void Setup(const ExperimentParams& params) override { (void)params; throw 1; }
    void Warmup() override {}
    void RunPhases() override {}
    void Teardown() noexcept override {}
    const char* Name() const noexcept override { return "failing"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Failing"; }
    const char* AllocatorName() const noexcept override { return "none"; }
    static IExperiment* Create() noexcept { return new FailingExperiment(); }
};

// Test SUCCESS exit code
TEST(ExitCodeTest, Success) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "success";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Success";
    desc.factory = &SuccessExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.measuredRepetitions = 1;

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

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kFailure);
}

// Test NO_EXPERIMENTS exit code
TEST(ExitCodeTest, NoExperiments) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "test";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Test";
    desc.factory = &SuccessExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.filter = "nonexistent*";
    config.measuredRepetitions = 1;

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
    desc1.name = "success";
    desc1.category = "test";
    desc1.allocatorName = "none";
    desc1.description = "Success";
    desc1.factory = &SuccessExperiment::Create;
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

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kPartialFailure);
}
