#pragma once

// =============================================================================
// csv_summary_writer.hpp
// CSV summary writer for aggregated benchmark metrics.
// =============================================================================

#include "../measurement/metric_collector.hpp"
#include "run_metadata.hpp"
#include "../../core/base/core_types.hpp"
#include <stdio.h>

namespace core {
namespace bench {

class CsvSummaryWriter {
public:
    explicit CsvSummaryWriter(const char* filePath) noexcept;
    ~CsvSummaryWriter() noexcept;

    CsvSummaryWriter(const CsvSummaryWriter&) = delete;
    CsvSummaryWriter& operator=(const CsvSummaryWriter&) = delete;

    bool Open() noexcept;
    void Close() noexcept;
    bool IsOpen() const noexcept { return _file != nullptr; }

    void SetMetadata(const RunMetadata& metadata) noexcept;
    void WriteSummary(const MetricCollector& collector) noexcept;

private:
    const char* _filePath;
    FILE* _file;
    RunMetadata _metadata;
    bool _headerWritten;

    void WriteHeader() noexcept;
    void WriteRow(const MetricCollector& collector) noexcept;

    void WriteMetricValue(const MetricCollector& collector, const char* metricName) noexcept;
    void WriteLastMetricValue(const MetricCollector& collector, const char* metricName) noexcept;
};

} // namespace bench
} // namespace core
