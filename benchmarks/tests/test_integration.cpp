#include "../runner/experiment_runner.hpp"
#include "../runner/cli_parser.hpp"
#include "test_helpers.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

// Test full run (register → filter → execute)
TEST(IntegrationTest, FullRun) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc1;
    desc1.name = "arena/test1";
    desc1.category = "arena";
    desc1.allocatorName = "test_alloc";
    desc1.description = "Test 1";
    desc1.factory = &NullExperiment::Create;
    registry.Register(desc1);

    ExperimentDescriptor desc2;
    desc2.name = "arena/test2";
    desc2.category = "arena";
    desc2.allocatorName = "test_alloc";
    desc2.description = "Test 2";
    desc2.factory = &CounterExperiment::Create;
    registry.Register(desc2);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.filter = "arena/*";
    config.seed = 42;
    config.warmupIterations = 2;
    config.measuredRepetitions = 3;

    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
}

// Test CLI end-to-end
TEST(IntegrationTest, CLIEndToEnd) {
    CLIParser parser;
    RunConfig config;
    
    char* argv[] = {
        (char*)"prog",
        (char*)"--filter=test*",
        (char*)"--seed=123",
        (char*)"--warmup=1",
        (char*)"--repetitions=2",
        (char*)"--format=text"
    };
    
    bool parsed = parser.Parse(6, argv, config);
    ASSERT_TRUE(parsed);

    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "test_exp";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Test";
    desc.factory = &NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
}

// Test minimal run (no outputs, no measurements)
TEST(IntegrationTest, MinimalRun) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "minimal";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Minimal";
    desc.factory = &NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    // No event sinks, no measurement systems

    RunConfig config;
    config.format = OutputFormat::None;
    config.warmupIterations = 0;
    config.measuredRepetitions = 1;

    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
}

// Test multiple experiments execution
TEST(IntegrationTest, MultipleExperiments) {
    ExperimentRegistry registry;

    // Use CounterExperiment to track execution
    static int createCount = 0;
    
    class TrackingCounter : public CounterExperiment {
    public:
        void Setup(const ExperimentParams& params) override {
            CounterExperiment::Setup(params);
            ++createCount;
        }
        static IExperiment* Create() noexcept {
            return new TrackingCounter();
        }
    };

    ExperimentDescriptor desc1;
    desc1.name = "exp1";
    desc1.category = "test";
    desc1.allocatorName = "none";
    desc1.description = "Exp 1";
    desc1.factory = &TrackingCounter::Create;
    registry.Register(desc1);

    ExperimentDescriptor desc2;
    desc2.name = "exp2";
    desc2.category = "test";
    desc2.allocatorName = "none";
    desc2.description = "Exp 2";
    desc2.factory = &TrackingCounter::Create;
    registry.Register(desc2);

    createCount = 0;

    ExperimentRunner runner(registry);
    RunConfig config;
    config.warmupIterations = 1;
    config.measuredRepetitions = 2;

    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
    EXPECT_EQ(createCount, 2); // Both experiments executed
}
