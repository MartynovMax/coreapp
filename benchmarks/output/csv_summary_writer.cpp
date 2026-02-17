#include "csv_summary_writer.hpp"
#include "../common/string_utils.hpp"

namespace core {
namespace bench {

CsvSummaryWriter::CsvSummaryWriter(const char* filePath) noexcept
    : _filePath(filePath), _file(nullptr), _metadata{}, _headerWritten(false) {
}

CsvSummaryWriter::~CsvSummaryWriter() noexcept {
    Close();
}

bool CsvSummaryWriter::Open() noexcept {
    if (_file != nullptr) {
        return true;
    }

    if (_filePath == nullptr) {
        return false;
    }

    _file = fopen(_filePath, "w");
    return _file != nullptr;
}

void CsvSummaryWriter::Close() noexcept {
    if (_file != nullptr) {
        fclose(_file);
        _file = nullptr;
    }
}

void CsvSummaryWriter::SetMetadata(const RunMetadata& metadata) noexcept {
    _metadata = metadata;
}

void CsvSummaryWriter::WriteSummary(const MetricCollector& collector) noexcept {
    if (_file == nullptr) {
        return;
    }

    if (!_headerWritten) {
        WriteHeader();
        _headerWritten = true;
    }

    WriteRow(collector);
    fflush(_file);
}

void CsvSummaryWriter::WriteHeader() noexcept {
    // Schema version
    fprintf(_file, "schema_version,");

    // Run identifiers
    fprintf(_file, "run_id,");
    fprintf(_file, "experiment_name,");
    fprintf(_file, "experiment_category,");
    fprintf(_file, "allocator,");
    fprintf(_file, "seed,");
    fprintf(_file, "warmup_iterations,");
    fprintf(_file, "measured_repetitions,");

    // Environment and build metadata
    fprintf(_file, "run_timestamp_utc,");
    fprintf(_file, "os_name,");
    fprintf(_file, "os_version,");
    fprintf(_file, "arch,");
    fprintf(_file, "compiler_name,");
    fprintf(_file, "compiler_version,");
    fprintf(_file, "build_type,");
    fprintf(_file, "build_flags,");
    fprintf(_file, "cpu_model,");
    fprintf(_file, "cpu_cores_logical,");
    fprintf(_file, "cpu_cores_physical,");

    // Timer metrics
    fprintf(_file, "phase_duration_ns,");
    fprintf(_file, "repetition_min_ns,");
    fprintf(_file, "repetition_max_ns,");
    fprintf(_file, "repetition_mean_ns,");
    fprintf(_file, "repetition_median_ns,");
    fprintf(_file, "repetition_p95_ns,");

    // Counter metrics
    fprintf(_file, "alloc_ops_total,");
    fprintf(_file, "free_ops_total,");
    fprintf(_file, "bytes_allocated_total,");
    fprintf(_file, "bytes_freed_total,");
    fprintf(_file, "peak_live_count,");
    fprintf(_file, "peak_live_bytes,");
    fprintf(_file, "final_live_count,");
    fprintf(_file, "final_live_bytes,");

    // Derived metrics
    fprintf(_file, "ops_per_sec_mean,");
    fprintf(_file, "throughput_bytes_per_sec_mean");

    fprintf(_file, "\n");
}

void CsvSummaryWriter::WriteRow(const MetricCollector& collector) noexcept {
    // Schema version
    fprintf(_file, "summary.v2,");

    // Run identifiers
    if (_metadata.runId != nullptr) {
        fprintf(_file, "%s,", _metadata.runId);
    } else {
        fprintf(_file, ",");
    }

    if (_metadata.experimentName != nullptr) {
        fprintf(_file, "%s,", _metadata.experimentName);
    } else {
        fprintf(_file, ",");
    }

    if (_metadata.experimentCategory != nullptr) {
        fprintf(_file, "%s,", _metadata.experimentCategory);
    } else {
        fprintf(_file, ",");
    }

    if (_metadata.allocatorName != nullptr) {
        fprintf(_file, "%s,", _metadata.allocatorName);
    } else {
        fprintf(_file, ",");
    }

    fprintf(_file, "%llu,", static_cast<unsigned long long>(_metadata.seed));
    fprintf(_file, "%u,", _metadata.warmupIterations);
    fprintf(_file, "%u,", _metadata.measuredRepetitions);

    // Environment and build metadata
    fprintf(_file, "%s,", _metadata.runTimestampUtc);
    fprintf(_file, "%s,", _metadata.osName);
    fprintf(_file, "%s,", _metadata.osVersion);
    fprintf(_file, "%s,", _metadata.arch);
    fprintf(_file, "%s,", _metadata.compilerName);
    fprintf(_file, "%s,", _metadata.compilerVersion);
    fprintf(_file, "%s,", _metadata.buildType);
    fprintf(_file, "%s,", _metadata.buildFlags);
    fprintf(_file, "%s,", _metadata.cpuModel);
    fprintf(_file, "%u,", _metadata.cpuCoresLogical);
    fprintf(_file, "%u,", _metadata.cpuCoresPhysical);

    // Timer metrics
    WriteMetricValue(collector, "timer.phase_duration_ns");
    WriteMetricValue(collector, "timer.repetition_min_ns");
    WriteMetricValue(collector, "timer.repetition_max_ns");
    WriteMetricValue(collector, "timer.repetition_mean_ns");
    WriteMetricValue(collector, "timer.repetition_median_ns");
    WriteMetricValue(collector, "timer.repetition_p95_ns");

    // Counter metrics
    WriteMetricValue(collector, "counter.alloc_ops");
    WriteMetricValue(collector, "counter.free_ops");
    WriteMetricValue(collector, "counter.bytes_allocated");
    WriteMetricValue(collector, "counter.bytes_freed");
    WriteMetricValue(collector, "counter.peak_live_count");
    WriteMetricValue(collector, "counter.peak_live_bytes");
    WriteMetricValue(collector, "counter.final_live_count");
    WriteMetricValue(collector, "counter.final_live_bytes");

    fprintf(_file, ",");  // ops_per_sec_mean
    fprintf(_file, "");   // throughput_bytes_per_sec_mean (last column, no comma)

    fprintf(_file, "\n");
}

void CsvSummaryWriter::WriteMetricValue(const MetricCollector& collector, const char* metricName) noexcept {
    const MetricEntry* entry = collector.FindMetric(metricName);

    if (entry != nullptr && !entry->value.IsNA()) {
        if (entry->value.IsU64()) {
            fprintf(_file, "%llu,", static_cast<unsigned long long>(entry->value.AsU64()));
        } else if (entry->value.IsI64()) {
            fprintf(_file, "%lld,", static_cast<long long>(entry->value.AsI64()));
        } else if (entry->value.IsF64()) {
            fprintf(_file, "%.6f,", entry->value.AsF64());
        } else {
            fprintf(_file, ",");
        }
    } else {
        // NA or not found
        fprintf(_file, ",");
    }
}

} // namespace bench
} // namespace core
