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
    // Setup context pointers
    _ctx.lifetimeTracker = nullptr;
    _ctx.currentOpIndex = 0;
    _ctx.allocCount = 0;
    _ctx.freeCount = 0;
    _ctx.bytesAllocated = 0;
    _ctx.bytesFreed = 0;

    // Create OperationStream and LifetimeTracker for this phase
    if (_opStream) { delete _opStream; _opStream = nullptr; }
    if (_tracker) { delete _tracker; _tracker = nullptr; }
    _opStream = new OperationStream(_desc.params, *_ctx.rng);
    _tracker = new LifetimeTracker(_desc.params.lifetimeModel, _desc.params.maxLiveObjects, *_ctx.rng, _ctx.allocator);
    _ctx.lifetimeTracker = _tracker;

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
                core::AllocationRequest req;
                req.size = op.size;
                req.alignment = 8;
                if (void* ptr = _ctx.allocator->Allocate(req)) {
                    _tracker->Track(ptr, op.size, opIndex);
                    _ctx.allocCount++;
                    _ctx.bytesAllocated += op.size;
                }
            } else if (op.type == OpType::Free) {
                if (void* ptr = _tracker->SelectForFree()) {
                    core::AllocationInfo info;
                    info.ptr = ptr;
                    info.size = 0;
                    info.alignment = 8;
                    info.tag = 0;
                    _ctx.allocator->Deallocate(info);
                    _tracker->Remove(ptr);
                    _ctx.freeCount++;
                }
            }
        }
        opIndex++;
        if (_desc.completionCheck && _desc.completionCheck(_ctx)) {
            break;
        }
    }

    // Reclaim phase if needed
    if (_desc.reclaimMode == ReclaimMode::FreeAll) {
        _tracker->Clear();
    } else if (_desc.reclaimMode == ReclaimMode::Custom && _desc.reclaimCallback) {
        _desc.reclaimCallback(_ctx);
    }

    // Update stats
    _stats.allocCount = _ctx.allocCount;
    _stats.freeCount = _ctx.freeCount;
    _stats.bytesAllocated = _ctx.bytesAllocated;
    _stats.bytesFreed = _ctx.bytesFreed;
    _stats.peakLiveCount = _tracker->GetPeakCount();
    _stats.peakLiveBytes = _tracker->GetPeakBytes();

    // Emit OnPhaseComplete event if event sink exists
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseEnd;
        evt.phaseName = _desc.name;
        _eventSink->OnEvent(evt);
    }
}

const PhaseStats& PhaseExecutor::GetStats() const noexcept {
    return _stats;
}

} // namespace bench
} // namespace core
