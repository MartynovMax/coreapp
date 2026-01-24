// =============================================================================
// tick_manager.cpp
// Implementation of TickManager for periodic tick event emission.
// =============================================================================

#include "tick_manager.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_types.hpp"
#include "core/memory/memory_ops.hpp"
#include "phase_context.hpp"

namespace core {
namespace bench {

TickManager::TickManager(u64 tickInterval) noexcept
    : _tickInterval(tickInterval), _lastTickOpIndex(0) {}

void TickManager::OnOperation(const TickContext& ctx, const PhaseContext& phaseCtx) noexcept {
    if (_tickInterval == 0) return;
    if (ctx.opIndex >= _lastTickOpIndex + _tickInterval) {
        EmitTickEvent(ctx, phaseCtx);
        _lastTickOpIndex = ctx.opIndex;
    }
}

void TickManager::EmitTickEvent(const TickContext& ctx, const PhaseContext& phaseCtx) noexcept {
    if (IEventSink* sink = phaseCtx.eventSink) {
        Event evt;
        core::memory_zero(&evt, sizeof(evt));
        evt.type = EventType::Tick;
        evt.phaseName = phaseCtx.phaseName;
        evt.data.tick.opIndex = ctx.opIndex;
        evt.data.tick.allocCount = ctx.allocCount;
        evt.data.tick.freeCount = ctx.freeCount;
        evt.data.tick.bytesAllocated = ctx.bytesAllocated;
        evt.data.tick.bytesFreed = ctx.bytesFreed;
        evt.data.tick.peakLiveCount = ctx.peakLiveCount;
        evt.data.tick.peakLiveBytes = ctx.peakLiveBytes;
        sink->OnEvent(evt);
    }
}

u64 TickManager::GetTickInterval() const noexcept {
    return _tickInterval;
}

} // namespace bench
} // namespace core
