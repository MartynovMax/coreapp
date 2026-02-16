#pragma once

// =============================================================================
// measurement_system.hpp
// Base interface for pluggable measurement systems.
// =============================================================================

#include "../events/event_types.hpp"
#include "../events/event_sink.hpp"
#include "metric_descriptor.hpp"
#include "metric_value.hpp"

namespace core {
namespace bench {

class MetricCollector;

// ----------------------------------------------------------------------------
// IMeasurementSystem
// ----------------------------------------------------------------------------

class IMeasurementSystem : public IEventSink {
public:
    virtual ~IMeasurementSystem() = default;

    virtual const char* Name() const noexcept = 0;
    virtual const char* Description() const noexcept = 0;

    virtual void OnRunStart(const char* experimentName, u64 seed, u32 repetitions) noexcept = 0;
    virtual void OnRunEnd() noexcept = 0;

    void OnEvent(const Event& event) noexcept override = 0;

    virtual void GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept = 0;
    virtual void PublishMetrics(MetricCollector& collector) noexcept = 0;

    virtual void SetEnabled(bool enabled) noexcept { _enabled = enabled; }
    virtual bool IsEnabled() const noexcept { return _enabled; }

protected:
    bool _enabled = true;
};

} // namespace bench
} // namespace core
