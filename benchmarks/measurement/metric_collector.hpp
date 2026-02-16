#pragma once

// =============================================================================
// metric_collector.hpp
// Central registry for metric publication.
// Schema-stable: all registered metrics appear in output (NA if unavailable).
// =============================================================================

#include "metric_descriptor.hpp"
#include "metric_value.hpp"
#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

struct MetricEntry {
    const char* name;
    MetricValue value;
    const MetricDescriptor* descriptor;
};

class MetricCollector {
public:
    static constexpr u32 kMaxMetrics = 256;

    MetricCollector() noexcept;
    ~MetricCollector() noexcept = default;

    void Publish(const char* name, MetricValue value, const MetricDescriptor* descriptor = nullptr) noexcept;

    void PublishFromDescriptor(const MetricDescriptor& descriptor, MetricValue value) noexcept {
        Publish(descriptor.name, value, &descriptor);
    }

    [[nodiscard]] u32 GetMetricCount() const noexcept { return _count; }
    [[nodiscard]] const MetricEntry* GetMetrics() const noexcept { return _metrics; }
    [[nodiscard]] const MetricEntry* FindMetric(const char* name) const noexcept;

    void Clear() noexcept;
    void RegisterMetric(const MetricDescriptor& descriptor) noexcept;

private:
    MetricEntry _metrics[kMaxMetrics];
    u32 _count;

    MetricEntry* FindOrCreateEntry(const char* name) noexcept;
};

} // namespace bench
} // namespace core
