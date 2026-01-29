#include "../experiments/simple_alloc_experiment.hpp"
#include "../runner/experiment_registry.hpp"
#include "../runner/experiment_runner.hpp"
#include "../runner/experiment_params.hpp"
#include "../events/event_types.hpp"
#include "test_helpers.hpp"
#include "core/memory/bump_arena.hpp"
#include "core/memory/arena.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <unordered_map>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

static Event FindPhaseComplete(const std::vector<Event>& events, const char* phaseName) {
    for (const auto& evt : events) {
        if (evt.type == EventType::PhaseComplete && evt.phaseName && phaseName) {
            if (std::string(evt.phaseName) == phaseName) {
                return evt;
            }
        }
    }
    return Event{};
}

TEST(SimpleAllocExperimentTest, RegistrationSucceeds) {
    ExperimentRegistry registry;
    ExperimentDescriptor desc;
    desc.name = "simple_alloc";
    desc.category = "allocation";
    desc.allocatorName = "DefaultAllocator";
    desc.description = "A simple allocation/free experiment with phase-based workload model.";
    desc.factory = &SimpleAllocExperiment::Create;
    EXPECT_TRUE(registry.Register(desc));
    EXPECT_EQ(registry.Count(), 1u);
    EXPECT_NE(registry.Find("simple_alloc"), nullptr);
}

TEST(SimpleAllocExperimentTest, DiscoverableByRunner) {
    ExperimentRegistry registry;
    ExperimentDescriptor desc;
    desc.name = "simple_alloc";
    desc.category = "allocation";
    desc.allocatorName = "DefaultAllocator";
    desc.description = "A simple allocation/free experiment with phase-based workload model.";
    desc.factory = &SimpleAllocExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.filter = "simple_alloc";
    config.seed = 123;
    config.warmupIterations = 1;
    config.measuredRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kSuccess);
}

TEST(SimpleAllocExperimentTest, SetupInitializesAllocator) {
    SimpleAllocExperiment experiment;
    MockEventSink sink;
    experiment.AttachEventSink(&sink);

    ExperimentParams params;
    params.seed = 1;
    params.warmupIterations = 1;
    params.measuredRepetitions = 1;

    experiment.Setup(params);
    experiment.Warmup();
    experiment.Teardown();

    bool hasWarmupComplete = false;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::PhaseComplete && evt.phaseName) {
            if (std::string(evt.phaseName) == "Warmup") {
                hasWarmupComplete = true;
                break;
            }
        }
    }
    EXPECT_TRUE(hasWarmupComplete);
}

TEST(SimpleAllocExperimentTest, RunPhasesExecutesAllThreePhases) {
    SimpleAllocExperiment experiment;
    MockEventSink sink;
    experiment.AttachEventSink(&sink);

    ExperimentParams params;
    params.seed = 2;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    experiment.Setup(params);
    experiment.Warmup();
    experiment.RunPhases();
    experiment.Teardown();

    bool hasRampUp = false;
    bool hasSteady = false;
    bool hasBulkReclaim = false;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::PhaseBegin && evt.phaseName) {
            if (std::string(evt.phaseName) == "RampUp") hasRampUp = true;
            if (std::string(evt.phaseName) == "Steady") hasSteady = true;
            if (std::string(evt.phaseName) == "BulkReclaim") hasBulkReclaim = true;
        }
    }
    EXPECT_TRUE(hasRampUp);
    EXPECT_TRUE(hasSteady);
    EXPECT_TRUE(hasBulkReclaim);
}

TEST(SimpleAllocExperimentTest, DeterminismSameSeedIdenticalResults) {
    ExperimentParams params;
    params.seed = 3;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    SimpleAllocExperiment exp1;
    MockEventSink sink1;
    exp1.AttachEventSink(&sink1);
    exp1.Setup(params);
    exp1.Warmup();
    exp1.RunPhases();
    exp1.Teardown();

    SimpleAllocExperiment exp2;
    MockEventSink sink2;
    exp2.AttachEventSink(&sink2);
    exp2.Setup(params);
    exp2.Warmup();
    exp2.RunPhases();
    exp2.Teardown();

    Event ramp1 = FindPhaseComplete(sink1.events, "RampUp");
    Event ramp2 = FindPhaseComplete(sink2.events, "RampUp");
    Event steady1 = FindPhaseComplete(sink1.events, "Steady");
    Event steady2 = FindPhaseComplete(sink2.events, "Steady");
    Event bulk1 = FindPhaseComplete(sink1.events, "BulkReclaim");
    Event bulk2 = FindPhaseComplete(sink2.events, "BulkReclaim");

    EXPECT_EQ(ramp1.data.phaseComplete.bytesAllocated, ramp2.data.phaseComplete.bytesAllocated);
    EXPECT_EQ(ramp1.data.phaseComplete.allocCount, ramp2.data.phaseComplete.allocCount);
    EXPECT_EQ(steady1.data.phaseComplete.bytesAllocated, steady2.data.phaseComplete.bytesAllocated);
    EXPECT_EQ(steady1.data.phaseComplete.freeCount, steady2.data.phaseComplete.freeCount);
    EXPECT_EQ(bulk1.data.phaseComplete.finalLiveCount, bulk2.data.phaseComplete.finalLiveCount);
    EXPECT_EQ(bulk1.data.phaseComplete.finalLiveBytes, bulk2.data.phaseComplete.finalLiveBytes);
}

TEST(SimpleAllocExperimentTest, MallocAllocatorBulkReclaimFreesAll) {
    SimpleAllocExperiment experiment;
    MockEventSink sink;
    experiment.AttachEventSink(&sink);

    ExperimentParams params;
    params.seed = 4;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    experiment.Setup(params);
    experiment.Warmup();
    experiment.RunPhases();
    experiment.Teardown();

    Event bulk = FindPhaseComplete(sink.events, "BulkReclaim");
    EXPECT_EQ(bulk.data.phaseComplete.finalLiveCount, 0u);
    EXPECT_EQ(bulk.data.phaseComplete.finalLiveBytes, 0u);
}

static void ResetArenaCallback(void* userData) {
    auto* arena = static_cast<BumpArena*>(userData);
    arena->Reset();
}

TEST(SimpleAllocExperimentTest, BumpArenaResetCallbackWorks) {
    std::vector<u8> buffer(1024 * 1024);
    BumpArena arena(buffer.data(), buffer.size());
    ArenaAllocatorAdapter adapter(arena);

    SimpleAllocExperiment experiment;
    experiment.SetAllocatorForTests(&adapter);
    experiment.SetResetCallbackForTests(&ResetArenaCallback, &arena);

    ExperimentParams params;
    params.seed = 5;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    experiment.Setup(params);
    experiment.Warmup();
    experiment.RunPhases();
    experiment.Teardown();

    EXPECT_EQ(arena.Used(), 0u);
}

TEST(SimpleAllocExperimentTest, DifferentSeedsProduceDifferentBehavior) {
    ExperimentParams params1;
    params1.seed = 6;
    params1.warmupIterations = 0;
    params1.measuredRepetitions = 1;

    ExperimentParams params2 = params1;
    params2.seed = 7;

    SimpleAllocExperiment exp1;
    MockEventSink sink1;
    exp1.AttachEventSink(&sink1);
    exp1.Setup(params1);
    exp1.Warmup();
    exp1.RunPhases();
    exp1.Teardown();

    SimpleAllocExperiment exp2;
    MockEventSink sink2;
    exp2.AttachEventSink(&sink2);
    exp2.Setup(params2);
    exp2.Warmup();
    exp2.RunPhases();
    exp2.Teardown();

    Event ramp1 = FindPhaseComplete(sink1.events, "RampUp");
    Event ramp2 = FindPhaseComplete(sink2.events, "RampUp");

    EXPECT_NE(ramp1.data.phaseComplete.bytesAllocated, ramp2.data.phaseComplete.bytesAllocated);
}

TEST(SimpleAllocExperimentTest, EventSinkCapturesAllPhaseEvents) {
    SimpleAllocExperiment experiment;
    MockEventSink sink;
    experiment.AttachEventSink(&sink);

    ExperimentParams params;
    params.seed = 8;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    experiment.Setup(params);
    experiment.Warmup();
    experiment.RunPhases();
    experiment.Teardown();

    u32 phaseBeginCount = 0;
    u32 phaseCompleteCount = 0;
    u32 phaseEndCount = 0;
    for (const auto& evt : sink.events) {
        if (evt.type == EventType::PhaseBegin) phaseBeginCount++;
        if (evt.type == EventType::PhaseComplete) phaseCompleteCount++;
        if (evt.type == EventType::PhaseEnd) phaseEndCount++;
    }

    EXPECT_EQ(phaseBeginCount, 4u);
    EXPECT_EQ(phaseCompleteCount, 4u);
    EXPECT_EQ(phaseEndCount, 4u);
}

TEST(SimpleAllocExperimentTest, PhaseStatsMatchExpectedValues) {
    SimpleAllocExperiment experiment;
    MockEventSink sink;
    experiment.AttachEventSink(&sink);

    ExperimentParams params;
    params.seed = 9;
    params.warmupIterations = 0;
    params.measuredRepetitions = 1;

    experiment.Setup(params);
    experiment.Warmup();
    experiment.RunPhases();
    experiment.Teardown();

    Event ramp = FindPhaseComplete(sink.events, "RampUp");
    Event bulk = FindPhaseComplete(sink.events, "BulkReclaim");

    EXPECT_EQ(ramp.data.phaseComplete.totalOperations, 10000u);
    EXPECT_EQ(ramp.data.phaseComplete.allocCount, 10000u);
    EXPECT_EQ(ramp.data.phaseComplete.freeCount, 0u);
    EXPECT_EQ(ramp.data.phaseComplete.peakLiveCount, 10000u);
    EXPECT_EQ(ramp.data.phaseComplete.finalLiveCount, 10000u);

    EXPECT_EQ(bulk.data.phaseComplete.totalOperations, 0u);
    EXPECT_EQ(bulk.data.phaseComplete.finalLiveCount, 0u);
    EXPECT_EQ(bulk.data.phaseComplete.finalLiveBytes, 0u);
}

TEST(SimpleAllocExperimentTest, StressRunHundredSeeds) {
    for (u32 i = 0; i < 100; ++i) {
        SimpleAllocExperiment experiment;
        ExperimentParams params;
        params.seed = 100 + i;
        params.warmupIterations = 0;
        params.measuredRepetitions = 1;
        experiment.Setup(params);
        experiment.Warmup();
        experiment.RunPhases();
        experiment.Teardown();
    }
    SUCCEED();
}
