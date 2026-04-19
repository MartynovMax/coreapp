#include "../measurement/measurement_system.hpp"
#include "../measurement/metric_collector.hpp"
#include "../measurement/metric_descriptor.hpp"
#include "../measurement/metric_value.hpp"
#include "../measurement/timer_system.hpp"
#include "../measurement/counter_system.hpp"
#include "../measurement/snapshot_system.hpp"
#include "../runner/experiment_runner.hpp"
#include "../runner/experiment_registry.hpp"
#include "../runner/run_config.hpp"
#include "../experiments/null_experiment.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

TEST(MeasurementSystemsTest, MetricValueNASemantics) {
    MetricValue na = MetricValue::NA();
    EXPECT_TRUE(na.IsNA());
    EXPECT_FALSE(na.IsU64());
    EXPECT_FALSE(na.IsI64());
    EXPECT_FALSE(na.IsF64());

    MetricValue u64Val = MetricValue::FromU64(42);
    EXPECT_FALSE(u64Val.IsNA());
    EXPECT_TRUE(u64Val.IsU64());
    EXPECT_EQ(u64Val.AsU64(), 42u);

    MetricValue f64Val = MetricValue::FromF64(3.14);
    EXPECT_FALSE(f64Val.IsNA());
    EXPECT_TRUE(f64Val.IsF64());
    EXPECT_DOUBLE_EQ(f64Val.AsF64(), 3.14);
}

TEST(MeasurementSystemsTest, MetricCollectorBasicPublish) {
    MetricCollector collector;

    collector.Publish("test.metric", MetricValue::FromU64(100));

    EXPECT_EQ(collector.GetMetricCount(), 1u);

    const MetricEntry* entry = collector.FindMetric("test.metric");
    ASSERT_NE(entry, nullptr);
    EXPECT_STREQ(entry->name, "test.metric");
    EXPECT_TRUE(entry->value.IsU64());
    EXPECT_EQ(entry->value.AsU64(), 100u);
}

TEST(MeasurementSystemsTest, MetricCollectorNARegistration) {
    MetricCollector collector;

    MetricDescriptor desc{
        .name = "test.optional",
        .unit = "count",
        .classification = MetricClassification::Optional,
        .capability = Capabilities::LiveSetTracking,
    };

    collector.RegisterMetric(desc);

    EXPECT_EQ(collector.GetMetricCount(), 1u);

    const MetricEntry* entry = collector.FindMetric("test.optional");
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->value.IsNA());
    EXPECT_EQ(entry->descriptor, &desc);
}

TEST(MeasurementSystemsTest, MetricCollectorOverwrite) {
    MetricCollector collector;

    collector.Publish("test.metric", MetricValue::FromU64(100));
    collector.Publish("test.metric", MetricValue::FromU64(200));

    EXPECT_EQ(collector.GetMetricCount(), 1u);

    const MetricEntry* entry = collector.FindMetric("test.metric");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->value.AsU64(), 200u);
}

TEST(MeasurementSystemsTest, TimerSystemCapabilities) {
    TimerMeasurementSystem timer;

    const MetricDescriptor* descriptors = nullptr;
    u32 count = 0;
    timer.GetCapabilities(&descriptors, count);

    EXPECT_GT(count, 0u);
    EXPECT_NE(descriptors, nullptr);

    // Verify all timer metrics are Exact classification
    for (u32 i = 0; i < count; ++i) {
        EXPECT_EQ(descriptors[i].classification, MetricClassification::Exact);
        EXPECT_EQ(descriptors[i].capability, nullptr); // No capability requirements
    }
}

TEST(MeasurementSystemsTest, TimerSystemPublishesMetrics) {
    TimerMeasurementSystem timer;
    MetricCollector collector;

    timer.OnRunStart("test", 0, 1);

    // Simulate PhaseComplete event
    Event evt{};
    evt.type = EventType::PhaseComplete;
    evt.experimentName = "test";
    evt.phaseName = "phase1";
    evt.repetitionId = 1;
    evt.data.phaseComplete.durationNs = 1000000; // 1ms

    timer.OnEvent(evt);
    timer.OnRunEnd();

    timer.PublishMetrics(collector);

    const MetricEntry* phaseDuration = collector.FindMetric("timer.phase_duration_ns");
    ASSERT_NE(phaseDuration, nullptr);
    EXPECT_TRUE(phaseDuration->value.IsU64());
    EXPECT_EQ(phaseDuration->value.AsU64(), 1000000u);
}

TEST(MeasurementSystemsTest, CounterSystemCapabilities) {
    CounterMeasurementSystem counter;

    const MetricDescriptor* descriptors = nullptr;
    u32 count = 0;
    counter.GetCapabilities(&descriptors, count);

    EXPECT_GT(count, 0u);
    EXPECT_NE(descriptors, nullptr);

    // Verify classification mix (exact + optional)
    bool hasExact = false;
    bool hasOptional = false;
    for (u32 i = 0; i < count; ++i) {
        if (descriptors[i].classification == MetricClassification::Exact) {
            hasExact = true;
        }
        if (descriptors[i].classification == MetricClassification::Optional) {
            hasOptional = true;
        }
    }

    EXPECT_TRUE(hasExact);
    EXPECT_TRUE(hasOptional);
}

TEST(MeasurementSystemsTest, CounterSystemPublishesExactMetrics) {
    CounterMeasurementSystem counter;
    MetricCollector collector;

    counter.OnRunStart("test", 0, 1);

    // Simulate PhaseComplete event
    Event evt{};
    evt.type = EventType::PhaseComplete;
    evt.experimentName = "test";
    evt.phaseName = "phase1";
    evt.repetitionId = 1;
    evt.data.phaseComplete.allocCount = 100;
    evt.data.phaseComplete.freeCount = 80;
    evt.data.phaseComplete.bytesAllocated = 4096;
    evt.data.phaseComplete.bytesFreed = 3200;
    evt.data.phaseComplete.peakLiveCount = 0; // No tracking
    evt.data.phaseComplete.peakLiveBytes = 0;
    evt.data.phaseComplete.finalLiveCount = 20;
    evt.data.phaseComplete.finalLiveBytes = 896;

    counter.OnEvent(evt);
    counter.OnRunEnd();

    counter.PublishMetrics(collector);

    // Verify exact metrics
    const MetricEntry* allocOps = collector.FindMetric("counter.alloc_ops");
    ASSERT_NE(allocOps, nullptr);
    EXPECT_EQ(allocOps->value.AsU64(), 100u);

    const MetricEntry* freeOps = collector.FindMetric("counter.free_ops");
    ASSERT_NE(freeOps, nullptr);
    EXPECT_EQ(freeOps->value.AsU64(), 80u);

    // Verify optional metrics are NA (no tracking)
    const MetricEntry* peakLive = collector.FindMetric("counter.peak_live_count");
    ASSERT_NE(peakLive, nullptr);
    EXPECT_TRUE(peakLive->value.IsNA());
}

TEST(MeasurementSystemsTest, CounterSystemPublishesThroughputAndFailures) {
    CounterMeasurementSystem counter;
    MetricCollector collector;

    counter.OnRunStart("test", 0, 1);

    Event evt{};
    evt.type = EventType::PhaseComplete;
    evt.experimentName = "test";
    evt.phaseName = "phase1";
    evt.repetitionId = 1;
    evt.data.phaseComplete.allocCount = 1000;
    evt.data.phaseComplete.freeCount = 900;
    evt.data.phaseComplete.bytesAllocated = 32000;
    evt.data.phaseComplete.bytesFreed = 28800;
    evt.data.phaseComplete.finalLiveCount = 100;
    evt.data.phaseComplete.finalLiveBytes = 3200;
    evt.data.phaseComplete.failedAllocCount = 3;
    evt.data.phaseComplete.opsPerSec = 1234567.0;
    evt.data.phaseComplete.throughput = 9876543.0;
    evt.data.phaseComplete.reservedBytes = 0; // not available for malloc

    counter.OnEvent(evt);
    counter.PublishMetrics(collector);

    const MetricEntry* throughput = collector.FindMetric("counter.throughput_ops_per_sec");
    ASSERT_NE(throughput, nullptr);
    EXPECT_DOUBLE_EQ(throughput->value.AsF64(), 1234567.0);

    const MetricEntry* bytesPerSec = collector.FindMetric("counter.throughput_bytes_per_sec");
    ASSERT_NE(bytesPerSec, nullptr);
    EXPECT_DOUBLE_EQ(bytesPerSec->value.AsF64(), 9876543.0);

    const MetricEntry* failed = collector.FindMetric("counter.failed_alloc_count");
    ASSERT_NE(failed, nullptr);
    EXPECT_EQ(failed->value.AsU64(), 3u);

    // reserved_bytes = 0 → should be NA
    const MetricEntry* reserved = collector.FindMetric("counter.reserved_bytes");
    ASSERT_NE(reserved, nullptr);
    EXPECT_TRUE(reserved->value.IsNA());
}

TEST(MeasurementSystemsTest, CounterSystemPublishesReservedBytesWhenAvailable) {
    CounterMeasurementSystem counter;
    MetricCollector collector;

    counter.OnRunStart("test", 0, 1);

    Event evt{};
    evt.type = EventType::PhaseComplete;
    evt.experimentName = "test";
    evt.phaseName = "phase1";
    evt.repetitionId = 1;
    evt.data.phaseComplete.allocCount = 500;
    evt.data.phaseComplete.reservedBytes = 1048576; // 1 MB arena

    counter.OnEvent(evt);
    counter.PublishMetrics(collector);

    const MetricEntry* reserved = collector.FindMetric("counter.reserved_bytes");
    ASSERT_NE(reserved, nullptr);
    EXPECT_FALSE(reserved->value.IsNA());
    EXPECT_EQ(reserved->value.AsU64(), 1048576u);
}

TEST(MeasurementSystemsTest, SnapshotSystemCapturesTicks) {
    SnapshotMeasurementSystem snapshot(1); // Every tick
    MetricCollector collector;

    snapshot.OnRunStart("test", 0, 1);

    // Simulate 3 Tick events
    for (u32 i = 0; i < 3; ++i) {
        Event evt{};
        evt.type = EventType::Tick;
        evt.experimentName = "test";
        evt.phaseName = "phase1";
        evt.data.tick.opIndex = i * 10;
        evt.data.tick.allocCount = i * 5;
        evt.data.tick.freeCount = i * 3;
        evt.data.tick.peakLiveCount = i * 2;
        evt.data.tick.peakLiveBytes = i * 64;

        snapshot.OnEvent(evt);
    }

    snapshot.OnRunEnd();
    snapshot.PublishMetrics(collector);

    const MetricEntry* tick1OpIndex = collector.FindMetric("snapshot.tick_1.op_index");
    ASSERT_NE(tick1OpIndex, nullptr);
    EXPECT_FALSE(tick1OpIndex->value.IsNA());
    EXPECT_EQ(tick1OpIndex->value.AsU64(), 0u);

    const MetricEntry* tick2OpIndex = collector.FindMetric("snapshot.tick_2.op_index");
    ASSERT_NE(tick2OpIndex, nullptr);
    EXPECT_FALSE(tick2OpIndex->value.IsNA());
    EXPECT_EQ(tick2OpIndex->value.AsU64(), 10u);

    const MetricEntry* tick3OpIndex = collector.FindMetric("snapshot.tick_3.op_index");
    ASSERT_NE(tick3OpIndex, nullptr);
    EXPECT_FALSE(tick3OpIndex->value.IsNA());
    EXPECT_EQ(tick3OpIndex->value.AsU64(), 20u);
}

TEST(MeasurementSystemsTest, SameWorkloadDifferentMeasurements) {
    ExperimentRegistry registry;
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null experiment";
    desc.factory = &core::bench::NullExperiment::Create;
    registry.Register(desc);

    {
        ExperimentRunner runner(registry);
        TimerMeasurementSystem timer;
        runner.RegisterMeasurementSystem(&timer);

        RunConfig config;
        config.format = OutputFormat::None;
        config.measuredRepetitions = 1;
        config.minRepetitions = 1;

        ExitCode exitCode = runner.Run(config);
        EXPECT_EQ(exitCode, kSuccess);

        MetricCollector* collector = runner.GetMetricCollector();
        ASSERT_NE(collector, nullptr);

        const MetricEntry* timerMetric = collector->FindMetric("timer.phase_duration_ns");
        if (timerMetric != nullptr) {
            EXPECT_TRUE(timerMetric->value.IsNA());
        }
    }

    {
        ExperimentRunner runner(registry);
        CounterMeasurementSystem counter;
        runner.RegisterMeasurementSystem(&counter);

        RunConfig config;
        config.format = OutputFormat::None;
        config.measuredRepetitions = 1;
        config.minRepetitions = 1;

        ExitCode exitCode = runner.Run(config);
        EXPECT_EQ(exitCode, kSuccess);

        MetricCollector* collector = runner.GetMetricCollector();
        ASSERT_NE(collector, nullptr);

        const MetricEntry* counterMetric = collector->FindMetric("counter.alloc_ops");
        if (counterMetric != nullptr) {
            EXPECT_TRUE(counterMetric->value.IsNA());
        }
    }

    {
        ExperimentRunner runner(registry);
        TimerMeasurementSystem timer;
        CounterMeasurementSystem counter;
        runner.RegisterMeasurementSystem(&timer);
        runner.RegisterMeasurementSystem(&counter);

        RunConfig config;
        config.format = OutputFormat::None;
        config.measuredRepetitions = 1;
        config.minRepetitions = 1;

        ExitCode exitCode = runner.Run(config);
        EXPECT_EQ(exitCode, kSuccess);
    }
}

TEST(MeasurementSystemsTest, DisabledSystemsPublishNA) {
    TimerMeasurementSystem timer;
    MetricCollector collector;

    timer.SetEnabled(false);
    timer.OnRunStart("test", 0, 1);
    timer.OnRunEnd();
    timer.PublishMetrics(collector);

    // All timer metrics should be registered as NA
    const MetricEntry* phaseDuration = collector.FindMetric("timer.phase_duration_ns");
    ASSERT_NE(phaseDuration, nullptr);
    EXPECT_TRUE(phaseDuration->value.IsNA());
}
