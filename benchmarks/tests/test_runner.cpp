#include "../runner/experiment_runner.hpp"
#include "test_helpers.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

// =============================================================================
// Priority resolution helpers
// Factory-captured params for Run(config) priority tests.
// =============================================================================

namespace {

// Shared capture target — reset before each priority test.
ExperimentParams g_capturedParams{};

IExperiment* PriorityCapturingFactory() noexcept {
    struct CapturingExperiment : public IExperiment {
        void Setup(const ExperimentParams& params) override { g_capturedParams = params; }
        void Warmup() override {}
        void RunPhases(u32) override {}
        void Teardown() noexcept override {}
        const char* Name()          const noexcept override { return "capturer"; }
        const char* Category()      const noexcept override { return "test"; }
        const char* Description()   const noexcept override { return "Priority capturing"; }
        const char* AllocatorName() const noexcept override { return "none"; }
    };
    return new CapturingExperiment();
}

ExperimentDescriptor MakeCapturingDescriptor(const char* name,
                                              u64 scenarioSeed = 0,
                                              u32 scenarioReps = 0) noexcept {
    ExperimentDescriptor desc{};
    desc.name                = name;
    desc.category            = "test";
    desc.allocatorName       = "none";
    desc.description         = "priority test";
    desc.factory             = PriorityCapturingFactory;
    desc.scenarioSeed        = scenarioSeed;
    desc.scenarioRepetitions = scenarioReps;
    return desc;
}

} // namespace

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
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
}

// Test event sink receives events
TEST(ExperimentRunnerTest, EventSinkReceivesEvents) {
    ExperimentRegistry registry;
    
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    
    // Create mock event sink (need to include event_bus test helper)
    class TestEventSink : public IEventSink {
    public:
        int eventCount = 0;
        void OnEvent(const Event& event) noexcept override {
            (void)event;
            ++eventCount;
        }
    };

    TestEventSink sink;
    runner.AttachEventSink(&sink);

    RunConfig config;
    config.warmupIterations = 1;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);

    EXPECT_EQ(exitCode, kSuccess);
    EXPECT_GT(sink.eventCount, 0); // Should receive events
}

// =============================================================================
// Three-level priority resolution: CLI > per-scenario config > built-in default
// =============================================================================

// Scenario has scenarioSeed=999; no --seed on CLI → runner must use 999.
TEST(ExperimentRunnerTest, PriorityResolution_ScenarioSeedUsedWhenNoExplicitCli) {
    ExperimentRegistry registry;
    registry.Register(MakeCapturingDescriptor("prio_seed_1", /*scenarioSeed=*/999));

    ExperimentRunner runner(registry);

    RunConfig config;
    config.seed              = 0;      // built-in default
    config.hasExplicitSeed   = false;  // user did NOT pass --seed
    config.measuredRepetitions = 1;
    config.minRepetitions    = 1;

    g_capturedParams = {};
    runner.Run(config);

    EXPECT_EQ(g_capturedParams.seed, 999u);
}

// Scenario has scenarioSeed=999; user passes --seed=42 → CLI wins.
TEST(ExperimentRunnerTest, PriorityResolution_CliSeedOverridesScenarioSeed) {
    ExperimentRegistry registry;
    registry.Register(MakeCapturingDescriptor("prio_seed_2", /*scenarioSeed=*/999));

    ExperimentRunner runner(registry);

    RunConfig config;
    config.seed              = 42;
    config.hasExplicitSeed   = true;   // user passed --seed=42
    config.measuredRepetitions = 1;
    config.minRepetitions    = 1;

    g_capturedParams = {};
    runner.Run(config);

    EXPECT_EQ(g_capturedParams.seed, 42u);
}

// Scenario has scenarioRepetitions=7; no --repetitions on CLI → runner must use 7.
TEST(ExperimentRunnerTest, PriorityResolution_ScenarioRepetitionsUsedWhenNoExplicitCli) {
    ExperimentRegistry registry;
    registry.Register(MakeCapturingDescriptor("prio_reps_1", /*scenarioSeed=*/0, /*scenarioReps=*/7));

    ExperimentRunner runner(registry);

    RunConfig config;
    config.seed                  = 0;
    config.measuredRepetitions   = 5;   // built-in default
    config.hasExplicitRepetitions = false;
    config.minRepetitions        = 1;

    g_capturedParams = {};
    runner.Run(config);

    EXPECT_EQ(g_capturedParams.measuredRepetitions, 7u);
}

// Scenario has scenarioRepetitions=7; user passes --repetitions=3 → CLI wins.
TEST(ExperimentRunnerTest, PriorityResolution_CliRepetitionsOverrideScenarioRepetitions) {
    ExperimentRegistry registry;
    registry.Register(MakeCapturingDescriptor("prio_reps_2", /*scenarioSeed=*/0, /*scenarioReps=*/7));

    ExperimentRunner runner(registry);

    RunConfig config;
    config.seed                  = 0;
    config.measuredRepetitions   = 3;
    config.hasExplicitRepetitions = true;  // user passed --repetitions=3
    config.minRepetitions        = 1;

    g_capturedParams = {};
    runner.Run(config);

    EXPECT_EQ(g_capturedParams.measuredRepetitions, 3u);
}

// =============================================================================
// repetitionIndex is passed correctly to RunPhases (0-based sequential indices)
// =============================================================================

TEST(ExperimentRunnerTest, RepetitionIndex_PassedSequentiallyToRunPhases) {
    ParamCapturingExperiment* exp = new ParamCapturingExperiment();
    ExperimentRegistry registry;
    ExperimentRunner runner(registry);

    ExperimentParams params;
    params.seed                = 0;
    params.warmupIterations    = 0;
    params.measuredRepetitions = 4;

    runner.RunExperiment(exp, params);

    ASSERT_EQ(exp->runPhasesIndices.size(), 4u);
    EXPECT_EQ(exp->runPhasesIndices[0], 0u);
    EXPECT_EQ(exp->runPhasesIndices[1], 1u);
    EXPECT_EQ(exp->runPhasesIndices[2], 2u);
    EXPECT_EQ(exp->runPhasesIndices[3], 3u);

    delete exp;
}

