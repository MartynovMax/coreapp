#pragma once

// =============================================================================
// timer_system.hpp
// Phase and repetition duration tracking with aggregation.
// =============================================================================

#include "measurement_system.hpp"
#include "metric_collector.hpp"
#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

class TimerMeasurementSystem : public IMeasurementSystem {
public:
    TimerMeasurementSystem() noexcept;
    ~TimerMeasurementSystem() noexcept override = default;

    const char* Name() const noexcept override { return "timer"; }
    const char* Description() const noexcept override {
        return "Phase and repetition duration tracking with aggregation";
    }

    void OnRunStart(const char* experimentName, u64 seed, u32 repetitions) noexcept override;
    void OnRunEnd() noexcept override;
    void OnEvent(const Event& event) noexcept override;
    void GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept override;
    void PublishMetrics(MetricCollector& collector) noexcept override;

private:
    struct RepetitionRecord {
        u32 repetitionId;
        u64 durationNs;
    };

    static constexpr u32 kMaxRepetitions = 1024;

    RepetitionRecord _repetitions[kMaxRepetitions];
    u32 _repetitionCount;
    const char* _experimentName;
    u32 _expectedRepetitions;
    u64 _currentPhaseDuration;
    bool _hasCurrentPhase;

    static const MetricDescriptor _descriptors[];
    static constexpr u32 _descriptorCount = 7;

    void CalculateStatistics(u64& outMin, u64& outMax, f64& outMean, u64& outMedian, u64& outP95) const noexcept;
};

} // namespace bench
} // namespace core
