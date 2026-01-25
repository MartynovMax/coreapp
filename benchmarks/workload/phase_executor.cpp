// =============================================================================
// phase_executor.cpp
// Implementation of PhaseExecutor for workload phase execution
// =============================================================================

#include "phase_executor.hpp"

#include "tick_manager.hpp"
#include "core/base/core_assert.hpp"

#include "events/event_sink.hpp"
#include "events/event_types.hpp"
#include "../common/high_res_timer.hpp"

namespace core::bench {

PhaseExecutor::PhaseExecutor(const PhaseDescriptor& desc,
                             PhaseContext& ctx,
                             IEventSink* eventSink) noexcept
    : _desc(desc), _ctx(ctx), _eventSink(eventSink), _stats{}, _opStream(nullptr), _tracker(nullptr)
{
    ASSERT(_ctx.rng != nullptr);
    ASSERT(_ctx.allocator != nullptr);
}

PhaseExecutor::~PhaseExecutor() noexcept {

    if (_opStream) { delete _opStream; _opStream = nullptr; }
    if (_tracker && _ownsTracker) {
        delete _tracker;
        _tracker = nullptr;
        _ownsTracker = false;
    }
    _opStream = nullptr;
    if (!_ctx.externalLifetimeTracker) _tracker = nullptr;
}

void PhaseExecutor::Execute() {
    ASSERT(_ctx.rng != nullptr);
    ASSERT(_ctx.allocator != nullptr);
    HighResTimer timer;
    u64 startTimestamp = timer.Now();

    // Create OperationStream and LifetimeTracker for this phase
    if (_opStream) { delete _opStream; _opStream = nullptr; }
    if (_tracker && _ownsTracker) {
        delete _tracker;
        _tracker = nullptr;
        _ownsTracker = false;
    }
    _opStream = new OperationStream(_desc.params, *_ctx.rng);
    ASSERT(_opStream != nullptr);

    // Compute capacity for LifetimeTracker
    u32 trackerCapacity = 0;
    if (_desc.params.maxLiveObjects > 0) {
        trackerCapacity = _desc.params.maxLiveObjects;
    } else if (_desc.params.operationCount > 0 && _desc.params.operationCount < 1000000) {
        trackerCapacity = static_cast<u32>(_desc.params.operationCount);
    } else if (_desc.params.operationCount > 0) {
        trackerCapacity = 1000000;
    } else {
        trackerCapacity = 0;
    }

    // Use external tracker if provided in context (for cross-phase live-set)
    bool ownsTracker = false;
    if (_ctx.externalLifetimeTracker) {
        _tracker = _ctx.externalLifetimeTracker;
        ownsTracker = false;
    } else if (trackerCapacity > 0) {
        _tracker = new LifetimeTracker(trackerCapacity, _desc.params.lifetimeModel, *_ctx.rng, _ctx.allocator);
        ownsTracker = true;
    } else {
        _tracker = nullptr;
        ownsTracker = false;
    }
    _ownsTracker = ownsTracker;
    _ctx.lifetimeTracker = _tracker;

    if (_tracker && (!_ctx.externalLifetimeTracker || _desc.reclaimMode == ReclaimMode::FreeAll)) {
        _tracker->Clear();
    }

    // Setup context pointers (always, even for operationCount==0)
    _ctx.eventSink = _eventSink;
    _ctx.userData = _desc.userData;
    _ctx.phaseName = _desc.name;
    _ctx.experimentName = _desc.experimentName;
    _ctx.phaseType = _desc.type;
    _ctx.repetitionId = _desc.repetitionId;
    _ctx.currentOpIndex = 0;
    _ctx.allocCount = 0;
    _ctx.freeCount = 0;
    _ctx.bytesAllocated = 0;
    _ctx.bytesFreed = 0;

    // Emit OnPhaseBegin event if event sink exists
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseBegin;
        evt.experimentName = _ctx.experimentName;
        evt.phaseName = _ctx.phaseName;
        evt.repetitionId = _ctx.repetitionId;
        evt.timestamp = startTimestamp;
        _eventSink->OnEvent(evt);
    }

    // TickManager integration
    TickManager* tickManager = nullptr;
    if (_desc.params.tickInterval > 0) {
        tickManager = new TickManager(_desc.params.tickInterval);
    }
    // Main operation loop
    u64 totalIssuedOperations = 0;
    if (_desc.params.operationCount > 0) {
        u64 opIndex = 0;
        while (_opStream->HasNext()) {
            Operation op = _opStream->Next();
            _ctx.currentOpIndex = opIndex;
            if (_desc.customOperation) {
                _desc.customOperation(_ctx);
            } else {
                if (op.type == OpType::Alloc) {
                    ExecuteOperationAlloc(op, opIndex);
                } else if (op.type == OpType::Free) {
                    ExecuteOperationFree(opIndex);
                }
            }
            if (tickManager) {
                TickContext tickCtx{};
                tickCtx.opIndex = opIndex;
                tickCtx.allocCount = _ctx.allocCount;
                tickCtx.freeCount = _ctx.freeCount;
                tickCtx.bytesAllocated = _ctx.bytesAllocated;
                tickCtx.bytesFreed = _ctx.bytesFreed;
                tickCtx.peakLiveCount = _tracker ? _tracker->GetPeakCount() : 0;
                tickCtx.peakLiveBytes = _tracker ? _tracker->GetPeakBytes() : 0;
                tickManager->OnOperation(tickCtx, _ctx);
            }
            opIndex++;
            totalIssuedOperations++;
            if (IsPhaseComplete()) {
                break;
            }
        }
    } else {
        // Even if no operations, allow completionCheck/customOperation
        if (_desc.completionCheck) {
            _desc.completionCheck(_ctx);
        }
        if (_desc.customOperation) {
            _desc.customOperation(_ctx);
        }
    }
    delete tickManager;
    u64 endTimestamp = timer.Now();
    u64 durationNs = endTimestamp - startTimestamp;

    // Save peak metrics before reclaim (they may be cleared)
    u64 peakLiveCount = _tracker ? _tracker->GetPeakCount() : 0;
    u64 peakLiveBytes = _tracker ? _tracker->GetPeakBytes() : 0;

    // Reclaim phase if needed (always, even if no operations)
    ExecuteReclaim();

    // Update stats
    _stats.allocCount = _ctx.allocCount;
    _stats.freeCount = _ctx.freeCount;
    _stats.bytesAllocated = _ctx.bytesAllocated;
    _stats.bytesFreed = _ctx.bytesFreed;
    _stats.peakLiveCount = peakLiveCount;
    _stats.peakLiveBytes = peakLiveBytes;

    // PhaseComplete event with full payload
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseComplete;
        evt.experimentName = _ctx.experimentName;
        evt.phaseName = _ctx.phaseName;
        evt.repetitionId = _ctx.repetitionId;
        evt.timestamp = endTimestamp;
        evt.data.phaseComplete.experimentName = _ctx.experimentName;
        evt.data.phaseComplete.phaseName = _ctx.phaseName;
        evt.data.phaseComplete.phaseType = _ctx.phaseType;
        evt.data.phaseComplete.repetitionId = _ctx.repetitionId;
        evt.data.phaseComplete.startTimestamp = startTimestamp;
        evt.data.phaseComplete.endTimestamp = endTimestamp;
        evt.data.phaseComplete.durationNs = durationNs;
        evt.data.phaseComplete.allocCount = _ctx.allocCount;
        evt.data.phaseComplete.freeCount = _ctx.freeCount;
        evt.data.phaseComplete.totalOperations = totalIssuedOperations; // Use issued ops, not alloc+free
        evt.data.phaseComplete.bytesAllocated = _ctx.bytesAllocated;
        evt.data.phaseComplete.bytesFreed = _ctx.bytesFreed;
        evt.data.phaseComplete.peakLiveCount = peakLiveCount;
        evt.data.phaseComplete.peakLiveBytes = peakLiveBytes;
        evt.data.phaseComplete.finalLiveCount = _tracker ? _tracker->GetLiveCount() : 0;
        evt.data.phaseComplete.finalLiveBytes = _tracker ? _tracker->GetLiveBytes() : 0;
        evt.data.phaseComplete.opsPerSec = durationNs > 0 ? static_cast<f64>(totalIssuedOperations) * 1e9 / static_cast<f64>(durationNs) : 0.0;
        evt.data.phaseComplete.throughput = durationNs > 0 ? static_cast<f64>(evt.data.phaseComplete.bytesAllocated) * 1e9 / static_cast<f64>(durationNs) : 0.0;
        _eventSink->OnEvent(evt);
    }

    // PhaseEnd marker
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseEnd;
        evt.experimentName = _ctx.experimentName;
        evt.phaseName = _ctx.phaseName;
        evt.repetitionId = _ctx.repetitionId;
        evt.timestamp = endTimestamp;
        _eventSink->OnEvent(evt);
    }
}

void PhaseExecutor::ExecuteOperationAlloc(const Operation& op, u64 opIndex) const {
    // Always perform allocation, even if tracker is not present (alloc+forget)
    core::AllocationRequest req{};
    req.size = op.size;
    req.alignment = op.alignment;
    req.tag = op.tag;
    req.flags = op.flags;
    void* ptr = _ctx.allocator->Allocate(req);
    if (!ptr) return; // Allocation failed
    if (_tracker && _tracker->isValid()) {
        auto result = _tracker->Track(ptr, op.size, op.alignment, op.tag, opIndex);
        if (!result.tracked && ptr) {
            // Free untracked allocation
            core::AllocationInfo info{};
            info.ptr = ptr;
            info.size = op.size;
            info.alignment = op.alignment;
            info.tag = op.tag;
            _ctx.allocator->Deallocate(info);
        }
        _ctx.allocCount++;
        _ctx.bytesAllocated += op.size;
        if (result.forcedFree) {
            _ctx.freeCount++;
            _ctx.bytesFreed += result.freedInfo.size; // Use actual freed size
        }
    } else {
        // Not tracked, just alloc+forget
        _ctx.allocCount++;
        _ctx.bytesAllocated += op.size;
    }
}

void PhaseExecutor::ExecuteOperationFree(u64 /*opIndex*/) const {
    if (!_tracker || !_tracker->isValid()) return;
    AllocInfo info{};
    if (!_tracker->PopForFree(info)) return;
    core::AllocationInfo deallocInfo{};
    deallocInfo.ptr = info.ptr;
    deallocInfo.size = info.size;
    deallocInfo.alignment = info.alignment;
    deallocInfo.tag = info.tag;
    _ctx.allocator->Deallocate(deallocInfo);
    _ctx.freeCount++;
    _ctx.bytesFreed += info.size;
}

// Reclaim phase: handle according to ReclaimMode
void PhaseExecutor::ExecuteReclaim() {
    switch (_desc.reclaimMode) {
        case ReclaimMode::None:
            // Do nothing, preserve live-set between phases
            break;
        case ReclaimMode::FreeAll:
            if (_tracker && _tracker->isValid()) {
                _tracker->FreeAll();
            }
            break;
        case ReclaimMode::Custom:
            if (_desc.reclaimCallback) {
                _desc.reclaimCallback(_ctx);
            }
            break;
        default:
            break;
    }
}

bool PhaseExecutor::IsPhaseComplete() const {
    if (_desc.completionCheck) {
        return _desc.completionCheck(_ctx);
    }
    return false;
}

} // namespace core::bench

