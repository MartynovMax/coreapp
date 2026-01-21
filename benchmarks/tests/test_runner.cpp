#include "../runner/experiment_runner.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;

// Mock experiment that tracks lifecycle calls
class MockExperiment : public IExperiment {
public:
    int setupCalls = 0;
    int warmupCalls = 0;
    int runPhasesCalls = 0;
    int teardownCalls = 0;
    bool throwInSetup = false;
    bool throwInRunPhases = false;

    void Setup(const ExperimentParams& params) override {
        (void)params;
        ++setupCalls;
        if (throwInSetup) throw 1;
    }

    void Warmup() override {
        ++warmupCalls;
    }

    void RunPhases() override {
        ++runPhasesCalls;
        if (throwInRunPhases) throw 2;
    }

    void Teardown() noexcept override {
        ++teardownCalls;
    }

    const char* Name() const noexcept override { return "mock"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Mock experiment"; }
    const char* AllocatorName() const noexcept override { return "none"; }

    static IExperiment* Create() noexcept { return new MockExperiment(); }
    static MockExperiment* instance;
};

MockExperiment* MockExperiment::instance = nullptr;

// Test lifecycle call order
TEST(ExperimentRunnerTest, LifecycleOrder) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "mock";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Mock";
    desc.factory = &MockExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.warmupIterations = 2;
    config.measuredRepetitions = 3;

    MockExperiment* mock = new MockExperiment();
    MockExperiment::instance = mock;

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 2;
    params.measuredRepetitions = 3;

    bool result = runner.RunExperiment(mock, params);

    EXPECT_TRUE(result);
    EXPECT_EQ(mock->setupCalls, 1);
    EXPECT_EQ(mock->warmupCalls, 2);
    EXPECT_EQ(mock->runPhasesCalls, 3);
    EXPECT_EQ(mock->teardownCalls, 1);

    delete mock;
}

// Test warmup iterations count
TEST(ExperimentRunnerTest, WarmupIterations) {
    MockExperiment* mock = new MockExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 5;
    params.measuredRepetitions = 1;

    runner.RunExperiment(mock, params);

    EXPECT_EQ(mock->warmupCalls, 5);

    delete mock;
}

// Test measured repetitions count
TEST(ExperimentRunnerTest, MeasuredRepetitions) {
    MockExperiment* mock = new MockExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 0;
    params.measuredRepetitions = 7;

    runner.RunExperiment(mock, params);

    EXPECT_EQ(mock->runPhasesCalls, 7);

    delete mock;
}

// Test exception handling in setup
TEST(ExperimentRunnerTest, ExceptionInSetup) {
    MockExperiment* mock = new MockExperiment();
    mock->throwInSetup = true;
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 1;
    params.measuredRepetitions = 1;

    bool result = runner.RunExperiment(mock, params);

    EXPECT_FALSE(result);
    EXPECT_EQ(mock->setupCalls, 1);
    EXPECT_EQ(mock->teardownCalls, 1); // Teardown still called

    delete mock;
}

// Test exception handling in run_phases
TEST(ExperimentRunnerTest, ExceptionInRunPhases) {
    MockExperiment* mock = new MockExperiment();
    mock->throwInRunPhases = true;
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    bool result = runner.RunExperiment(mock, params);

    EXPECT_FALSE(result);
    EXPECT_EQ(mock->setupCalls, 1);
    EXPECT_EQ(mock->runPhasesCalls, 1);
    EXPECT_EQ(mock->teardownCalls, 1); // Teardown still called

    delete mock;
}

// Test zero event sinks (minimal-run mode)
TEST(ExperimentRunnerTest, ZeroEventSinks) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "mock";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Mock";
    desc.factory = &MockExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    // No event sink attached

    RunConfig config;
    config.warmupIterations = 1;
    config.measuredRepetitions = 1;

    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
}
