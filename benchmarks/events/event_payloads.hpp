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

struct PhaseCompletePayload {
    PhaseStats stats;
    u64 finalLiveCount;
    u64 finalLiveBytes;
};

} // namespace bench
} // namespace core
