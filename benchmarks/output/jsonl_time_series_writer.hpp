#pragma once

// =============================================================================
// jsonl_time_series_writer.hpp
// JSONL time-series writer for benchmark events.
// =============================================================================

#include "../events/event_types.hpp"
#include "run_metadata.hpp"
#include "../../core/base/core_types.hpp"
#include <stdio.h>

namespace core {
namespace bench {

class JsonlTimeSeriesWriter {
public:
    explicit JsonlTimeSeriesWriter(const char* filePath) noexcept;
    ~JsonlTimeSeriesWriter() noexcept;

    JsonlTimeSeriesWriter(const JsonlTimeSeriesWriter&) = delete;
    JsonlTimeSeriesWriter& operator=(const JsonlTimeSeriesWriter&) = delete;

    bool Open() noexcept;
    void Close() noexcept;
    bool IsOpen() const noexcept { return _file != nullptr; }

    void SetMetadata(const RunMetadata& metadata) noexcept;
    void WriteEvent(const Event& event) noexcept;

private:
    const char* _filePath;
    FILE* _file;
    RunMetadata _metadata;

    void WriteEventRecord(const Event& event) noexcept;
    void WriteTickRecord(const Event& event) noexcept;
    void WriteWarningRecord(const Event& event) noexcept;

    void WriteCommonFields(const Event& event, const char* recordType) noexcept;
    void EscapeJsonString(const char* str) noexcept;
};

} // namespace bench
} // namespace core
