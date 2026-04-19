#include "../runner/experiment_runner.hpp"
#include "../runner/experiment_registry.hpp"
#include "../runner/cli_parser.hpp"
#include "../runner/run_config.hpp"
#include "../experiments/null_experiment.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <stdio.h>

using namespace core;
using namespace core::bench;
using namespace core::bench::test;

namespace {

bool FileExists(const char* path) {
    FILE* f = fopen(path, "r");
    if (f == nullptr) {
        return false;
    }
    fclose(f);
    return true;
}

void DeleteFile(const char* path) {
    remove(path);
}

} // anonymous namespace

TEST(OutputModesTest, CLIParserSetsOutputFlags) {
    CLIParser parser;
    RunConfig config;

    // Test --format=none
    char* argv1[] = {(char*)"prog", (char*)"--format=none"};
    ASSERT_TRUE(parser.Parse(2, argv1, config));
    EXPECT_EQ(config.format, OutputFormat::None);
    EXPECT_FALSE(config.enableTextOutput);
    EXPECT_FALSE(config.enableTimeSeriesOutput);
    EXPECT_FALSE(config.enableSummaryOutput);

    // Test --format=text
    char* argv2[] = {(char*)"prog", (char*)"--format=text"};
    ASSERT_TRUE(parser.Parse(2, argv2, config));
    EXPECT_EQ(config.format, OutputFormat::Text);
    EXPECT_TRUE(config.enableTextOutput);
    EXPECT_FALSE(config.enableTimeSeriesOutput);
    EXPECT_FALSE(config.enableSummaryOutput);

    char* argv3[] = {(char*)"prog", (char*)"--format=jsonl", (char*)"--out=test"};
    ASSERT_TRUE(parser.Parse(3, argv3, config));
    EXPECT_EQ(config.format, OutputFormat::Jsonl);
    EXPECT_FALSE(config.enableTextOutput);
    EXPECT_TRUE(config.enableTimeSeriesOutput);
    EXPECT_FALSE(config.enableSummaryOutput);

    char* argv4[] = {(char*)"prog", (char*)"--format=summary", (char*)"--out=test"};
    ASSERT_TRUE(parser.Parse(3, argv4, config));
    EXPECT_EQ(config.format, OutputFormat::Summary);
    EXPECT_FALSE(config.enableTextOutput);
    EXPECT_FALSE(config.enableTimeSeriesOutput);
    EXPECT_TRUE(config.enableSummaryOutput);

    char* argv5[] = {(char*)"prog", (char*)"--format=all", (char*)"--out=test"};
    ASSERT_TRUE(parser.Parse(3, argv5, config));
    EXPECT_EQ(config.format, OutputFormat::All);
    EXPECT_TRUE(config.enableTextOutput);
    EXPECT_TRUE(config.enableTimeSeriesOutput);
    EXPECT_TRUE(config.enableSummaryOutput);
}

TEST(OutputModesTest, CLIParserRequiresOutPathForStructuredOutputs) {
    CLIParser parser;
    RunConfig config;

    // Test --format=jsonl without --out (should fail)
    char* argv1[] = {(char*)"prog", (char*)"--format=jsonl"};
    EXPECT_FALSE(parser.Parse(2, argv1, config));
    EXPECT_STREQ(parser.GetError(), "Structured outputs (jsonl/summary) require --out=<path> or --batch");

    // Test --format=summary without --out (should fail)
    char* argv2[] = {(char*)"prog", (char*)"--format=summary"};
    EXPECT_FALSE(parser.Parse(2, argv2, config));
    EXPECT_STREQ(parser.GetError(), "Structured outputs (jsonl/summary) require --out=<path> or --batch");

    // Test --format=all without --out (should fail)
    char* argv3[] = {(char*)"prog", (char*)"--format=all"};
    EXPECT_FALSE(parser.Parse(2, argv3, config));
    EXPECT_STREQ(parser.GetError(), "Structured outputs (jsonl/summary) require --out=<path> or --batch");

    // Test --format=jsonl with --out (should succeed)
    char* argv4[] = {(char*)"prog", (char*)"--format=jsonl", (char*)"--out=test"};
    EXPECT_TRUE(parser.Parse(3, argv4, config));

    // Test --format=text without --out (should succeed - text doesn't need file)
    char* argv5[] = {(char*)"prog", (char*)"--format=text"};
    EXPECT_TRUE(parser.Parse(2, argv5, config));

    // Test --format=none without --out (should succeed)
    char* argv6[] = {(char*)"prog", (char*)"--format=none"};
    EXPECT_TRUE(parser.Parse(2, argv6, config));
}

TEST(OutputModesTest, RunWithJsonlOutput) {
    const char* outputPath = "test_run_jsonl";
    const char* jsonlPath = "test_run_jsonl.jsonl";
    DeleteFile(jsonlPath);

    ExperimentRegistry registry;
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &core::bench::NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.format = OutputFormat::Jsonl;
    config.enableTimeSeriesOutput = true;
    config.enableSummaryOutput = false;
    config.outputPath = outputPath;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kSuccess);

    EXPECT_TRUE(FileExists(jsonlPath));
    DeleteFile(jsonlPath);
}

TEST(OutputModesTest, RunWithSummaryOutput) {
    const char* outputPath = "test_run_summary";
    const char* csvPath = "test_run_summary.csv";
    DeleteFile(csvPath);

    ExperimentRegistry registry;
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &core::bench::NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.format = OutputFormat::Summary;
    config.enableTimeSeriesOutput = false;
    config.enableSummaryOutput = true;
    config.outputPath = outputPath;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kSuccess);

    EXPECT_TRUE(FileExists(csvPath));
    DeleteFile(csvPath);
}

TEST(OutputModesTest, RunWithAllOutputs) {
    const char* outputPath = "test_run_all";
    const char* jsonlPath = "test_run_all.jsonl";
    const char* csvPath = "test_run_all.csv";
    DeleteFile(jsonlPath);
    DeleteFile(csvPath);

    ExperimentRegistry registry;
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &core::bench::NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.format = OutputFormat::All;
    config.enableTextOutput = true;
    config.enableTimeSeriesOutput = true;
    config.enableSummaryOutput = true;
    config.outputPath = outputPath;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kSuccess);

    EXPECT_TRUE(FileExists(jsonlPath));
    EXPECT_TRUE(FileExists(csvPath));
    DeleteFile(jsonlPath);
    DeleteFile(csvPath);
}

TEST(OutputModesTest, RunWithNoOutputs) {
    ExperimentRegistry registry;
    ExperimentDescriptor desc;
    desc.name = "null";
    desc.category = "test";
    desc.allocatorName = "none";
    desc.description = "Null";
    desc.factory = &core::bench::NullExperiment::Create;
    registry.Register(desc);

    ExperimentRunner runner(registry);
    RunConfig config;
    config.format = OutputFormat::None;
    config.enableTextOutput = false;
    config.enableTimeSeriesOutput = false;
    config.enableSummaryOutput = false;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    ExitCode exitCode = runner.Run(config);
    EXPECT_EQ(exitCode, kSuccess);
}
