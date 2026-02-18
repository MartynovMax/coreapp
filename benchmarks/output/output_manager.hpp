#pragma once

// =============================================================================
// output_manager.hpp
// Central coordinator for structured benchmark outputs.
// =============================================================================

#include "../events/event_sink.hpp"
#include "../measurement/metric_collector.hpp"
#include "output_config.hpp"
#include "run_metadata.hpp"
#include "jsonl_time_series_writer.hpp"
#include "csv_summary_writer.hpp"
#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

class OutputManager : public IEventSink {
public:
    explicit OutputManager(const OutputConfig& config) noexcept;
    ~OutputManager() noexcept;

    OutputManager(const OutputManager&) = delete;
    OutputManager& operator=(const OutputManager&) = delete;

    bool Initialize(const RunMetadata& metadata) noexcept;
    void Finalize(const MetricCollector& collector) noexcept;
    void SetMetadata(const RunMetadata& metadata) noexcept;

    void OnEvent(const Event& event) noexcept override;

private:
    OutputConfig _config;
    RunMetadata _metadata;

    JsonlTimeSeriesWriter* _timeSeriesWriter;
    CsvSummaryWriter* _summaryWriter;

    bool _initialized;
};

} // namespace bench
} // namespace core
