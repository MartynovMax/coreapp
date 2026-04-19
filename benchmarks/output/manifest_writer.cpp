#include "manifest_writer.hpp"
#include "../../core/base/core_types.hpp"
#include <stdio.h>
#include <cstddef>

namespace core {
namespace bench {

namespace {

// Write a JSON-escaped string value to file (handles null, quotes, backslashes)
void WriteJsonString(FILE* f, const char* str) noexcept {
    if (str == nullptr) {
        fprintf(f, "\"\"");
        return;
    }
    fprintf(f, "\"");
    while (*str != '\0') {
        switch (*str) {
            case '\"': fprintf(f, "\\\""); break;
            case '\\': fprintf(f, "\\\\"); break;
            case '\n': fprintf(f, "\\n"); break;
            case '\r': fprintf(f, "\\r"); break;
            case '\t': fprintf(f, "\\t"); break;
            default:   fputc(*str, f); break;
        }
        ++str;
    }
    fprintf(f, "\"");
}


// Helper: write "  "key": value" with optional leading comma for safe JSON formatting.
// The first field (fieldIndex == 0) has no comma; all subsequent fields get a leading ",\n".
void WriteJsonStringField(FILE* f, int& fieldIndex, const char* key, const char* value) noexcept {
    if (fieldIndex > 0) fprintf(f, ",\n");
    fprintf(f, "  \"%s\": ", key);
    WriteJsonString(f, value);
    ++fieldIndex;
}

void WriteJsonU32Field(FILE* f, int& fieldIndex, const char* key, u32 value) noexcept {
    if (fieldIndex > 0) fprintf(f, ",\n");
    fprintf(f, "  \"%s\": %u", key, value);
    ++fieldIndex;
}

} // anonymous namespace

bool WriteManifest(const char* basePath, const RunMetadata& metadata) noexcept {
    if (basePath == nullptr) {
        return false;
    }

    // Build path: "<basePath>_manifest.json"
    char path[512];
    int written = snprintf(path, sizeof(path), "%s_manifest.json", basePath);
    if (written < 0 || static_cast<size_t>(written) >= sizeof(path)) {
        return false; // Path truncated
    }

    FILE* f = fopen(path, "w");
    if (f == nullptr) {
        return false;
    }

    int idx = 0;
    fprintf(f, "{\n");

    // Required fields
    WriteJsonStringField(f, idx, "git_sha",           metadata.gitSha);
    WriteJsonStringField(f, idx, "compiler",           metadata.compilerName);
    WriteJsonStringField(f, idx, "compiler_version",   metadata.compilerVersion);
    WriteJsonStringField(f, idx, "build_type",         metadata.buildType);
    WriteJsonStringField(f, idx, "os_name",            metadata.osName);
    WriteJsonStringField(f, idx, "os_version",         metadata.osVersion);
    WriteJsonStringField(f, idx, "arch",               metadata.arch);
    WriteJsonStringField(f, idx, "cpu_model",          metadata.cpuModel);
    WriteJsonU32Field   (f, idx, "cpu_cores_logical",  metadata.cpuCoresLogical);
    WriteJsonU32Field   (f, idx, "cpu_cores_physical", metadata.cpuCoresPhysical);
    WriteJsonStringField(f, idx, "run_timestamp_utc",  metadata.runTimestampUtc);
    WriteJsonStringField(f, idx, "run_id",             metadata.runId);

    // Optional fields
    WriteJsonStringField(f, idx, "command_line",       metadata.commandLine);
    WriteJsonStringField(f, idx, "config_path",        metadata.configPath);
    WriteJsonStringField(f, idx, "bench_version",      metadata.benchVersion);
    WriteJsonStringField(f, idx, "build_flags",        metadata.buildFlags);

    fprintf(f, "\n}\n");
    fclose(f);
    return true;
}

} // namespace bench
} // namespace core

