#include "../runner/experiment_runner.hpp"
#include "test_helpers.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

// Test lifecycle call order
TEST(ExperimentRunnerTest, LifecycleOrder) {
    CounterExperiment* counter = new CounterExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 2;
    params.measuredRepetitions = 3;

    bool result = runner.RunExperiment(counter, params);

    EXPECT_TRUE(result);
    EXPECT_EQ(counter->setupCalls, 1);
    EXPECT_EQ(counter->warmupCalls, 2);
    EXPECT_EQ(counter->runPhasesCalls, 3);
    EXPECT_EQ(counter->teardownCalls, 1);

    delete counter;
}

// Test warmup iterations count
TEST(ExperimentRunnerTest, WarmupIterations) {
    CounterExperiment* counter = new CounterExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 5;
    params.measuredRepetitions = 1;

    runner.RunExperiment(counter, params);

    EXPECT_EQ(counter->warmupCalls, 5);

    delete counter;
}

// Test measured repetitions count
TEST(ExperimentRunnerTest, MeasuredRepetitions) {
    CounterExperiment* counter = new CounterExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 0;
    params.measuredRepetitions = 7;

    runner.RunExperiment(counter, params);

    EXPECT_EQ(counter->runPhasesCalls, 7);

    delete counter;
}

// Test exception handling in setup
TEST(ExperimentRunnerTest, ExceptionInSetup) {
    FailingInSetupExperiment* failing = new FailingInSetupExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 1;
    params.measuredRepetitions = 1;

    bool result = runner.RunExperiment(failing, params);

    EXPECT_FALSE(result);

    delete failing;
}

// Test exception handling in run_phases
TEST(ExperimentRunnerTest, ExceptionInRunPhases) {
    FailingExperiment* failing = new FailingExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed = 0;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    bool result = runner.RunExperiment(failing, params);

    EXPECT_FALSE(result);

    delete failing;
}

// Test zero event sinks (minimal-run mode)
TEST(ExperimentRunnerTest, ZeroEventSinks) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    // No event sink attached

    RunConfig config;
    config.warmupIterations = 1;
    config.measuredRepetitions = 1;

    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
}
