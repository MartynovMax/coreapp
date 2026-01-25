// =============================================================================
// phase_executor.cpp
// Implementation of PhaseExecutor for workload phase execution
// =============================================================================

#include "phase_executor.hpp"

#include "tick_manager.hpp"
#include "core/memory/memory_ops.hpp"
#include "core/base/core_assert.hpp"

#include "events/event_sink.hpp"
#include "events/event_types.hpp"
#include "../common/high_res_timer.hpp"

namespace core {
namespace bench {

PhaseExecutor::PhaseExecutor(const PhaseDescriptor& desc,
                             PhaseContext& ctx,
                             IEventSink* eventSink) noexcept
    : _desc(desc), _ctx(ctx), _eventSink(eventSink), _stats{}, _opStream(nullptr), _tracker(nullptr)
{
    ASSERT(_ctx.rng != nullptr);
    ASSERT(_ctx.allocator != nullptr);
}

PhaseExecutor::~PhaseExecutor() noexcept {
    if (_opStream) {
        delete _opStream;
        _opStream = nullptr;
    }
    if (_ownsTracker && _tracker) {
        delete _tracker;
        _tracker = nullptr;
        _ownsTracker = false;
    } else {
        _tracker = nullptr;
    }
}

void PhaseExecutor::Execute() {
    ASSERT(_ctx.rng != nullptr);
    ASSERT(_ctx.allocator != nullptr);

    HighResTimer timer;
    u64 startTimestamp = timer.Now();

    // Create OperationStream and LifetimeTracker for this phase
    if (_opStream) { delete _opStream; _opStream = nullptr; }
    if (_tracker) { delete _tracker; _tracker = nullptr; }
    _opStream = new OperationStream(_desc.params, *_ctx.rng);
    ASSERT(_opStream != nullptr);

    // Use external tracker if provided in context (for cross-phase live-set)
    bool ownsTracker = false;
    if (_ctx.externalLifetimeTracker) {
        _tracker = _ctx.externalLifetimeTracker;
        ownsTracker = false;
    } else {
        _tracker = new LifetimeTracker(_desc.params.lifetimeModel, _desc.params.maxLiveObjects, *_ctx.rng, _ctx.allocator);
        ownsTracker = true;
    }
    ASSERT(_tracker != nullptr);
    _ctx.lifetimeTracker = _tracker;
    // Save ownership flag for destructor
    _ownsTracker = ownsTracker;

    // Setup context pointers
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
        evt.data.phaseComplete.startTimestamp = startTimestamp;
        _eventSink->OnEvent(evt);
    }

    // TickManager integration
    TickManager* tickManager = nullptr;
    if (_desc.params.tickInterval > 0) {
        tickManager = new TickManager(_desc.params.tickInterval);
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
        if (tickManager) {
            TickContext tickCtx{};
            tickCtx.opIndex = opIndex;
            tickCtx.allocCount = _ctx.allocCount;
            tickCtx.freeCount = _ctx.freeCount;
            tickCtx.bytesAllocated = _ctx.bytesAllocated;
            tickCtx.bytesFreed = _ctx.bytesFreed;
            tickCtx.peakLiveCount = _tracker->GetPeakCount();
            tickCtx.peakLiveBytes = _tracker->GetPeakBytes();
            tickManager->OnOperation(tickCtx, _ctx);
        }
        opIndex++;
        if (IsPhaseComplete()) {
            break;
        }
    }
    if (tickManager) { delete tickManager; }
    u64 endTimestamp = timer.Now();
    u64 durationNs = endTimestamp - startTimestamp;

    // Reclaim phase if needed
    ExecuteReclaim();

    // Update stats
    _stats.allocCount = _ctx.allocCount;
    _stats.freeCount = _ctx.freeCount;
    _stats.bytesAllocated = _ctx.bytesAllocated;
    _stats.bytesFreed = _ctx.bytesFreed;
    _stats.peakLiveCount = _tracker->GetPeakCount();
    _stats.peakLiveBytes = _tracker->GetPeakBytes();

    // PhaseComplete event with full payload
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseComplete;
        evt.experimentName = _ctx.experimentName;
        evt.phaseName = _ctx.phaseName;
        evt.repetitionId = _ctx.repetitionId;
        evt.data.phaseComplete.experimentName = _ctx.experimentName;
        evt.data.phaseComplete.phaseName = _ctx.phaseName;
        evt.data.phaseComplete.phaseType = _ctx.phaseType;
        evt.data.phaseComplete.repetitionId = _ctx.repetitionId;
        evt.data.phaseComplete.startTimestamp = startTimestamp;
        evt.data.phaseComplete.endTimestamp = endTimestamp;
        evt.data.phaseComplete.durationNs = durationNs;
        evt.data.phaseComplete.allocCount = _ctx.allocCount;
        evt.data.phaseComplete.freeCount = _ctx.freeCount;
        evt.data.phaseComplete.totalOperations = _ctx.allocCount + _ctx.freeCount;
        evt.data.phaseComplete.bytesAllocated = _ctx.bytesAllocated;
        evt.data.phaseComplete.bytesFreed = _ctx.bytesFreed;
        evt.data.phaseComplete.peakLiveCount = _tracker->GetPeakCount();
        evt.data.phaseComplete.peakLiveBytes = _tracker->GetPeakBytes();
        evt.data.phaseComplete.finalLiveCount = _tracker->GetLiveCount();
        evt.data.phaseComplete.finalLiveBytes = _tracker->GetLiveBytes();
        evt.data.phaseComplete.opsPerSec = (durationNs > 0) ?
            static_cast<double>(evt.data.phaseComplete.totalOperations) * 1e9 / static_cast<double>(durationNs) : 0.0;
        evt.data.phaseComplete.throughput = (durationNs > 0) ?
            static_cast<double>(evt.data.phaseComplete.bytesAllocated) * 1e9 / static_cast<double>(durationNs) : 0.0;
        _eventSink->OnEvent(evt);
    }

    // PhaseEnd marker
    if (_eventSink) {
        Event evt{};
        evt.type = EventType::PhaseEnd;
        evt.experimentName = _ctx.experimentName;
        evt.phaseName = _ctx.phaseName;
        evt.repetitionId = _ctx.repetitionId;
        evt.data.phaseComplete.endTimestamp = endTimestamp;
        _eventSink->OnEvent(evt);
    }
}

void PhaseExecutor::ExecuteOperationAlloc(const Operation& op, u64 opIndex) const {
    ASSERT(_ctx.allocator != nullptr);
    ASSERT(_tracker != nullptr);

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
    ASSERT(_ctx.allocator != nullptr);
    ASSERT(_tracker != nullptr);

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
        // Before FreeAll, count all live objects and bytes for stats
        AllocInfo* liveArray = nullptr;
        u32 liveCount = 0;
        _tracker->GetAllLive(&liveArray, &liveCount);
        u64 freedBytes = 0;
        for (u32 i = 0; i < liveCount; ++i) {
            freedBytes += liveArray[i].size;
        }
        _ctx.freeCount += liveCount;
        _ctx.bytesFreed += freedBytes;
        _tracker->FreeAll();
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
