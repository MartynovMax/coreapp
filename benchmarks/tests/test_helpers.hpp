#pragma once

// =============================================================================
// test_helpers.hpp
// Mock experiment classes for testing.
// =============================================================================

#include "../runner/experiment_interface.hpp"
#include "../events/event_types.hpp"
#include "../workload/phase_executor.hpp"
#include "../workload/phase_descriptor.hpp"
#include "../workload/workload_params.hpp"
#include "../workload/operation_stream.hpp"
#include "../workload/lifetime_tracker.hpp"
#include "../common/seeded_rng.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/base/core_types.hpp"
#include <vector>

namespace core {
namespace bench {
namespace test {

// ----------------------------------------------------------------------------
// NullExperiment - No-op experiment
// ----------------------------------------------------------------------------

class NullExperiment : public IExperiment {
public:
    void Setup(const ExperimentParams& params) override { (void)params; }
    void Warmup() override {}
    void RunPhases(u32 /*repetitionIndex*/) override {}
    void Teardown() noexcept override {}
    const char* Name() const noexcept override { return "null"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Null experiment"; }
    const char* AllocatorName() const noexcept override { return "none"; }
    static IExperiment* Create() noexcept { return new NullExperiment(); }
};

// ----------------------------------------------------------------------------
// CounterExperiment - Tracks lifecycle call counts
// ----------------------------------------------------------------------------

class CounterExperiment : public IExperiment {
public:
    int setupCalls = 0;
    int warmupCalls = 0;
    int runPhasesCalls = 0;
    int teardownCalls = 0;

    void Setup(const ExperimentParams& params) override {
        (void)params;
        ++setupCalls;
    }

    void Warmup() override {
        ++warmupCalls;
    }

    void RunPhases(u32 /*repetitionIndex*/) override {
        ++runPhasesCalls;
    }

    void Teardown() noexcept override {
        ++teardownCalls;
    }

    const char* Name() const noexcept override { return "counter"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Counter experiment"; }
    const char* AllocatorName() const noexcept override { return "none"; }

    static IExperiment* Create() noexcept { return new CounterExperiment(); }
};

// ----------------------------------------------------------------------------
// FailingExperiment - Throws exception in RunPhases
// ----------------------------------------------------------------------------

class FailingExperiment : public IExperiment {
public:
    void Setup(const ExperimentParams& params) override { (void)params; }
    void Warmup() override {}
    
    void RunPhases(u32 /*repetitionIndex*/) override {
        throw 1; // Simulate failure
    }
    
    void Teardown() noexcept override {}
    const char* Name() const noexcept override { return "failing"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Failing experiment"; }
    const char* AllocatorName() const noexcept override { return "none"; }
    static IExperiment* Create() noexcept { return new FailingExperiment(); }
};

// ----------------------------------------------------------------------------
// FailingInSetupExperiment - Throws exception in Setup
// ----------------------------------------------------------------------------

class FailingInSetupExperiment : public IExperiment {
public:
    void Setup(const ExperimentParams& params) override {
        (void)params;
        throw 1; // Simulate setup failure
    }
    void Warmup() override {}
    void RunPhases(u32 /*repetitionIndex*/) override {}
    void Teardown() noexcept override {}
    const char* Name() const noexcept override { return "failing_setup"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Failing in setup"; }
    const char* AllocatorName() const noexcept override { return "none"; }
    static IExperiment* Create() noexcept { return new FailingInSetupExperiment(); }
};

// ----------------------------------------------------------------------------
// MockEventSink - Mock implementation of IEventSink for testing
// ----------------------------------------------------------------------------

class MockEventSink : public IEventSink {
public:
    std::vector<Event> events;
    void OnEvent(const Event& event) noexcept override {
        events.push_back(event);
    }
};

// ----------------------------------------------------------------------------
// ParamCapturingExperiment - Captures ExperimentParams from Setup and
// repetitionIndex values from each RunPhases call
// ----------------------------------------------------------------------------

class ParamCapturingExperiment : public IExperiment {
public:
    ExperimentParams capturedSetupParams{};
    std::vector<u32> runPhasesIndices;

    void Setup(const ExperimentParams& params) override {
        capturedSetupParams = params;
    }
    void Warmup() override {}
    void RunPhases(u32 repetitionIndex) override {
        runPhasesIndices.push_back(repetitionIndex);
    }
    void Teardown() noexcept override {}
    const char* Name()          const noexcept override { return "param_capturing"; }
    const char* Category()      const noexcept override { return "test"; }
    const char* Description()   const noexcept override { return "Param capturing experiment"; }
    const char* AllocatorName() const noexcept override { return "none"; }
    static IExperiment* Create() noexcept { return new ParamCapturingExperiment(); }
};

// ----------------------------------------------------------------------------
// Advanced workload test helpers
// ----------------------------------------------------------------------------

namespace advanced {

struct PhaseRunResult {
    PhaseStats stats{};
    u64 liveCount = 0;
    u64 liveBytes = 0;
};

inline PhaseRunResult RunPhaseWithTracker(const PhaseDescriptor& desc,
                                          IAllocator* allocator,
                                          LifetimeTracker* tracker,
                                          MockEventSink* sink) {
    SeededRNG rng(desc.params.seed);
    PhaseContext ctx{};
    ctx.allocator = allocator;
    ctx.callbackRng = &rng;
    ctx.eventSink = sink;
    ctx.phaseName = desc.name;
    ctx.experimentName = desc.experimentName;
    ctx.phaseType = desc.type;
    ctx.repetitionId = desc.repetitionId;
    ctx.userData = desc.userData;
    if (desc.reclaimMode == ReclaimMode::None) {
        ctx.externalLifetimeTracker = tracker;
    } else {
        ctx.externalLifetimeTracker = nullptr;
    }
    PhaseExecutor exec(desc, ctx, sink);
    exec.Execute();
    PhaseRunResult result;
    result.stats = exec.GetStats();
    if (tracker) {
        result.liveCount = tracker->GetLiveCount();
        result.liveBytes = tracker->GetLiveBytes();
    }
    return result;
}

inline std::vector<u32> SampleSizes(const WorkloadParams& params, u32 count, u64 seed) {
    OperationStream stream(params);
    std::vector<u32> sizes;
    sizes.reserve(count);
    for (u32 i = 0; i < count && stream.HasNext(); ++i) {
        Operation op = stream.Next(0);
        if (op.type == OpType::Alloc) {
            sizes.push_back(op.size);
        }
    }
    return sizes;
}

} // namespace advanced

} // namespace test
} // namespace bench
} // namespace core
