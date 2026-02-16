#include "counter_system.hpp"

namespace core {
namespace bench {

const MetricDescriptor CounterMeasurementSystem::_descriptors[] = {
    {
        .name = "counter.alloc_ops",
        .unit = "count",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "counter.free_ops",
        .unit = "count",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "counter.bytes_allocated",
        .unit = "bytes",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "counter.bytes_freed",
        .unit = "bytes",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "counter.peak_live_count",
        .unit = "count",
        .classification = MetricClassification::Optional,
        .capability = Capabilities::LiveSetTracking,
    },
    {
        .name = "counter.peak_live_bytes",
        .unit = "bytes",
        .classification = MetricClassification::Optional,
        .capability = Capabilities::LiveSetTracking,
    },
    {
        .name = "counter.final_live_count",
        .unit = "count",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "counter.final_live_bytes",
        .unit = "bytes",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
};

CounterMeasurementSystem::CounterMeasurementSystem() noexcept
    : _counters{}, _hasData(false) {
}

void CounterMeasurementSystem::OnRunStart(const char* /*experimentName*/, u64 /*seed*/, u32 /*repetitions*/) noexcept {
    _counters = Counters{};
    _hasData = false;
}

void CounterMeasurementSystem::OnRunEnd() noexcept {
    // Nothing to do on run end
}

void CounterMeasurementSystem::OnEvent(const Event& event) noexcept {
    if (!_enabled) return;

    switch (event.type) {
        case EventType::PhaseComplete: {
            // Extract counters from PhaseComplete payload
            const auto& payload = event.data.phaseComplete;

            _counters.allocOps = payload.allocCount;
            _counters.freeOps = payload.freeCount;
            _counters.bytesAllocated = payload.bytesAllocated;
            _counters.bytesFreed = payload.bytesFreed;
            _counters.peakLiveCount = payload.peakLiveCount;
            _counters.peakLiveBytes = payload.peakLiveBytes;
            _counters.finalLiveCount = payload.finalLiveCount;
            _counters.finalLiveBytes = payload.finalLiveBytes;

            // Check if peak metrics are valid (non-zero indicates tracking was active)
            _counters.hasPeakMetrics = (payload.peakLiveCount > 0 || payload.peakLiveBytes > 0);

            _hasData = true;
            break;
        }

        default:
            // Ignore other events
            break;
    }
}

void CounterMeasurementSystem::GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept {
    *outDescriptors = _descriptors;
    outCount = _descriptorCount;
}

void CounterMeasurementSystem::PublishMetrics(MetricCollector& collector) noexcept {
    if (!_enabled || !_hasData) {
        // Publish NA for all metrics if disabled or no data
        for (u32 i = 0; i < _descriptorCount; ++i) {
            collector.RegisterMetric(_descriptors[i]);
        }
        return;
    }

    // Publish exact metrics (always available)
    collector.PublishFromDescriptor(_descriptors[0], MetricValue::FromU64(_counters.allocOps));
    collector.PublishFromDescriptor(_descriptors[1], MetricValue::FromU64(_counters.freeOps));
    collector.PublishFromDescriptor(_descriptors[2], MetricValue::FromU64(_counters.bytesAllocated));
    collector.PublishFromDescriptor(_descriptors[3], MetricValue::FromU64(_counters.bytesFreed));

    // Publish peak metrics (optional: NA if tracking not active)
    if (_counters.hasPeakMetrics) {
        collector.PublishFromDescriptor(_descriptors[4], MetricValue::FromU64(_counters.peakLiveCount));
        collector.PublishFromDescriptor(_descriptors[5], MetricValue::FromU64(_counters.peakLiveBytes));
    } else {
        collector.RegisterMetric(_descriptors[4]); // NA
        collector.RegisterMetric(_descriptors[5]); // NA
    }

    // Publish final live-set metrics (exact)
    collector.PublishFromDescriptor(_descriptors[6], MetricValue::FromU64(_counters.finalLiveCount));
    collector.PublishFromDescriptor(_descriptors[7], MetricValue::FromU64(_counters.finalLiveBytes));
}

} // namespace bench
} // namespace core
