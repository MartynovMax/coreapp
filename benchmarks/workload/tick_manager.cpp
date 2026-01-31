// =============================================================================
// tick_manager.cpp
// Implementation of TickManager for periodic tick event emission.
// =============================================================================

#include "tick_manager.hpp"
#include "../common/high_res_timer.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_types.hpp"
#include "core/memory/memory_ops.hpp"
#include "phase_context.hpp"

namespace core::bench {

TickManager::TickManager(u64 tickInterval) noexcept
    : _tickInterval(tickInterval), _lastTickOpIndex(0) {}

void TickManager::OnOperation(const TickContext& ctx, const PhaseContext& phaseCtx) noexcept {
    if (_tickInterval == 0) return;
    if (const u64 opCount = ctx.opIndex + 1; opCount >= _lastTickOpIndex + _tickInterval) {
        EmitTickEvent(ctx, phaseCtx);
        _lastTickOpIndex = opCount;
    }
}

void TickManager::EmitTickEvent(const TickContext& ctx, const PhaseContext& phaseCtx) noexcept {
    if (!phaseCtx.eventSink) return;
    HighResTimer timer;
    Event evt{};
    evt.type = EventType::Tick;
    evt.experimentName = phaseCtx.experimentName;
    evt.phaseName = phaseCtx.phaseName;
    evt.repetitionId = phaseCtx.repetitionId;
    evt.timestamp = timer.Now();
    evt.data.tick.opIndex = ctx.opIndex;
    evt.data.tick.allocCount = ctx.allocCount;
    evt.data.tick.freeCount = ctx.freeCount;
    evt.data.tick.bytesAllocated = ctx.bytesAllocated;
    evt.data.tick.bytesFreed = ctx.bytesFreed;
    evt.data.tick.peakLiveCount = ctx.peakLiveCount;
    evt.data.tick.peakLiveBytes = ctx.peakLiveBytes;
    phaseCtx.eventSink->OnEvent(evt);
}

u64 TickManager::GetTickInterval() const noexcept {
    return _tickInterval;
}

} // namespace core::bench
