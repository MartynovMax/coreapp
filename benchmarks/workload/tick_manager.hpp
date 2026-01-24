#pragma once

// =============================================================================
// tick_manager.hpp
// Tick event management for periodic metrics and progress reporting.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "core/memory/core_allocator.hpp"

namespace core {
namespace bench {

struct TickContext {
    u64 opIndex;
    u64 allocCount;
    u64 freeCount;
    u64 bytesAllocated;
    u64 bytesFreed;
    u64 peakLiveCount;
    u64 peakLiveBytes;
};

class TickManager {
public:
    TickManager(u64 tickInterval) noexcept;
    void OnOperation(const TickContext& ctx) noexcept;
    void EmitTickEvent(const TickContext& ctx) noexcept;
    u64 GetTickInterval() const noexcept;
private:
    u64 _tickInterval;
    u64 _lastTickOpIndex;
};

} // namespace bench
} // namespace core
