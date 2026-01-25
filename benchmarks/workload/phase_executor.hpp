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
#include "../events/event_payloads.hpp"
#include "events/event_sink.hpp"

namespace core::bench {

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
    void ExecuteOperationAlloc(const Operation& op, u64 opIndex) const;
    void ExecuteOperationFree(u64 opIndex) const;
    void ExecuteReclaim();
    bool IsPhaseComplete() const;

    const PhaseDescriptor& _desc;
    PhaseContext& _ctx;
    IEventSink *_eventSink;
    PhaseStats _stats;
    OperationStream* _opStream;
    LifetimeTracker* _tracker;
    bool _ownsTracker = false;
};

} // namespace core::bench
