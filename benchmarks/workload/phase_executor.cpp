// =============================================================================
// phase_executor.cpp
// Implementation of PhaseExecutor for workload phase execution
// =============================================================================

#include "phase_executor.hpp"

#include "core/memory/memory_ops.hpp"

#include "events/event_sink.hpp"
#include "events/event_types.hpp"

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
    // Create OperationStream and LifetimeTracker for this phase
    if (_opStream) { delete _opStream; _opStream = nullptr; }
    if (_tracker) { delete _tracker; _tracker = nullptr; }
    _opStream = new OperationStream(_desc.params, *_ctx.rng);
    _tracker = new LifetimeTracker(_desc.params.lifetimeModel, _desc.params.maxLiveObjects, *_ctx.rng, _ctx.allocator);

    // Setup context pointers
    _ctx.lifetimeTracker = _tracker;
    _ctx.eventSink = _eventSink;
    _ctx.userData = _desc.userData;
    _ctx.currentOpIndex = 0;
    _ctx.allocCount = 0;
    _ctx.freeCount = 0;
    _ctx.bytesAllocated = 0;
    _ctx.bytesFreed = 0;

    // Emit OnPhaseBegin event if event sink exists
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseBegin;
        evt.phaseName = _desc.name;
        _eventSink->OnEvent(evt);
    }

    // Main operation loop
    u64 opIndex = 0;
    while (_opStream->HasNext()) {
        const Operation op = _opStream->Next();
        _ctx.currentOpIndex = opIndex;
        if (_desc.customOperation) {
            _desc.customOperation(_ctx, opIndex);
        } else {
            if (op.type == OpType::Alloc) {
                ExecuteOperationAlloc(op, opIndex);
            } else if (op.type == OpType::Free) {
                ExecuteOperationFree(opIndex);
            }
        }
        opIndex++;
        if (IsPhaseComplete()) {
            break;
        }
    }

    // Reclaim phase if needed
    ExecuteReclaim();

    // Update stats
    _stats.allocCount = _ctx.allocCount;
    _stats.freeCount = _ctx.freeCount;
    _stats.bytesAllocated = _ctx.bytesAllocated;
    _stats.bytesFreed = _ctx.bytesFreed;
    _stats.peakLiveCount = _tracker->GetPeakCount();
    _stats.peakLiveBytes = _tracker->GetPeakBytes();

    // PhaseComplete event with payload
    if (_eventSink) {
        Event evt;
        core::memory_zero(&evt, sizeof(evt));
        evt.type = EventType::PhaseComplete;
        evt.phaseName = _desc.name;
        evt.data.phaseComplete.stats = _stats;
        evt.data.phaseComplete.finalLiveCount = _tracker->GetLiveCount();
        evt.data.phaseComplete.finalLiveBytes = _tracker->GetLiveBytes();
        _eventSink->OnEvent(evt);
    }

    // PhaseEnd marker
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseEnd;
        evt.phaseName = _desc.name;
        _eventSink->OnEvent(evt);
    }
}

void PhaseExecutor::ExecuteOperationAlloc(const Operation& op, u64 opIndex) const {
    core::AllocationRequest req{};
    req.size = op.size;
    req.alignment = op.alignment;
    req.tag = op.tag;
    req.flags = op.flags;

    if (void* ptr = _ctx.allocator->Allocate(req)) {
        _tracker->Track(ptr, op.size, op.alignment, op.tag, opIndex);
        _ctx.allocCount++;
        _ctx.bytesAllocated += op.size;
    }
}

void PhaseExecutor::ExecuteOperationFree(u64 opIndex) const {
    AllocInfo a{};
    if (!_tracker->PopForFree(a)) {
        return;
    }
    core::AllocationInfo info{};
    info.ptr = a.ptr;
    info.size = a.size;
    info.alignment = a.alignment;
    info.tag = a.tag;
    _ctx.allocator->Deallocate(info);
    _ctx.freeCount++;
    _ctx.bytesFreed += a.size;
}

void PhaseExecutor::ExecuteReclaim() const {
    if (_desc.reclaimMode == ReclaimMode::None) {
        return;
    } else if (_desc.reclaimMode == ReclaimMode::FreeAll) {
        _tracker->Clear();
    } else if (_desc.reclaimMode == ReclaimMode::Custom && _desc.reclaimCallback) {
        _desc.reclaimCallback(_ctx);
    }
}

bool PhaseExecutor::IsPhaseComplete() const {
    if (_desc.completionCheck) {
        return _desc.completionCheck(_ctx);
    }
    return false;
}

const PhaseStats& PhaseExecutor::GetStats() const noexcept {
    return _stats;
}

} // namespace bench
} // namespace core
