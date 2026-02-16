#pragma once

// =============================================================================
// snapshot_system.hpp
// Tick-aligned periodic metric snapshots.
// =============================================================================

#include "measurement_system.hpp"
#include "metric_collector.hpp"
#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

class SnapshotMeasurementSystem : public IMeasurementSystem {
public:
    explicit SnapshotMeasurementSystem(u32 everyNTicks = 1) noexcept;
    ~SnapshotMeasurementSystem() noexcept override = default;

    const char* Name() const noexcept override { return "snapshot"; }
    const char* Description() const noexcept override {
        return "Tick-aligned periodic metric snapshots";
    }

    void OnRunStart(const char* experimentName, u64 seed, u32 repetitions) noexcept override;
    void OnRunEnd() noexcept override;
    void OnEvent(const Event& event) noexcept override;
    void GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept override;
    void PublishMetrics(MetricCollector& collector) noexcept override;

    void SetEveryNTicks(u32 n) noexcept { _everyNTicks = n; }
    [[nodiscard]] u32 GetEveryNTicks() const noexcept { return _everyNTicks; }

private:
    struct SnapshotRecord {
        u32 tickId;
        u64 opIndex;
        u64 liveCount;
        u64 liveBytes;
        u64 allocCount;
        u64 freeCount;
        bool hasLiveMetrics;
    };

    static constexpr u32 kMaxSnapshots = 100;
    static constexpr u32 kMaxMetricNameLength = 64;
    static constexpr u32 kMetricsPerSnapshot = 5;

    SnapshotRecord _snapshots[kMaxSnapshots];
    u32 _snapshotCount;
    u32 _everyNTicks;
    u32 _tickCounter;

    char _metricNames[kMaxSnapshots * kMetricsPerSnapshot][kMaxMetricNameLength];
    u32 _nameCount;
};

} // namespace bench
} // namespace core
