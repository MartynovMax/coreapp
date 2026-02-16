#pragma once

// =============================================================================
// metric_descriptor.hpp
// Metric metadata: name, unit, classification, capability.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

enum class MetricClassification : u8 {
    Exact,      // Guaranteed correct when available
    Optional,   // Depends on capability; may be NA
    Proxy,      // Approximation; explicitly marked as estimate
};

struct MetricDescriptor {
    const char* name;
    const char* unit;
    MetricClassification classification;
    const char* capability;  // nullptr if always available

    [[nodiscard]] constexpr bool IsAlwaysAvailable() const noexcept {
        return capability == nullptr;
    }

    [[nodiscard]] bool RequiresCapability(const char* cap) const noexcept;
};

namespace Capabilities {
    constexpr const char* LiveSetTracking = "live_set_tracking";
    constexpr const char* AllocationTracking = "allocation_tracking";
    constexpr const char* TimingAccuracy = "timing_accuracy";
}

} // namespace bench
} // namespace core
