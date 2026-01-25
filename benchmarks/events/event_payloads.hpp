#pragma once

#include "../../core/base/core_types.hpp"
#include "../workload/phase_types.hpp"

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
    const char* experimentName;
    const char* phaseName;
    PhaseType phaseType;
    u32 repetitionId;
    u64 startTimestamp;
    u64 endTimestamp;
    u64 durationNs;
    u64 allocCount;
    u64 freeCount;
    u64 totalOperations;
    u64 bytesAllocated;
    u64 bytesFreed;
    u64 peakLiveCount;
    u64 peakLiveBytes;
    u64 finalLiveCount;
    u64 finalLiveBytes;
    double opsPerSec;
    double throughput;
};

// TickPayload - metrics for tick events
struct TickPayload {
    u64 opIndex;
    u64 allocCount;
    u64 freeCount;
    u64 bytesAllocated;
    u64 bytesFreed;
    u64 peakLiveCount;
    u64 peakLiveBytes;
};

} // namespace bench
} // namespace core
