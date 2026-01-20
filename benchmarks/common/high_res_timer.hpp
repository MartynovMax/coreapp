#pragma once

// =============================================================================
// high_res_timer.hpp
// High-resolution timer for benchmarks.
//
// Platform support:
//   - Windows: QueryPerformanceCounter
//   - Linux/macOS: clock_gettime(CLOCK_MONOTONIC)
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// HighResTimer - Platform-agnostic high-resolution timer
// ----------------------------------------------------------------------------

class HighResTimer {
public:
    HighResTimer() noexcept;
    ~HighResTimer() noexcept = default;

    // Get current timestamp in nanoseconds
    u64 Now() const noexcept;

    // Get elapsed time in nanoseconds since start
    u64 Elapsed(u64 start) const noexcept;

private:
    u64 _frequency;
};

} // namespace bench
} // namespace core
