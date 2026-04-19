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
        // Sum of user-requested allocation sizes (op.size). NOT post-alignment actual bytes.
        // Equivalent to Task4 "requested_bytes". Rename would break CSV join keys.
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
    // Throughput
    {
        .name = "counter.throughput_ops_per_sec",
        .unit = "ops/sec",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "counter.throughput_bytes_per_sec",
        .unit = "bytes/sec",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    // Failure
    {
        .name = "counter.failed_alloc_count",
        .unit = "count",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    // Fallback (allocator-specific: segregated_list overflow to fallback allocator)
    {
        .name = "counter.fallback_count",
        .unit = "count",
        .classification = MetricClassification::Optional,
        .capability = Capabilities::AllocationTracking,
    },
    // Footprint
    {
        .name = "counter.reserved_bytes",
        .unit = "bytes",
        .classification = MetricClassification::Optional,
        .capability = Capabilities::FootprintTracking,
    },
    // Overhead proxy 1: reserved_bytes / peak_live_bytes
    {
        .name = "counter.overhead_ratio",
        .unit = "ratio",
        .classification = MetricClassification::Proxy,
        .capability = Capabilities::FootprintTracking,
    },
    // Overhead proxy 2: reserved_bytes / requested_bytes
    // Shows memory amplification relative to raw user-requested bytes.
    // Captures alignment/header overhead that overhead_ratio (vs live) does not.
    {
        .name = "counter.overhead_ratio_req",
        .unit = "ratio",
        .classification = MetricClassification::Proxy,
        .capability = Capabilities::FootprintTracking,
    },
    // Sanity check failures
    {
        .name = "counter.sanity_check_failures",
        .unit = "count",
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
            // Accumulate counters across all phases (RampUp + Steady + BulkReclaim).
            // Peak metrics use max(); additive counters use +=.
            const auto& payload = event.data.phaseComplete;

            _counters.allocOps       += payload.allocCount;
            _counters.freeOps        += payload.freeCount;
            _counters.bytesAllocated += payload.bytesAllocated;
            _counters.bytesFreed     += payload.bytesFreed;
            _counters.failedAllocCount += payload.failedAllocCount;
            _counters.sanityCheckFailures += payload.sanityCheckFailures;

            // Peak metrics: take max across all phases
            if (payload.peakLiveCount > _counters.peakLiveCount)
                _counters.peakLiveCount = payload.peakLiveCount;
            if (payload.peakLiveBytes > _counters.peakLiveBytes)
                _counters.peakLiveBytes = payload.peakLiveBytes;

            // Final live-set: always use the latest phase's final values
            _counters.finalLiveCount = payload.finalLiveCount;
            _counters.finalLiveBytes = payload.finalLiveBytes;

            // Fallback: accumulate (only meaningful for segregated_list Steady phase)
            _counters.fallbackCount += payload.fallbackCount;
            if (payload.hasFallbackTracking) _counters.hasFallbackTracking = true;

            // Reserved bytes: use the latest non-zero value (footprint at end of phase)
            if (payload.reservedBytes > 0)
                _counters.reservedBytes = payload.reservedBytes;

            // Throughput: use the Steady phase values (highest ops/sec with real work)
            if (payload.opsPerSec > _counters.throughputOpsPerSec) {
                _counters.throughputOpsPerSec = payload.opsPerSec;
                _counters.throughputBytesPerSec = payload.throughput;
            }

            // Check if peak metrics are valid (non-zero indicates tracking was active)
            if (payload.peakLiveCount > 0 || payload.peakLiveBytes > 0)
                _counters.hasPeakMetrics = true;

            // --- Runtime invariant checks (Task 10) ---
            // peak_live_bytes must be >= final_live_bytes
            if (_counters.hasPeakMetrics && payload.peakLiveBytes < payload.finalLiveBytes) {
                ++_counters.sanityCheckFailures;
            }
            // peak_live_count must be >= final_live_count
            if (_counters.hasPeakMetrics && payload.peakLiveCount < payload.finalLiveCount) {
                ++_counters.sanityCheckFailures;
            }
            // final_live_bytes cannot exceed total bytes ever allocated
            if (payload.finalLiveBytes > payload.bytesAllocated && payload.bytesAllocated > 0) {
                ++_counters.sanityCheckFailures;
            }

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

    // Compute overhead ratios from accumulated totals
    if (_counters.reservedBytes > 0 && _counters.peakLiveBytes > 0) {
        _counters.overheadRatio = static_cast<f64>(_counters.reservedBytes)
                                / static_cast<f64>(_counters.peakLiveBytes);
    }
    if (_counters.reservedBytes > 0 && _counters.bytesAllocated > 0) {
        _counters.overheadRatioReq = static_cast<f64>(_counters.reservedBytes)
                                   / static_cast<f64>(_counters.bytesAllocated);
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

    // Publish throughput
    collector.PublishFromDescriptor(_descriptors[8], MetricValue::FromF64(_counters.throughputOpsPerSec));
    collector.PublishFromDescriptor(_descriptors[9], MetricValue::FromF64(_counters.throughputBytesPerSec));

    // Publish failure count
    collector.PublishFromDescriptor(_descriptors[10], MetricValue::FromU64(_counters.failedAllocCount));

    // Publish fallback count: 0 is valid for allocators with fallback tracking (e.g. SegregatedList),
    // NA for allocators that don't support fallback (e.g. Malloc, Arena, Pool).
    if (_counters.hasFallbackTracking) {
        collector.PublishFromDescriptor(_descriptors[11], MetricValue::FromU64(_counters.fallbackCount));
    } else {
        collector.RegisterMetric(_descriptors[11]); // NA
    }

    // Publish footprint (optional: NA if not available for this allocator)
    if (_counters.reservedBytes > 0) {
        collector.PublishFromDescriptor(_descriptors[12], MetricValue::FromU64(_counters.reservedBytes));
    } else {
        collector.RegisterMetric(_descriptors[12]); // NA
    }

    // Publish overhead ratio proxy 1 (reserved / peak_live): NA if unavailable
    if (_counters.overheadRatio > 0.0) {
        collector.PublishFromDescriptor(_descriptors[13], MetricValue::FromF64(_counters.overheadRatio));
    } else {
        collector.RegisterMetric(_descriptors[13]); // NA
    }

    // Publish overhead ratio proxy 2 (reserved / requested_bytes): NA if unavailable
    if (_counters.overheadRatioReq > 0.0) {
        collector.PublishFromDescriptor(_descriptors[14], MetricValue::FromF64(_counters.overheadRatioReq));
    } else {
        collector.RegisterMetric(_descriptors[14]); // NA
    }

    // Publish sanity check failure count
    collector.PublishFromDescriptor(_descriptors[15], MetricValue::FromU64(_counters.sanityCheckFailures));
}

} // namespace bench
} // namespace core
