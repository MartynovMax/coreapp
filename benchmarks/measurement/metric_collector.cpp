#include "metric_collector.hpp"
#include "../../core/base/core_assert.hpp"
#include "../common/string_utils.hpp"

namespace core {
namespace bench {

MetricCollector::MetricCollector() noexcept : _metrics{}, _count(0) {
}

void MetricCollector::Publish(const char* name, MetricValue value, const MetricDescriptor* descriptor) noexcept {
    ASSERT(name != nullptr && "Metric name cannot be null");

    MetricEntry* entry = FindOrCreateEntry(name);
    if (entry == nullptr) {
        ASSERT(false && "MetricCollector: max metrics exceeded");
        return;
    }

    entry->value = value;
    if (descriptor != nullptr) {
        entry->descriptor = descriptor;
    }
}

const MetricEntry* MetricCollector::FindMetric(const char* name) const noexcept {
    if (name == nullptr) return nullptr;

    for (u32 i = 0; i < _count; ++i) {
        if (_metrics[i].name != nullptr && StringsEqual(_metrics[i].name, name)) {
            return &_metrics[i];
        }
    }

    return nullptr;
}

void MetricCollector::Clear() noexcept {
    for (u32 i = 0; i < _count; ++i) {
        _metrics[i] = MetricEntry{};
    }
    _count = 0;
}

void MetricCollector::RegisterMetric(const MetricDescriptor& descriptor) noexcept {
    ASSERT(descriptor.name != nullptr && "Descriptor name cannot be null");

    if (FindMetric(descriptor.name) != nullptr) {
        return;
    }

    MetricEntry* entry = FindOrCreateEntry(descriptor.name);
    if (entry == nullptr) {
        ASSERT(false && "MetricCollector: max metrics exceeded during registration");
        return;
    }

    entry->value = MetricValue::NA();
    entry->descriptor = &descriptor;
}

MetricEntry* MetricCollector::FindOrCreateEntry(const char* name) noexcept {
    ASSERT(name != nullptr);

    for (u32 i = 0; i < _count; ++i) {
        if (_metrics[i].name != nullptr && StringsEqual(_metrics[i].name, name)) {
            return &_metrics[i];
        }
    }

    if (_count >= kMaxMetrics) {
        return nullptr;
    }

    MetricEntry* entry = &_metrics[_count++];
    entry->name = name;
    entry->value = MetricValue::NA();
    entry->descriptor = nullptr;

    return entry;
}

} // namespace bench
} // namespace core
