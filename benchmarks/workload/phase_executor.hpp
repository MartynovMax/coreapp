#pragma once

// =============================================================================
// phase_executor.hpp
// Executes a workload phase using OperationStream and LifetimeTracker.
// =============================================================================

#include "workload_params.hpp"
#include "phase_descriptor.hpp"
#include "phase_context.hpp"
#include "operation_stream.hpp"
#include "lifetime_tracker.hpp"
#include "../events/event_sink.hpp"

namespace core {
namespace bench {

struct PhaseStats {
    u64 allocCount = 0;
    u64 freeCount = 0;
    u64 bytesAllocated = 0;
    u64 bytesFreed = 0;
    u64 peakLiveCount = 0;
    u64 peakLiveBytes = 0;
};

class PhaseExecutor {
public:
    PhaseExecutor(const PhaseDescriptor& desc,
                  PhaseContext& ctx,
                  IEventSink* eventSink = nullptr) noexcept;
    ~PhaseExecutor() noexcept;

    // Execute the phase (main entry point)
    void Execute();

    // Get stats after execution
    const PhaseStats& GetStats() const noexcept;

private:
    const PhaseDescriptor& _desc;
    PhaseContext& _ctx;
    IEventSink* _eventSink;
    PhaseStats _stats;
    OperationStream* _opStream;
    LifetimeTracker* _tracker;
};

} // namespace bench
} // namespace core
