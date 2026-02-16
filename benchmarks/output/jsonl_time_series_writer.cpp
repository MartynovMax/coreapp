#include "jsonl_time_series_writer.hpp"
#include "../common/string_utils.hpp"

namespace core {
namespace bench {

JsonlTimeSeriesWriter::JsonlTimeSeriesWriter(const char* filePath) noexcept
    : _filePath(filePath), _file(nullptr), _metadata{} {
}

JsonlTimeSeriesWriter::~JsonlTimeSeriesWriter() noexcept {
    Close();
}

bool JsonlTimeSeriesWriter::Open() noexcept {
    if (_file != nullptr) {
        return true;
    }

    if (_filePath == nullptr) {
        return false;
    }

    _file = fopen(_filePath, "w");
    return _file != nullptr;
}

void JsonlTimeSeriesWriter::Close() noexcept {
    if (_file != nullptr) {
        fclose(_file);
        _file = nullptr;
    }
}

void JsonlTimeSeriesWriter::SetMetadata(const RunMetadata& metadata) noexcept {
    _metadata = metadata;
}

void JsonlTimeSeriesWriter::WriteEvent(const Event& event) noexcept {
    if (_file == nullptr) {
        return;
    }

    switch (event.type) {
        case EventType::Tick:
            WriteTickRecord(event);
            break;

        case EventType::OutOfMemory:
        case EventType::AllocationFailed:
        case EventType::PhaseFailure:
            WriteWarningRecord(event);
            break;

        default:
            WriteEventRecord(event);
            break;
    }

    fprintf(_file, "\n");
    fflush(_file);
}

void JsonlTimeSeriesWriter::WriteEventRecord(const Event& event) noexcept {
    fprintf(_file, "{");
    WriteCommonFields(event, "event");

    // Event-specific payload
    fprintf(_file, ",\"payload\":{");
    fprintf(_file, "\"event_type\":\"");

    switch (event.type) {
        case EventType::ExperimentBegin: fprintf(_file, "ExperimentBegin"); break;
        case EventType::ExperimentEnd: fprintf(_file, "ExperimentEnd"); break;
        case EventType::PhaseBegin: fprintf(_file, "PhaseBegin"); break;
        case EventType::PhaseEnd: fprintf(_file, "PhaseEnd"); break;
        case EventType::PhaseComplete:
            fprintf(_file, "PhaseComplete\",");
            fprintf(_file, "\"duration_ns\":%llu,",
                    static_cast<unsigned long long>(event.data.phaseComplete.durationNs));
            fprintf(_file, "\"alloc_count\":%llu,",
                    static_cast<unsigned long long>(event.data.phaseComplete.allocCount));
            fprintf(_file, "\"free_count\":%llu,",
                    static_cast<unsigned long long>(event.data.phaseComplete.freeCount));
            fprintf(_file, "\"bytes_allocated\":%llu,",
                    static_cast<unsigned long long>(event.data.phaseComplete.bytesAllocated));
            fprintf(_file, "\"bytes_freed\":%llu,",
                    static_cast<unsigned long long>(event.data.phaseComplete.bytesFreed));
            fprintf(_file, "\"ops_per_sec\":%.2f,", event.data.phaseComplete.opsPerSec);
            fprintf(_file, "\"throughput\":%.2f", event.data.phaseComplete.throughput);
            fprintf(_file, "}");
            fprintf(_file, "}");
            return;
        case EventType::Allocation: fprintf(_file, "Allocation"); break;
        case EventType::Free: fprintf(_file, "Free"); break;
        case EventType::AllocatorReset: fprintf(_file, "AllocatorReset"); break;
        case EventType::AllocatorRewind: fprintf(_file, "AllocatorRewind"); break;
        default: fprintf(_file, "Unknown"); break;
    }

    fprintf(_file, "\"");
    fprintf(_file, "}");
    fprintf(_file, "}");
}

void JsonlTimeSeriesWriter::WriteTickRecord(const Event& event) noexcept {
    fprintf(_file, "{");
    WriteCommonFields(event, "tick");

    fprintf(_file, ",\"payload\":{");
    fprintf(_file, "\"op_index\":%llu,", static_cast<unsigned long long>(event.data.tick.opIndex));
    fprintf(_file, "\"alloc_count\":%llu,", static_cast<unsigned long long>(event.data.tick.allocCount));
    fprintf(_file, "\"free_count\":%llu,", static_cast<unsigned long long>(event.data.tick.freeCount));
    fprintf(_file, "\"bytes_allocated\":%llu,", static_cast<unsigned long long>(event.data.tick.bytesAllocated));
    fprintf(_file, "\"bytes_freed\":%llu,", static_cast<unsigned long long>(event.data.tick.bytesFreed));
    fprintf(_file, "\"peak_live_count\":%llu,", static_cast<unsigned long long>(event.data.tick.peakLiveCount));
    fprintf(_file, "\"peak_live_bytes\":%llu", static_cast<unsigned long long>(event.data.tick.peakLiveBytes));
    fprintf(_file, "}");
    fprintf(_file, "}");
}

void JsonlTimeSeriesWriter::WriteWarningRecord(const Event& event) noexcept {
    fprintf(_file, "{");
    WriteCommonFields(event, "warning");

    fprintf(_file, ",\"payload\":{");
    fprintf(_file, "\"warning_type\":\"");

    switch (event.type) {
        case EventType::OutOfMemory: fprintf(_file, "OutOfMemory"); break;
        case EventType::AllocationFailed: fprintf(_file, "AllocationFailed"); break;
        case EventType::PhaseFailure: fprintf(_file, "PhaseFailure"); break;
        default: fprintf(_file, "Unknown"); break;
    }

    fprintf(_file, "\"");
    fprintf(_file, "}");
    fprintf(_file, "}");
}

void JsonlTimeSeriesWriter::WriteCommonFields(const Event& event, const char* recordType) noexcept {
    fprintf(_file, "\"schema_version\":\"ts.v1\"");
    fprintf(_file, ",\"record_type\":\"%s\"", recordType);

    // Run metadata
    if (_metadata.runId != nullptr) {
        fprintf(_file, ",\"run_id\":\"");
        EscapeJsonString(_metadata.runId);
        fprintf(_file, "\"");
    }

    if (_metadata.experimentName != nullptr) {
        fprintf(_file, ",\"experiment_name\":\"");
        EscapeJsonString(_metadata.experimentName);
        fprintf(_file, "\"");
    }

    if (_metadata.experimentCategory != nullptr) {
        fprintf(_file, ",\"experiment_category\":\"");
        EscapeJsonString(_metadata.experimentCategory);
        fprintf(_file, "\"");
    }

    if (_metadata.allocatorName != nullptr) {
        fprintf(_file, ",\"allocator\":\"");
        EscapeJsonString(_metadata.allocatorName);
        fprintf(_file, "\"");
    }

    fprintf(_file, ",\"seed\":%llu", static_cast<unsigned long long>(_metadata.seed));
    fprintf(_file, ",\"repetition_id\":%u", event.repetitionId);
    fprintf(_file, ",\"timestamp_ns\":%llu", static_cast<unsigned long long>(event.timestamp));
    fprintf(_file, ",\"event_seq_no\":%llu", static_cast<unsigned long long>(event.eventSeqNo));

    // Event-specific fields
    if (event.experimentName != nullptr) {
        fprintf(_file, ",\"event_experiment_name\":\"");
        EscapeJsonString(event.experimentName);
        fprintf(_file, "\"");
    }

    if (event.phaseName != nullptr) {
        fprintf(_file, ",\"phase_name\":\"");
        EscapeJsonString(event.phaseName);
        fprintf(_file, "\"");
    }
}

void JsonlTimeSeriesWriter::EscapeJsonString(const char* str) noexcept {
    if (str == nullptr) {
        return;
    }

    while (*str != '\0') {
        switch (*str) {
            case '\"': fprintf(_file, "\\\""); break;
            case '\\': fprintf(_file, "\\\\"); break;
            case '\n': fprintf(_file, "\\n"); break;
            case '\r': fprintf(_file, "\\r"); break;
            case '\t': fprintf(_file, "\\t"); break;
            default: fprintf(_file, "%c", *str); break;
        }
        ++str;
    }
}

} // namespace bench
} // namespace core
