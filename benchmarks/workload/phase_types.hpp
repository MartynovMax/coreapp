#pragma once

namespace core::bench {

// PhaseType - Types of workload phases
enum class PhaseType {
    RampUp,
    Steady,
    Drain,
    BulkReclaim,
    Evolution,
    Custom,
};

} // namespace core::bench
