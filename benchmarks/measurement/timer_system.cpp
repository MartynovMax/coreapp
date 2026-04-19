#include "timer_system.hpp"
#include "../../core/base/core_assert.hpp"

namespace core {
namespace bench {

const MetricDescriptor TimerMeasurementSystem::_descriptors[] = {
    {
        .name = "timer.phase_duration_ns",
        .unit = "nanoseconds",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "timer.repetition_duration_ns",
        .unit = "nanoseconds",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "timer.repetition_min_ns",
        .unit = "nanoseconds",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "timer.repetition_max_ns",
        .unit = "nanoseconds",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "timer.repetition_mean_ns",
        .unit = "nanoseconds",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "timer.repetition_median_ns",
        .unit = "nanoseconds",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
    {
        .name = "timer.repetition_p95_ns",
        .unit = "nanoseconds",
        .classification = MetricClassification::Exact,
        .capability = nullptr,
    },
};

TimerMeasurementSystem::TimerMeasurementSystem() noexcept
    : _repetitions{},
      _repetitionCount(0),
      _experimentName(nullptr),
      _expectedRepetitions(0),
      _currentPhaseDuration(0),
      _hasCurrentPhase(false) {
}

void TimerMeasurementSystem::OnRunStart(const char* experimentName, u64 /*seed*/, u32 repetitions) noexcept {
    _experimentName = experimentName;
    _expectedRepetitions = repetitions;
    _repetitionCount = 0;
    _currentPhaseDuration = 0;
    _hasCurrentPhase = false;
}

void TimerMeasurementSystem::OnRunEnd() noexcept {
    // Nothing to do on run end
}

void TimerMeasurementSystem::OnEvent(const Event& event) noexcept {
    if (!_enabled) return;

    switch (event.type) {
        case EventType::PhaseComplete: {
            // Only measure Steady phase (main measurement phase)
            // RampUp and BulkReclaim are setup/teardown — not part of timing data.
            if (event.data.phaseComplete.phaseType != PhaseType::Steady) {
                break;
            }

            // Capture phase duration
            _currentPhaseDuration = event.data.phaseComplete.durationNs;
            _hasCurrentPhase = true;

            if (_repetitionCount < kMaxRepetitions) {
                RepetitionRecord& record = _repetitions[_repetitionCount++];
                record.repetitionId = event.repetitionId;
                record.durationNs = _currentPhaseDuration;
            }
            break;
        }

        default:
            // Ignore other events
            break;
    }
}

void TimerMeasurementSystem::GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept {
    *outDescriptors = _descriptors;
    outCount = _descriptorCount;
}

void TimerMeasurementSystem::PublishMetrics(MetricCollector& collector) noexcept {
    if (!_enabled) {
        // Publish NA for all metrics if disabled
        for (u32 i = 0; i < _descriptorCount; ++i) {
            collector.RegisterMetric(_descriptors[i]);
        }
        return;
    }

    // Publish current phase duration (last recorded)
    if (_hasCurrentPhase) {
        collector.PublishFromDescriptor(_descriptors[0], MetricValue::FromU64(_currentPhaseDuration));
    } else {
        collector.RegisterMetric(_descriptors[0]); // NA
    }

    // Publish repetition statistics if we have data
    if (_repetitionCount == 0) {
        for (u32 i = 1; i < _descriptorCount; ++i) {
            collector.RegisterMetric(_descriptors[i]);
        }
        return;
    }

    collector.PublishFromDescriptor(_descriptors[1], MetricValue::FromU64(_repetitions[_repetitionCount - 1].durationNs));

    // Calculate and publish statistics
    u64 min, max, median, p95;
    f64 mean;
    CalculateStatistics(min, max, mean, median, p95);

    collector.PublishFromDescriptor(_descriptors[2], MetricValue::FromU64(min));
    collector.PublishFromDescriptor(_descriptors[3], MetricValue::FromU64(max));
    collector.PublishFromDescriptor(_descriptors[4], MetricValue::FromF64(mean));
    collector.PublishFromDescriptor(_descriptors[5], MetricValue::FromU64(median));
    collector.PublishFromDescriptor(_descriptors[6], MetricValue::FromU64(p95));
}

void TimerMeasurementSystem::CalculateStatistics(u64& outMin, u64& outMax, f64& outMean, u64& outMedian, u64& outP95) const noexcept {
    ASSERT(_repetitionCount > 0 && "Cannot calculate statistics on empty data");

    u64 durations[kMaxRepetitions];
    for (u32 i = 0; i < _repetitionCount; ++i) {
        durations[i] = _repetitions[i].durationNs;
    }

    for (u32 i = 0; i < _repetitionCount - 1; ++i) {
        for (u32 j = 0; j < _repetitionCount - i - 1; ++j) {
            if (durations[j] > durations[j + 1]) {
                u64 temp = durations[j];
                durations[j] = durations[j + 1];
                durations[j + 1] = temp;
            }
        }
    }

    outMin = durations[0];
    outMax = durations[_repetitionCount - 1];

    u64 sum = 0;
    for (u32 i = 0; i < _repetitionCount; ++i) {
        sum += durations[i];
    }
    outMean = static_cast<f64>(sum) / static_cast<f64>(_repetitionCount);

    u32 medianIdx = _repetitionCount / 2;
    if (_repetitionCount % 2 == 0 && _repetitionCount > 1) {
        outMedian = (durations[medianIdx - 1] + durations[medianIdx]) / 2;
    } else {
        outMedian = durations[medianIdx];
    }

    u32 p95Idx = static_cast<u32>(0.95 * static_cast<f64>(_repetitionCount));
    if (p95Idx >= _repetitionCount) {
        p95Idx = _repetitionCount - 1;
    }
    outP95 = durations[p95Idx];
}

} // namespace bench
} // namespace core
