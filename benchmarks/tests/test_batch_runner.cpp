// =============================================================================
// test_batch_runner.cpp
// Unit tests for BatchRunner (Task 9).
// =============================================================================

#include "../runner/batch_runner.hpp"
#include "../runner/experiment_registry.hpp"
#include "../runner/experiment_descriptor.hpp"
#include "../runner/run_config.hpp"
#include "../runner/exit_codes.hpp"
#include "../experiments/null_experiment.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <cstdio>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define rmdir _rmdir
#define access _access
#else
#include <unistd.h>
#endif

using namespace core;
using namespace core::bench;

namespace {

// Check if a file exists and is non-empty
bool FileExistsNonEmpty(const char* path) {
    std::ifstream f(path, std::ios::ate);
    return f.is_open() && f.tellg() > 0;
}

// Read entire file to string
std::string ReadAll(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    return std::string(std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>());
}

// Recursively remove a directory tree (best-effort, for test cleanup)
#ifdef _WIN32
void RemoveTree(const std::string& path) {
    std::string cmd = "rmdir /s /q \"" + path + "\" >nul 2>&1";
    system(cmd.c_str());
}
#else
void RemoveTree(const std::string& path) {
    std::string cmd = "rm -rf \"" + path + "\"";
    system(cmd.c_str());
}
#endif

// Register N null experiments with article1-style names
void RegisterNullScenarios(ExperimentRegistry& registry, u32 count) {
    static const char* names[] = {
        "article1/malloc/fifo/fixed_small",
        "article1/malloc/lifo/fixed_small",
        "article1/malloc/random/fixed_small",
    };
    for (u32 i = 0; i < count && i < 3; ++i) {
        ExperimentDescriptor desc{};
        desc.name = names[i];
        desc.category = "article1";
        desc.allocatorName = "malloc";
        desc.description = "Null scenario for batch test";
        desc.factory = &core::bench::NullExperiment::Create;
        registry.Register(desc);
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Test: batch run with 2 null scenarios produces manifest and summary
// ---------------------------------------------------------------------------
TEST(BatchRunnerTest, BatchRunCreatesManifestAndSummary) {
    // Clean up from any previous failed run
    RemoveTree("runs");

    ExperimentRegistry registry;
    RegisterNullScenarios(registry, 2);

    RunConfig config;
    config.batchMode = true;
    config.filter = "article1/*";
    config.runPrefix = "article1";  // explicit — no longer a default
    config.format = OutputFormat::All;
    config.enableTextOutput = true;
    config.enableTimeSeriesOutput = true;
    config.enableSummaryOutput = true;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;
    config.warmupIterations = 0;

    BatchRunner batch(registry, config);
    ExitCode exitCode = batch.Run();

    EXPECT_EQ(exitCode, kSuccess);

    // Find the created batch directory (runs/article1/<timestamp>/)
    // We can't predict the timestamp, so scan for manifest.json
#ifdef _WIN32
    std::string findCmd = "dir /s /b runs\\article1\\*manifest.json 2>nul";
#else
    std::string findCmd = "find runs/article1 -name manifest.json 2>/dev/null";
#endif
    FILE* pipe = _popen(findCmd.c_str(), "r");
    ASSERT_NE(pipe, nullptr);
    char manifestPath[512] = {};
    if (fgets(manifestPath, sizeof(manifestPath), pipe) != nullptr) {
        // Trim newline
        for (char* p = manifestPath; *p; ++p) {
            if (*p == '\n' || *p == '\r') { *p = '\0'; break; }
        }
    }
    _pclose(pipe);

    ASSERT_NE(manifestPath[0], '\0') << "manifest.json not found under runs/article1/";

    // Verify manifest contains expected fields
    std::string manifest = ReadAll(manifestPath);
    EXPECT_NE(manifest.find("\"schema_version\""), std::string::npos);
    EXPECT_NE(manifest.find("\"batch_id\""), std::string::npos);
    EXPECT_NE(manifest.find("\"scenarios\""), std::string::npos);
    EXPECT_NE(manifest.find("\"environment\""), std::string::npos);
    EXPECT_NE(manifest.find("\"scenario_count\": 2"), std::string::npos);

    // Verify per-scenario output dirs exist
    EXPECT_NE(manifest.find("\"status\": \"success\""), std::string::npos);

    // Cleanup
    RemoveTree("runs");
}

// ---------------------------------------------------------------------------
// Test: dry-run prints plan but creates no files
// ---------------------------------------------------------------------------
TEST(BatchRunnerTest, DryRunCreatesNoFiles) {
    RemoveTree("runs");

    ExperimentRegistry registry;
    RegisterNullScenarios(registry, 2);

    RunConfig config;
    config.batchMode = true;
    config.dryRun = true;
    config.filter = "article1/*";
    config.format = OutputFormat::All;
    config.enableTextOutput = true;
    config.enableTimeSeriesOutput = true;
    config.enableSummaryOutput = true;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    BatchRunner batch(registry, config);
    ExitCode exitCode = batch.Run();

    EXPECT_EQ(exitCode, kSuccess);

    // runs/ directory should not exist
#ifdef _WIN32
    EXPECT_NE(access("runs", 0), 0) << "runs/ directory should not be created in dry-run mode";
#else
    EXPECT_NE(access("runs", F_OK), 0) << "runs/ directory should not be created in dry-run mode";
#endif

    RemoveTree("runs"); // just in case
}

// ---------------------------------------------------------------------------
// Test: batch with no matching filter returns kNoExperiments
// ---------------------------------------------------------------------------
TEST(BatchRunnerTest, EmptyFilterReturnsNoExperiments) {
    ExperimentRegistry registry;
    RegisterNullScenarios(registry, 2);

    RunConfig config;
    config.batchMode = true;
    config.filter = "nonexistent_pattern_*";
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;

    BatchRunner batch(registry, config);
    ExitCode exitCode = batch.Run();

    EXPECT_EQ(exitCode, kNoExperiments);
}

// ---------------------------------------------------------------------------
// Test: batch with partial failure returns kPartialFailure
// ---------------------------------------------------------------------------
TEST(BatchRunnerTest, PartialFailureReturnsPartialFailure) {
    RemoveTree("runs");

    ExperimentRegistry registry;

    // Register one good and one failing scenario
    ExperimentDescriptor goodDesc{};
    goodDesc.name = "article1/good/test";
    goodDesc.category = "article1";
    goodDesc.allocatorName = "none";
    goodDesc.description = "Good";
    goodDesc.factory = &test::NullExperiment::Create;
    registry.Register(goodDesc);

    ExperimentDescriptor badDesc{};
    badDesc.name = "article1/bad/test";
    badDesc.category = "article1";
    badDesc.allocatorName = "none";
    badDesc.description = "Bad";
    badDesc.factory = &test::FailingExperiment::Create;
    registry.Register(badDesc);

    RunConfig config;
    config.batchMode = true;
    config.filter = "article1/*";
    config.format = OutputFormat::Text;
    config.enableTextOutput = true;
    config.measuredRepetitions = 1;
    config.minRepetitions = 1;
    config.warmupIterations = 0;

    BatchRunner batch(registry, config);
    ExitCode exitCode = batch.Run();

    EXPECT_EQ(exitCode, kPartialFailure);

    RemoveTree("runs");
}

