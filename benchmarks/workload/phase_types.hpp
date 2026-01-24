#pragma once

namespace core {
namespace bench {

// PhaseType - Types of workload phases
enum class PhaseType {
    RampUp,
    Steady,
    Drain,
    BulkReclaim,
    Evolution,
    Custom,
};

} // namespace bench
} // namespace core
