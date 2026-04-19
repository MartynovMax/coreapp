#include "batch_runner.hpp"
#include "experiment_runner.hpp"
#include "experiment_registry.hpp"
#include "../system/filesystem_utils.hpp"
#include "../system/environment_metadata.hpp"
#include "../common/string_utils.hpp"
#include "../common/high_res_timer.hpp"
#include "../measurement/measurement_factory.hpp"
#include <stdio.h>
#include <string.h>

namespace core {
namespace bench {

namespace {

// Escape a string for safe embedding in a JSON value (double-quotes and backslashes).
// Writes at most outSize-1 characters and always null-terminates.
void JsonEscape(const char* src, char* out, size_t outSize) noexcept {
    if (src == nullptr) { if (outSize > 0) out[0] = '\0'; return; }
    size_t pos = 0;
    for (const char* p = src; *p != '\0' && pos + 2 < outSize; ++p) {
        if (*p == '"' || *p == '\\') {
            out[pos++] = '\\';
        }
        out[pos++] = *p;
    }
    out[pos] = '\0';
}

} // anonymous namespace

BatchRunner::BatchRunner(ExperimentRegistry& registry, const RunConfig& baseConfig) noexcept
    : _registry(&registry),
      _baseConfig(baseConfig),
      _results{},
      _resultCount(0)
{
}

ExitCode BatchRunner::Run() noexcept {
    // 1. Enumerate all experiments that match the filter
    static constexpr u32 kMaxFiltered = 256;
    const ExperimentDescriptor* experiments[kMaxFiltered];
    u32 experimentCount = 0;

    if (_baseConfig.filter != nullptr) {
        experimentCount = _registry->Filter(_baseConfig.filter, experiments, kMaxFiltered);
    } else {
        u32 allCount = 0;
        const ExperimentDescriptor* all = _registry->GetAll(allCount);
        experimentCount = (allCount > kMaxFiltered) ? kMaxFiltered : allCount;
        for (u32 i = 0; i < experimentCount; ++i) {
            experiments[i] = &all[i];
        }
    }

    if (experimentCount == 0) {
        printf("[batch] No experiments matched filter '%s'\n",
               _baseConfig.filter ? _baseConfig.filter : "(all)");
        return kNoExperiments;
    }

    printf("[batch] Found %u scenario(s) to run.\n", experimentCount);

    // 2. Generate timestamp and create batch directory
    char timestamp[32];
    FormatTimestamp(timestamp, sizeof(timestamp));

    const char* runPrefix = (_baseConfig.runPrefix != nullptr && *_baseConfig.runPrefix != '\0')
                            ? _baseConfig.runPrefix : "default";

    char batchDir[512];
    snprintf(batchDir, sizeof(batchDir), "runs%c%s%c%s",
#ifdef _WIN32
             '\\', runPrefix, '\\',
#else
             '/', runPrefix, '/',
#endif
             timestamp);

    char rawDir[512];
    BuildPath(rawDir, sizeof(rawDir), batchDir, "raw");

    // Dry-run mode: print plan and exit without executing
    if (_baseConfig.dryRun) {
        printf("[batch] DRY RUN — showing planned execution matrix:\n");
        printf("[batch] Output root: %s\n", batchDir);
        printf("[batch] ============================================================\n");
        for (u32 i = 0; i < experimentCount; ++i) {
            const ExperimentDescriptor* desc = experiments[i];
            if (desc == nullptr || desc->name == nullptr) continue;
            char sanitized[128];
            SanitizeScenarioName(desc->name, sanitized, sizeof(sanitized));
            char scenarioDir[512];
            BuildPath(scenarioDir, sizeof(scenarioDir), rawDir, sanitized);
            printf("  [%u/%u] %s\n", i + 1, experimentCount, desc->name);
            printf("         -> %s\n", scenarioDir);
        }
        printf("[batch] ============================================================\n");
        printf("[batch] Dry run complete. %u scenario(s) would be executed.\n", experimentCount);
        return kSuccess;
    }

    if (!CreateDirectoriesRecursive(rawDir)) {
        printf("[batch] ERROR: Failed to create output directory: %s\n", rawDir);
        return kFailure;
    }

    printf("[batch] Output directory: %s\n", batchDir);
    printf("[batch] ============================================================\n\n");

    // 3. Run each scenario individually
    HighResTimer timer;
    u64 batchStartNs = timer.Now();
    u32 successCount = 0;
    u32 failureCount = 0;

    for (u32 i = 0; i < experimentCount && i < kMaxBatchScenarios; ++i) {
        const ExperimentDescriptor* desc = experiments[i];
        if (desc == nullptr || desc->name == nullptr) {
            ++failureCount;
            continue;
        }

        BatchScenarioResult& result = _results[_resultCount];
        result.scenarioName = desc->name;

        // Sanitize name for directory
        SanitizeScenarioName(desc->name, result.sanitizedName, sizeof(result.sanitizedName));

        // Build per-scenario output path: <batchDir>/raw/<sanitized>/data
        char scenarioDir[512];
        BuildPath(scenarioDir, sizeof(scenarioDir), rawDir, result.sanitizedName);

        if (!CreateDirectoriesRecursive(scenarioDir)) {
            printf("[batch] ERROR: Failed to create dir: %s\n", scenarioDir);
            result.exitCode = kFailure;
            ++failureCount;
            ++_resultCount;
            continue;
        }

        char outputBase[512];
        BuildPath(outputBase, sizeof(outputBase), scenarioDir, "data");

        SafeStringCopy(result.outputDir, scenarioDir, sizeof(result.outputDir));

        // Build per-scenario RunConfig
        RunConfig scenarioConfig = _baseConfig;
        scenarioConfig.batchMode = false;     // prevent recursion
        scenarioConfig.outputPath = outputBase;

        // Filter to exactly this scenario (exact match)
        scenarioConfig.filter = desc->name;

        printf("[batch] [%u/%u] %s\n", i + 1, experimentCount, desc->name);

        u64 scenarioStartNs = timer.Now();

        // Create a fresh runner + measurement systems per scenario for isolation
        ExperimentRunner runner(*_registry);

        MeasurementFactory measurementFactory;
        if (scenarioConfig.measurements != nullptr) {
            u32 systemCount = measurementFactory.ParseAndCreate(scenarioConfig.measurements);
            IMeasurementSystem** systems = measurementFactory.GetSystems();
            for (u32 j = 0; j < systemCount; ++j) {
                if (systems[j] != nullptr) {
                    runner.RegisterMeasurementSystem(systems[j]);
                }
            }
        }

        result.exitCode = runner.Run(scenarioConfig);

        u64 scenarioEndNs = timer.Now();
        result.durationMs = (scenarioEndNs - scenarioStartNs) / 1000000ull;

        if (result.exitCode == kSuccess) {
            ++successCount;
            printf("[batch]   -> OK (%llu ms)\n\n",
                   static_cast<unsigned long long>(result.durationMs));
        } else {
            ++failureCount;
            printf("[batch]   -> FAILED (exit %d, %llu ms)\n\n",
                   result.exitCode,
                   static_cast<unsigned long long>(result.durationMs));
        }

        ++_resultCount;
    }

    u64 batchEndNs = timer.Now();
    u64 totalDurationMs = (batchEndNs - batchStartNs) / 1000000ull;

    // 4. Write batch-level manifest
    printf("[batch] ============================================================\n");
    printf("[batch] Completed: %u success, %u failed, %llu ms total\n",
           successCount, failureCount,
           static_cast<unsigned long long>(totalDurationMs));

    WriteBatchManifest(batchDir, timestamp, totalDurationMs);
    MergeSummaries(batchDir);

    printf("[batch] Results in: %s\n", batchDir);

    if (failureCount == 0) return kSuccess;
    if (successCount == 0) return kFailure;
    return kPartialFailure;
}

bool BatchRunner::WriteBatchManifest(const char* batchDir, const char* timestamp,
                                      u64 totalDurationMs) noexcept {
    char manifestPath[512];
    BuildPath(manifestPath, sizeof(manifestPath), batchDir, "manifest.json");

    FILE* f = fopen(manifestPath, "w");
    if (f == nullptr) {
        printf("[batch] Warning: failed to write batch manifest: %s\n", manifestPath);
        return false;
    }

    // Collect environment metadata
    RunMetadata envMeta{};
    HighResTimer timer;
    envMeta.startTimestampNs = timer.Now();
    envMeta.commandLine = _baseConfig.commandLine;
    envMeta.configPath = _baseConfig.scenarioConfigPath ? _baseConfig.scenarioConfigPath : "";
    CollectEnvironmentMetadata(envMeta);

    // Buffers for JSON-escaped strings
    char esc_batch_id[64], esc_matrix_config[512], esc_command_line[2048];
    char esc_filter[512], esc_measurements[256], esc_run_prefix[128];
    JsonEscape(timestamp ? timestamp : "unknown", esc_batch_id, sizeof(esc_batch_id));
    JsonEscape(_baseConfig.scenarioConfigPath ? _baseConfig.scenarioConfigPath : "", esc_matrix_config, sizeof(esc_matrix_config));
    JsonEscape(_baseConfig.commandLine ? _baseConfig.commandLine : "", esc_command_line, sizeof(esc_command_line));
    JsonEscape(_baseConfig.filter ? _baseConfig.filter : "", esc_filter, sizeof(esc_filter));
    JsonEscape(_baseConfig.measurements ? _baseConfig.measurements : "", esc_measurements, sizeof(esc_measurements));
    JsonEscape(_baseConfig.runPrefix ? _baseConfig.runPrefix : "default", esc_run_prefix, sizeof(esc_run_prefix));

    fprintf(f, "{\n");
    fprintf(f, "  \"schema_version\": \"batch_manifest.v1\",\n");
    fprintf(f, "  \"batch_id\": \"%s\",\n", esc_batch_id);
    fprintf(f, "  \"run_prefix\": \"%s\",\n", esc_run_prefix);
    fprintf(f, "  \"matrix_config\": \"%s\",\n", esc_matrix_config);
    fprintf(f, "  \"command_line\": \"%s\",\n", esc_command_line);
    fprintf(f, "  \"filter\": \"%s\",\n", esc_filter);
    fprintf(f, "  \"seed\": %llu,\n",
            static_cast<unsigned long long>(_baseConfig.seed));
    fprintf(f, "  \"repetitions\": %u,\n", _baseConfig.measuredRepetitions);
    fprintf(f, "  \"warmup\": %u,\n", _baseConfig.warmupIterations);
    fprintf(f, "  \"measurements\": \"%s\",\n", esc_measurements);
    fprintf(f, "  \"scenario_count\": %u,\n", _resultCount);
    fprintf(f, "  \"total_duration_ms\": %llu,\n",
            static_cast<unsigned long long>(totalDurationMs));

    // Environment metadata
    fprintf(f, "  \"environment\": {\n");
    fprintf(f, "    \"os_name\": \"%s\",\n", envMeta.osName);
    fprintf(f, "    \"os_version\": \"%s\",\n", envMeta.osVersion);
    fprintf(f, "    \"arch\": \"%s\",\n", envMeta.arch);
    fprintf(f, "    \"compiler_name\": \"%s\",\n", envMeta.compilerName);
    fprintf(f, "    \"compiler_version\": \"%s\",\n", envMeta.compilerVersion);
    fprintf(f, "    \"build_type\": \"%s\",\n", envMeta.buildType);
    fprintf(f, "    \"build_flags\": \"%s\",\n", envMeta.buildFlags);
    fprintf(f, "    \"cpu_model\": \"%s\",\n", envMeta.cpuModel);
    fprintf(f, "    \"cpu_cores_logical\": %u,\n", envMeta.cpuCoresLogical);
    fprintf(f, "    \"cpu_cores_physical\": %u,\n", envMeta.cpuCoresPhysical);
    fprintf(f, "    \"git_sha\": \"%s\",\n", envMeta.gitSha);
    fprintf(f, "    \"bench_version\": \"%s\"\n", envMeta.benchVersion);
    fprintf(f, "  },\n");

    fprintf(f, "  \"scenarios\": [\n");
    for (u32 i = 0; i < _resultCount; ++i) {
        const BatchScenarioResult& r = _results[i];
        char esc_name[256], esc_sanitized[256];
        JsonEscape(r.scenarioName ? r.scenarioName : "", esc_name, sizeof(esc_name));
        JsonEscape(r.sanitizedName, esc_sanitized, sizeof(esc_sanitized));
        fprintf(f, "    {\n");
        fprintf(f, "      \"name\": \"%s\",\n", esc_name);
        fprintf(f, "      \"status\": \"%s\",\n", r.exitCode == kSuccess ? "success" : "failed");
        fprintf(f, "      \"exit_code\": %d,\n", r.exitCode);
        fprintf(f, "      \"duration_ms\": %llu,\n",
                static_cast<unsigned long long>(r.durationMs));
        fprintf(f, "      \"output_dir\": \"raw/%s\"\n", esc_sanitized);
        fprintf(f, "    }%s\n", (i + 1 < _resultCount) ? "," : "");
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    printf("[batch] Manifest: %s\n", manifestPath);
    return true;
}

bool BatchRunner::MergeSummaries(const char* batchDir) noexcept {
    char mergedPath[512];
    BuildPath(mergedPath, sizeof(mergedPath), batchDir, "summary.csv");

    FILE* out = fopen(mergedPath, "w");
    if (out == nullptr) {
        printf("[batch] Warning: failed to create merged summary: %s\n", mergedPath);
        return false;
    }

    bool headerWritten = false;

    for (u32 i = 0; i < _resultCount; ++i) {
        const BatchScenarioResult& r = _results[i];

        char csvPath[512];
        snprintf(csvPath, sizeof(csvPath), "%s%cdata.csv",
                 r.outputDir,
#ifdef _WIN32
                 '\\'
#else
                 '/'
#endif
        );

        FILE* in = fopen(csvPath, "r");
        if (in == nullptr) continue;

        char line[4096];
        bool isFirstLine = true;
        while (fgets(line, sizeof(line), in) != nullptr) {
            if (isFirstLine) {
                isFirstLine = false;
                if (!headerWritten) {
                    fputs(line, out);
                    headerWritten = true;
                }
                // Skip header for subsequent files
                continue;
            }
            fputs(line, out);
        }
        fclose(in);
    }

    fclose(out);

    if (headerWritten) {
        printf("[batch] Summary:  %s\n", mergedPath);
    }
    return headerWritten;
}

} // namespace bench
} // namespace core


