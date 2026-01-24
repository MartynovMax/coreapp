// =============================================================================
// phase_executor.cpp
// Implementation of PhaseExecutor for workload phase execution
// =============================================================================

#include "phase_executor.hpp"

namespace core {
namespace bench {

PhaseExecutor::PhaseExecutor(const PhaseDescriptor& desc,
                             PhaseContext& ctx,
                             IEventSink* eventSink) noexcept
    : _desc(desc), _ctx(ctx), _eventSink(eventSink), _stats{}, _opStream(nullptr), _tracker(nullptr)
{
}

PhaseExecutor::~PhaseExecutor() noexcept {
    if (_opStream) {
        delete _opStream;
        _opStream = nullptr;
    }
    if (_tracker) {
        delete _tracker;
        _tracker = nullptr;
    }
}

void PhaseExecutor::Execute() {
}

const PhaseStats& PhaseExecutor::GetStats() const noexcept {
    return _stats;
}

} // namespace bench
} // namespace core
