#include "snapshot_system.hpp"
#include <cstdio>

namespace core {
namespace bench {

SnapshotMeasurementSystem::SnapshotMeasurementSystem(u32 everyNTicks) noexcept
    : _snapshots{},
      _snapshotCount(0),
      _everyNTicks(everyNTicks),
      _tickCounter(0),
      _metricNames{},
      _nameCount(0) {
}

void SnapshotMeasurementSystem::OnRunStart(const char* /*experimentName*/, u64 /*seed*/, u32 /*repetitions*/) noexcept {
    _snapshotCount = 0;
    _tickCounter = 0;
    _nameCount = 0;
}

void SnapshotMeasurementSystem::OnRunEnd() noexcept {
    // Nothing to do on run end
}

void SnapshotMeasurementSystem::OnEvent(const Event& event) noexcept {
    if (!_enabled) return;

    switch (event.type) {
        case EventType::Tick: {
            _tickCounter++;

            if (_everyNTicks > 0 && (_tickCounter % _everyNTicks) != 0) {
                return;
            }

            if (_snapshotCount >= kMaxSnapshots) {
                return;
            }

            const auto& payload = event.data.tick;

            SnapshotRecord& snapshot = _snapshots[_snapshotCount++];
            snapshot.tickId = _tickCounter;
            snapshot.opIndex = payload.opIndex;
            snapshot.liveCount = payload.peakLiveCount;
            snapshot.liveBytes = payload.peakLiveBytes;
            snapshot.allocCount = payload.allocCount;
            snapshot.freeCount = payload.freeCount;
            snapshot.hasLiveMetrics = (payload.peakLiveCount > 0 || payload.peakLiveBytes > 0);
            break;
        }

        default:
            // Ignore other events
            break;
    }
}

void SnapshotMeasurementSystem::GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept {
    *outDescriptors = nullptr;
    outCount = 0;
}

void SnapshotMeasurementSystem::PublishMetrics(MetricCollector& collector) noexcept {
    if (!_enabled || _snapshotCount == 0) {
        return;
    }

    _nameCount = 0;

    for (u32 i = 0; i < _snapshotCount; ++i) {
        const SnapshotRecord& snapshot = _snapshots[i];

        if (_nameCount + kMetricsPerSnapshot > kMaxSnapshots * kMetricsPerSnapshot) {
            break;
        }

        char* opIndexName = _metricNames[_nameCount++];
        std::snprintf(opIndexName, kMaxMetricNameLength, "snapshot.tick_%u.op_index", snapshot.tickId);
        collector.Publish(opIndexName, MetricValue::FromU64(snapshot.opIndex), nullptr);

        char* liveCountName = _metricNames[_nameCount++];
        std::snprintf(liveCountName, kMaxMetricNameLength, "snapshot.tick_%u.live_count", snapshot.tickId);
        if (snapshot.hasLiveMetrics) {
            collector.Publish(liveCountName, MetricValue::FromU64(snapshot.liveCount), nullptr);
        } else {
            collector.Publish(liveCountName, MetricValue::NA(), nullptr);
        }

        char* liveBytesName = _metricNames[_nameCount++];
        std::snprintf(liveBytesName, kMaxMetricNameLength, "snapshot.tick_%u.live_bytes", snapshot.tickId);
        if (snapshot.hasLiveMetrics) {
            collector.Publish(liveBytesName, MetricValue::FromU64(snapshot.liveBytes), nullptr);
        } else {
            collector.Publish(liveBytesName, MetricValue::NA(), nullptr);
        }

        char* allocCountName = _metricNames[_nameCount++];
        std::snprintf(allocCountName, kMaxMetricNameLength, "snapshot.tick_%u.alloc_count", snapshot.tickId);
        collector.Publish(allocCountName, MetricValue::FromU64(snapshot.allocCount), nullptr);

        char* freeCountName = _metricNames[_nameCount++];
        std::snprintf(freeCountName, kMaxMetricNameLength, "snapshot.tick_%u.free_count", snapshot.tickId);
        collector.Publish(freeCountName, MetricValue::FromU64(snapshot.freeCount), nullptr);
    }
}

} // namespace bench
} // namespace core
