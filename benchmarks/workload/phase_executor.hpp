#pragma once

// =============================================================================
// phase_executor.hpp
// Executes a workload phase using OperationStream and LifetimeTracker.
// =============================================================================

#include "phase_descriptor.hpp"
#include "phase_context.hpp"
#include "operation_stream.hpp"
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
    [[nodiscard]] const PhaseStats& GetStats() const noexcept;

private:
    void SetupLifetimeTracker() noexcept;
    void ExecuteOperationAlloc(const Operation& op, u64 opIndex) const;
    void ExecuteOperationFree(u64 opIndex) const;
    void ExecuteReclaim();
    [[nodiscard]] bool IsPhaseComplete() const;

    const PhaseDescriptor& _desc;
    PhaseContext& _ctx;
    IEventSink *_eventSink;
    PhaseStats _stats;
    LifetimeTracker* _tracker;
    bool _ownsTracker = false;
    bool _needsTracker;
};

} // namespace core::bench
