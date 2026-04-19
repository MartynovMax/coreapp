#pragma once

// =============================================================================
// counter_system.hpp
// Operation and byte counters with live-set tracking.
// =============================================================================

#include "measurement_system.hpp"
#include "metric_collector.hpp"
#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

class CounterMeasurementSystem : public IMeasurementSystem {
public:
    CounterMeasurementSystem() noexcept;
    ~CounterMeasurementSystem() noexcept override = default;

    const char* Name() const noexcept override { return "counter"; }
    const char* Description() const noexcept override {
        return "Operation and byte counters with live-set tracking";
    }

    void OnRunStart(const char* experimentName, u64 seed, u32 repetitions) noexcept override;
    void OnRunEnd() noexcept override;
    void OnEvent(const Event& event) noexcept override;
    void GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept override;
    void PublishMetrics(MetricCollector& collector) noexcept override;

private:
    struct Counters {
        u64 allocOps;
        u64 freeOps;
        // Sum of user-requested allocation sizes (op.size), NOT post-alignment actual bytes.
        // Corresponds to Task4 "requested_bytes".
        u64 bytesAllocated;
        u64 bytesFreed;
        u64 peakLiveCount;
        u64 peakLiveBytes;
        u64 finalLiveCount;
        u64 finalLiveBytes;
        u64 failedAllocCount;
        u64 fallbackCount;   // segregated_list: times routed to fallback allocator
        u64 reservedBytes;
        f64 throughputOpsPerSec;
        f64 throughputBytesPerSec;
        f64 overheadRatio;          // reserved_bytes / peak_live_bytes (0 if unavailable)
        f64 overheadRatioReq;       // reserved_bytes / requested_bytes (0 if unavailable)
        bool hasPeakMetrics;
        u32 sanityCheckFailures;
    };

    Counters _counters;
    bool _hasData;

    static const MetricDescriptor _descriptors[];
    static constexpr u32 _descriptorCount = 16;
};

} // namespace bench
} // namespace core
