#pragma once

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

struct PhaseStats {
    u64 allocCount;
    u64 freeCount;
    u64 bytesAllocated;
    u64 bytesFreed;
    u64 peakLiveCount;
    u64 peakLiveBytes;
};

// PhaseCompletePayload - full phase completion metrics for event sink
struct PhaseCompletePayload {
    const char* experimentName = nullptr;
    const char* phaseName = nullptr;
    PhaseType phaseType = PhaseType::Steady;
    u32 repetitionId = 0;
    u64 startTimestamp = 0;
    u64 endTimestamp = 0;
    u64 durationNs = 0;
    u64 allocCount = 0;
    u64 freeCount = 0;
    u64 totalOperations = 0;
    u64 bytesAllocated = 0;
    u64 bytesFreed = 0;
    u64 peakLiveCount = 0;
    u64 peakLiveBytes = 0;
    u64 finalLiveCount = 0;
    u64 finalLiveBytes = 0;
    double opsPerSec = 0.0;
    double throughput = 0.0;
};

// TickPayload - metrics for tick events
struct TickPayload {
    u64 opIndex = 0;
    u64 allocCount = 0;
    u64 freeCount = 0;
    u64 bytesAllocated = 0;
    u64 bytesFreed = 0;
    u64 peakLiveCount = 0;
    u64 peakLiveBytes = 0;
};

} // namespace bench
} // namespace core
