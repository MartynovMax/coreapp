#include "../output/jsonl_time_series_writer.hpp"
#include "../output/csv_summary_writer.hpp"
#include "../output/output_manager.hpp"
#include "../events/event_types.hpp"
#include "../measurement/metric_collector.hpp"
#include "../system/environment_metadata.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

namespace {

// Helper to check if file exists and has content
bool FileExistsAndHasContent(const char* path) {
    FILE* f = fopen(path, "r");
    if (f == nullptr) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return size > 0;
}

// Helper to delete test file
void DeleteTestFile(const char* path) {
    remove(path);
}

} // anonymous namespace

TEST(OutputTest, JsonlWriterCreatesFile) {
    const char* testPath = "test_output.jsonl";
    DeleteTestFile(testPath);

    {
        JsonlTimeSeriesWriter writer(testPath);
        ASSERT_TRUE(writer.Open());
        EXPECT_TRUE(writer.IsOpen());

        RunMetadata metadata;
        metadata.runId = "test_run_1";
        metadata.experimentName = "test_exp";
        metadata.allocatorName = "test_alloc";
        metadata.seed = 42;
        writer.SetMetadata(metadata);

        Event evt{};
        evt.type = EventType::ExperimentBegin;
        evt.experimentName = "test_exp";
        evt.timestamp = 1000000;
        evt.eventSeqNo = 1;

        writer.WriteEvent(evt);
    }

    EXPECT_TRUE(FileExistsAndHasContent(testPath));
    DeleteTestFile(testPath);
}

TEST(OutputTest, JsonlWriterWritesMultipleEventTypes) {
    const char* testPath = "test_events.jsonl";
    DeleteTestFile(testPath);

    {
        JsonlTimeSeriesWriter writer(testPath);
        ASSERT_TRUE(writer.Open());

        RunMetadata metadata;
        metadata.runId = "test_run_2";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 100;
        writer.SetMetadata(metadata);

        // ExperimentBegin
        Event evt1{};
        evt1.type = EventType::ExperimentBegin;
        evt1.experimentName = "test";
        evt1.eventSeqNo = 1;
        writer.WriteEvent(evt1);

        // Tick
        Event evt2{};
        evt2.type = EventType::Tick;
        evt2.experimentName = "test";
        evt2.phaseName = "phase1";
        evt2.eventSeqNo = 2;
        evt2.data.tick.opIndex = 10;
        evt2.data.tick.allocCount = 5;
        evt2.data.tick.freeCount = 3;
        writer.WriteEvent(evt2);

        // PhaseComplete
        Event evt3{};
        evt3.type = EventType::PhaseComplete;
        evt3.experimentName = "test";
        evt3.phaseName = "phase1";
        evt3.eventSeqNo = 3;
        evt3.data.phaseComplete.durationNs = 5000000;
        evt3.data.phaseComplete.allocCount = 100;
        evt3.data.phaseComplete.freeCount = 80;
        writer.WriteEvent(evt3);

        // Warning
        Event evt4{};
        evt4.type = EventType::OutOfMemory;
        evt4.experimentName = "test";
        evt4.eventSeqNo = 4;
        writer.WriteEvent(evt4);
    }

    EXPECT_TRUE(FileExistsAndHasContent(testPath));
    DeleteTestFile(testPath);
}

TEST(OutputTest, CsvSummaryWriterCreatesFile) {
    const char* testPath = "test_summary.csv";
    DeleteTestFile(testPath);

    {
        CsvSummaryWriter writer(testPath);
        ASSERT_TRUE(writer.Open());
        EXPECT_TRUE(writer.IsOpen());

        RunMetadata metadata;
        metadata.runId = "test_run_3";
        metadata.experimentName = "test_exp";
        metadata.allocatorName = "test_alloc";
        metadata.seed = 42;
        metadata.warmupIterations = 1;
        metadata.measuredRepetitions = 5;
        writer.SetMetadata(metadata);

        MetricCollector collector;
        collector.Publish("timer.phase_duration_ns", MetricValue::FromU64(1000000));
        collector.Publish("counter.alloc_ops", MetricValue::FromU64(100));
        collector.Publish("counter.free_ops", MetricValue::FromU64(80));

        writer.WriteSummary(collector);
    }

    EXPECT_TRUE(FileExistsAndHasContent(testPath));
    DeleteTestFile(testPath);
}

TEST(OutputTest, CsvSummaryHandlesNAMetrics) {
    const char* testPath = "test_summary_na.csv";
    DeleteTestFile(testPath);

    {
        CsvSummaryWriter writer(testPath);
        ASSERT_TRUE(writer.Open());

        RunMetadata metadata;
        metadata.runId = "test_run_4";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 0;
        writer.SetMetadata(metadata);

        MetricCollector collector;
        // Only publish some metrics, others will be NA
        collector.Publish("timer.phase_duration_ns", MetricValue::FromU64(2000000));
        collector.Publish("counter.alloc_ops", MetricValue::NA());

        writer.WriteSummary(collector);
    }

    EXPECT_TRUE(FileExistsAndHasContent(testPath));
    DeleteTestFile(testPath);
}

TEST(OutputTest, OutputManagerTimeSeriesOnly) {
    const char* basePath = "test_manager_ts";
    const char* jsonlPath = "test_manager_ts.jsonl";
    DeleteTestFile(jsonlPath);

    {
        OutputConfig config;
        config.enableTimeSeriesOutput = true;
        config.enableSummaryOutput = false;
        config.outputPath = basePath;

        OutputManager manager(config);

        RunMetadata metadata;
        metadata.runId = "test_run_5";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 123;
        ASSERT_TRUE(manager.Initialize(metadata));

        Event evt{};
        evt.type = EventType::ExperimentBegin;
        evt.experimentName = "test";
        evt.eventSeqNo = 1;
        manager.OnEvent(evt);

        MetricCollector collector;
        manager.Finalize(collector);
    }

    EXPECT_TRUE(FileExistsAndHasContent(jsonlPath));
    DeleteTestFile(jsonlPath);
}

TEST(OutputTest, OutputManagerSummaryOnly) {
    const char* basePath = "test_manager_summary";
    const char* csvPath = "test_manager_summary.csv";
    DeleteTestFile(csvPath);

    {
        OutputConfig config;
        config.enableTimeSeriesOutput = false;
        config.enableSummaryOutput = true;
        config.outputPath = basePath;

        OutputManager manager(config);

        RunMetadata metadata;
        metadata.runId = "test_run_6";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 456;
        ASSERT_TRUE(manager.Initialize(metadata));

        MetricCollector collector;
        collector.Publish("timer.phase_duration_ns", MetricValue::FromU64(3000000));
        manager.Finalize(collector);
    }

    EXPECT_TRUE(FileExistsAndHasContent(csvPath));
    DeleteTestFile(csvPath);
}

TEST(OutputTest, OutputManagerBothOutputs) {
    const char* basePath = "test_manager_both";
    const char* jsonlPath = "test_manager_both.jsonl";
    const char* csvPath = "test_manager_both.csv";
    DeleteTestFile(jsonlPath);
    DeleteTestFile(csvPath);

    {
        OutputConfig config;
        config.enableTimeSeriesOutput = true;
        config.enableSummaryOutput = true;
        config.outputPath = basePath;

        OutputManager manager(config);

        RunMetadata metadata;
        metadata.runId = "test_run_7";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 789;
        ASSERT_TRUE(manager.Initialize(metadata));

        Event evt{};
        evt.type = EventType::PhaseComplete;
        evt.experimentName = "test";
        evt.phaseName = "phase1";
        evt.eventSeqNo = 1;
        evt.data.phaseComplete.durationNs = 4000000;
        manager.OnEvent(evt);

        MetricCollector collector;
        collector.Publish("timer.phase_duration_ns", MetricValue::FromU64(4000000));
        collector.Publish("counter.alloc_ops", MetricValue::FromU64(200));
        manager.Finalize(collector);
    }

    EXPECT_TRUE(FileExistsAndHasContent(jsonlPath));
    EXPECT_TRUE(FileExistsAndHasContent(csvPath));
    DeleteTestFile(jsonlPath);
    DeleteTestFile(csvPath);
}

TEST(OutputTest, OutputManagerNoOutputs) {
    OutputConfig config;
    config.enableTimeSeriesOutput = false;
    config.enableSummaryOutput = false;
    config.outputPath = nullptr;

    OutputManager manager(config);

    RunMetadata metadata;
    metadata.runId = "test_run_8";
    EXPECT_TRUE(manager.Initialize(metadata));

    Event evt{};
    evt.type = EventType::ExperimentBegin;
    manager.OnEvent(evt);

    MetricCollector collector;
    manager.Finalize(collector);
}

TEST(OutputTest, JsonlWriterEscapesSpecialCharacters) {
    const char* testPath = "test_escape.jsonl";
    DeleteTestFile(testPath);

    {
        JsonlTimeSeriesWriter writer(testPath);
        ASSERT_TRUE(writer.Open());

        RunMetadata metadata;
        metadata.runId = "test\"with\\quotes\nand\nnewlines";
        metadata.experimentName = "test\ttab";
        metadata.allocatorName = "alloc";
        metadata.seed = 123;
        writer.SetMetadata(metadata);

        Event evt{};
        evt.type = EventType::ExperimentBegin;
        evt.experimentName = "exp\"name";
        evt.phaseName = "phase\nname";
        evt.eventSeqNo = 1;
        writer.WriteEvent(evt);
    }
    
    EXPECT_TRUE(FileExistsAndHasContent(testPath));
    DeleteTestFile(testPath);
}

TEST(OutputTest, JsonlWriterIncludesEnvironmentMetadata) {
    const char* testPath = "test_env_metadata.jsonl";
    DeleteTestFile(testPath);

    {
        JsonlTimeSeriesWriter writer(testPath);
        ASSERT_TRUE(writer.Open());

        RunMetadata metadata;
        metadata.runId = "test_env";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 42;

        // Collect environment metadata
        CollectEnvironmentMetadata(metadata);
        writer.SetMetadata(metadata);

        Event evt{};
        evt.type = EventType::ExperimentBegin;
        evt.experimentName = "test";
        evt.eventSeqNo = 1;
        writer.WriteEvent(evt);
    }

    ASSERT_TRUE(FileExistsAndHasContent(testPath));

    // Read file and check for metadata fields
    FILE* f = fopen(testPath, "r");
    ASSERT_NE(f, nullptr);

    char buffer[4096];
    ASSERT_NE(fgets(buffer, sizeof(buffer), f), nullptr);
    fclose(f);

    // Check schema version updated to v2
    EXPECT_NE(strstr(buffer, "\"schema_version\":\"ts.v2\""), nullptr);

    // Check environment metadata fields are present
    EXPECT_NE(strstr(buffer, "\"run_timestamp_utc\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"os_name\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"os_version\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"arch\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"compiler_name\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"compiler_version\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"build_type\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"build_flags\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"cpu_model\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"cpu_cores_logical\":"), nullptr);
    EXPECT_NE(strstr(buffer, "\"cpu_cores_physical\":"), nullptr);

    DeleteTestFile(testPath);
}

TEST(OutputTest, CsvSummaryIncludesEnvironmentMetadata) {
    const char* testPath = "test_env_metadata.csv";
    DeleteTestFile(testPath);

    {
        CsvSummaryWriter writer(testPath);
        ASSERT_TRUE(writer.Open());

        RunMetadata metadata;
        metadata.runId = "test_env";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 42;

        // Collect environment metadata
        CollectEnvironmentMetadata(metadata);
        writer.SetMetadata(metadata);

        MetricCollector collector;
        collector.Publish("timer.phase_duration_ns", MetricValue::FromU64(1000000));
        writer.WriteSummary(collector);
    }

    ASSERT_TRUE(FileExistsAndHasContent(testPath));

    // Read file and check for metadata columns
    FILE* f = fopen(testPath, "r");
    ASSERT_NE(f, nullptr);

    char header[4096];
    char dataRow[4096];
    ASSERT_NE(fgets(header, sizeof(header), f), nullptr);
    ASSERT_NE(fgets(dataRow, sizeof(dataRow), f), nullptr);
    fclose(f);

    // Check schema version in data row
    EXPECT_TRUE(strncmp(dataRow, "summary.v2,", 11) == 0);

    // Check header contains environment metadata columns
    EXPECT_NE(strstr(header, "run_timestamp_utc"), nullptr);
    EXPECT_NE(strstr(header, "os_name"), nullptr);
    EXPECT_NE(strstr(header, "os_version"), nullptr);
    EXPECT_NE(strstr(header, "arch"), nullptr);
    EXPECT_NE(strstr(header, "compiler_name"), nullptr);
    EXPECT_NE(strstr(header, "compiler_version"), nullptr);
    EXPECT_NE(strstr(header, "build_type"), nullptr);
    EXPECT_NE(strstr(header, "build_flags"), nullptr);
    EXPECT_NE(strstr(header, "cpu_model"), nullptr);
    EXPECT_NE(strstr(header, "cpu_cores_logical"), nullptr);
    EXPECT_NE(strstr(header, "cpu_cores_physical"), nullptr);

    DeleteTestFile(testPath);
}

TEST(OutputTest, MetadataPresentWithNAMetrics) {
    const char* testPath = "test_na_with_metadata.csv";
    DeleteTestFile(testPath);

    {
        CsvSummaryWriter writer(testPath);
        ASSERT_TRUE(writer.Open());

        RunMetadata metadata;
        metadata.runId = "test_na";
        metadata.experimentName = "test";
        metadata.allocatorName = "alloc";
        metadata.seed = 99;

        // Collect environment metadata
        CollectEnvironmentMetadata(metadata);
        writer.SetMetadata(metadata);

        MetricCollector collector;
        // All metrics are NA
        collector.Publish("timer.phase_duration_ns", MetricValue::NA());
        collector.Publish("counter.alloc_ops", MetricValue::NA());
        writer.WriteSummary(collector);
    }

    ASSERT_TRUE(FileExistsAndHasContent(testPath));

    // Read and verify metadata is present even with NA metrics
    FILE* f = fopen(testPath, "r");
    ASSERT_NE(f, nullptr);

    char header[4096];
    char dataRow[4096];
    ASSERT_NE(fgets(header, sizeof(header), f), nullptr);
    ASSERT_NE(fgets(dataRow, sizeof(dataRow), f), nullptr);
    fclose(f);

    // Verify schema version and run_id are present
    EXPECT_TRUE(strncmp(dataRow, "summary.v2,test_na,", 19) == 0);

    // Verify environment fields are populated (not empty)
    // Count commas to ensure all fields are present
    int commaCount = 0;
    for (const char* p = dataRow; *p; ++p) {
        if (*p == ',') commaCount++;
    }
    EXPECT_GT(commaCount, 15);  // Should have many more fields now

    DeleteTestFile(testPath);
}

