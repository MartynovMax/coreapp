#pragma once

#include "../../core/base/core_types.hpp"
#include "../workload/phase_types.hpp"

namespace core {
namespace bench {

struct PhaseStats {
    u64 allocCount;
    u64 freeCount;
    u64 issuedOpCount;      
    u64 forcedAllocCount; 
    u64 noopFreeCount;    
    u64 bytesAllocated;
    u64 bytesFreed;
    u64 peakLiveCount;
    u64 peakLiveBytes;
    u64 preReclaimLiveCount;
    u64 preReclaimLiveBytes;
    u64 finalLiveCount;
    u64 finalLiveBytes;
    u64 internalFreeCount;
    u64 internalBytesFreed;
    u64 reclaimFreeCount;
    u64 reclaimBytesFreed;
    u64 totalFreeCount;
    u64 totalBytesFreed;
    u64 failedAllocCount;
};

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
    u64 issuedOpCount; 
    u64 forcedAllocCount;
    u64 noopFreeCount;
    u64 bytesAllocated;
    u64 bytesFreed;
    u64 peakLiveCount;
    u64 peakLiveBytes;
    u64 preReclaimLiveCount;
    u64 preReclaimLiveBytes;
    u64 finalLiveCount;
    u64 finalLiveBytes;
    u64 internalFreeCount;
    u64 internalBytesFreed;
    u64 reclaimFreeCount;
    u64 reclaimBytesFreed;
    u64 totalFreeCount;
    u64 totalBytesFreed;
    u64 failedAllocCount;
    f64 opsPerSec;
    f64 throughput;
};

struct TickPayload {
    u64 opIndex;
    u64 allocCount;
    u64 freeCount;
    u64 bytesAllocated;
    u64 bytesFreed;
    u64 peakLiveCount;
    u64 peakLiveBytes;
};

struct AllocationPayload {
    u64 allocationId;
    void* ptr;
    u64 size;
    u64 alignment;
    u64 tag;
    bool success;
    u64 opIndex;
};

struct FreePayload {
    u64 allocationId;
    void* ptr;
    u64 size;
    u64 alignment;
    u64 tag;
    u64 opIndex;
};

struct AllocatorResetPayload {
    const char* allocatorName;
    u64 freedCount;
    u64 freedBytes;
};

enum class FailureReason : u32 {
    OutOfMemory,
    AssertionFailed,
    MaxIterationsReached,
    InvalidState,
    Custom,
};

struct FailurePayload {
    FailureReason reason;
    const char* message;
    u64 opIndex;
    bool isRecoverable;
};

} // namespace bench
} // namespace core
