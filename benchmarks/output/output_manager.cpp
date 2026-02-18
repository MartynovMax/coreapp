#include "output_manager.hpp"
#include "../common/string_utils.hpp"
#include <stdio.h>

namespace core {
namespace bench {

OutputManager::OutputManager(const OutputConfig& config) noexcept
    : _config(config),
      _metadata{},
      _timeSeriesWriter(nullptr),
      _summaryWriter(nullptr),
      _initialized(false) {
}

OutputManager::~OutputManager() noexcept {
    delete _timeSeriesWriter;
    delete _summaryWriter;
}

bool OutputManager::Initialize(const RunMetadata& metadata) noexcept {
    if (_initialized) {
        return true;
    }

    _metadata = metadata;

    // Create time-series writer if enabled
    if (_config.enableTimeSeriesOutput) {
        if (_config.outputPath != nullptr) {
            // Append .jsonl extension
            static char timeSeriesPath[512];
            snprintf(timeSeriesPath, sizeof(timeSeriesPath), "%s.jsonl", _config.outputPath);

            _timeSeriesWriter = new JsonlTimeSeriesWriter(timeSeriesPath);
            if (!_timeSeriesWriter->Open()) {
                if (_config.verbose) {
                    printf("[OutputManager] Failed to open time-series output: %s\n", timeSeriesPath);
                }
                delete _timeSeriesWriter;
                _timeSeriesWriter = nullptr;
            } else {
                _timeSeriesWriter->SetMetadata(_metadata);
                if (_config.verbose) {
                    printf("[OutputManager] Time-series output: %s\n", timeSeriesPath);
                }
            }
        }
    }

    // Create summary writer if enabled
    if (_config.enableSummaryOutput) {
        if (_config.outputPath != nullptr) {
            // Append .csv extension
            static char summaryPath[512];
            snprintf(summaryPath, sizeof(summaryPath), "%s.csv", _config.outputPath);

            _summaryWriter = new CsvSummaryWriter(summaryPath);
            if (!_summaryWriter->Open()) {
                if (_config.verbose) {
                    printf("[OutputManager] Failed to open summary output: %s\n", summaryPath);
                }
                delete _summaryWriter;
                _summaryWriter = nullptr;
            } else {
                _summaryWriter->SetMetadata(_metadata);
                if (_config.verbose) {
                    printf("[OutputManager] Summary output: %s\n", summaryPath);
                }
            }
        }
    }

    _initialized = true;
    return true;
}

void OutputManager::Finalize(const MetricCollector& collector) noexcept {
    if (_summaryWriter != nullptr && _summaryWriter->IsOpen()) {
        _summaryWriter->WriteSummary(collector);
    }
}

void OutputManager::SetMetadata(const RunMetadata& metadata) noexcept {
    _metadata = metadata;
    
    if (_timeSeriesWriter != nullptr) {
        _timeSeriesWriter->SetMetadata(_metadata);
    }
    if (_summaryWriter != nullptr) {
        _summaryWriter->SetMetadata(_metadata);
    }
}

void OutputManager::OnEvent(const Event& event) noexcept {
    if (_timeSeriesWriter != nullptr && _timeSeriesWriter->IsOpen()) {
        _timeSeriesWriter->WriteEvent(event);
    }
}

} // namespace bench
} // namespace core
