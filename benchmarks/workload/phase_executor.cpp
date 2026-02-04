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
#include "lifetime_tracker.hpp"
#include <memory>

namespace core::bench {

namespace {
constexpr u32 kMaxTrackerCapacity = 1000000;
}

PhaseExecutor::PhaseExecutor(const PhaseDescriptor& desc,
                             PhaseContext& ctx,
                             IEventSink* eventSink) noexcept
    : _desc(desc), _ctx(ctx), _eventSink(eventSink), _stats{}, _tracker(nullptr), _ownsTracker(false), _needsTracker(false)
{
    ASSERT(_ctx.rng != nullptr);
    ASSERT(_ctx.allocator != nullptr);
}

PhaseExecutor::~PhaseExecutor() noexcept {
    if (_ownsTracker && _tracker) {
        delete _tracker;
        _tracker = nullptr;
    }
    _ownsTracker = false;
}

void PhaseExecutor::Execute() {
    ASSERT(_ctx.rng != nullptr);
    ASSERT(_ctx.allocator != nullptr);
    HighResTimer timer;
    u64 startTimestamp = timer.Now();

    _stats = PhaseStats{};

    SetupLifetimeTracker();

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
    _ctx.internalFreeCount = 0;
    _ctx.internalBytesFreed = 0;
    _ctx.reclaimFreeCount = 0;
    _ctx.reclaimBytesFreed = 0;

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


    std::unique_ptr<TickManager> tickManager;
    if (_desc.params.tickInterval > 0) {
        tickManager = std::make_unique<TickManager>(_desc.params.tickInterval);
    }

    u64 totalIssuedOperations = 0;
    if (_desc.params.operationCount > 0) {
        std::unique_ptr<OperationStream> opStream = std::make_unique<OperationStream>(_desc.params, *_ctx.rng);
        ASSERT(opStream != nullptr);

        u64 opIndex = 0;
        const bool canAllocate = (_desc.params.allocFreeRatio > 0.0f);
        while (opStream->HasNext()) {
            const u64 liveCount = _tracker ? _tracker->GetLiveCount() : 0;
            if (liveCount == 0 && !canAllocate) {
                break;
            }

            Operation op = opStream->Next(liveCount);
            _ctx.currentOpIndex = opIndex;

            if (_desc.customOperation) {
                _desc.customOperation(_ctx);
            } else {
                if (op.type == OpType::Alloc) {
                    ExecuteOperationAlloc(op, opIndex);
                } else { // OpType::Free
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
        if (_desc.customOperation) {
            _desc.customOperation(_ctx);
        }
        if (_desc.completionCheck) {
            (void)_desc.completionCheck(_ctx);
        }
    }

    u64 endTimestamp = timer.Now();
    u64 durationNs = endTimestamp - startTimestamp;

    u64 peakLiveCount = _tracker ? _tracker->GetPeakCount() : 0;
    u64 peakLiveBytes = _tracker ? _tracker->GetPeakBytes() : 0;

    u64 preReclaimLiveCount = _tracker ? _tracker->GetLiveCount() : 0;
    u64 preReclaimLiveBytes = _tracker ? _tracker->GetLiveBytes() : 0;

    ExecuteReclaim();

    u64 finalLiveCount = _tracker ? _tracker->GetLiveCount() : 0;
    u64 finalLiveBytes = _tracker ? _tracker->GetLiveBytes() : 0;

    _stats.allocCount = _ctx.allocCount;
    _stats.freeCount = _ctx.freeCount;
    _stats.bytesAllocated = _ctx.bytesAllocated;
    _stats.bytesFreed = _ctx.bytesFreed;
    _stats.peakLiveCount = peakLiveCount;
    _stats.peakLiveBytes = peakLiveBytes;
    _stats.preReclaimLiveCount = preReclaimLiveCount;
    _stats.preReclaimLiveBytes = preReclaimLiveBytes;
    _stats.finalLiveCount = finalLiveCount;
    _stats.finalLiveBytes = finalLiveBytes;
    _stats.internalFreeCount = _ctx.internalFreeCount;
    _stats.internalBytesFreed = _ctx.internalBytesFreed;
    _stats.reclaimFreeCount = _ctx.reclaimFreeCount;
    _stats.reclaimBytesFreed = _ctx.reclaimBytesFreed;

    _stats.totalFreeCount =
        _stats.freeCount + _stats.internalFreeCount + _stats.reclaimFreeCount;
    _stats.totalBytesFreed =
        _stats.bytesFreed + _stats.internalBytesFreed + _stats.reclaimBytesFreed;

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
        evt.data.phaseComplete.allocCount = _stats.allocCount;
        evt.data.phaseComplete.freeCount = _stats.freeCount;
        evt.data.phaseComplete.totalOperations = totalIssuedOperations; // Use issued ops, not alloc+free
        evt.data.phaseComplete.bytesAllocated = _stats.bytesAllocated;
        evt.data.phaseComplete.bytesFreed = _stats.bytesFreed;
        evt.data.phaseComplete.peakLiveCount = _stats.peakLiveCount;
        evt.data.phaseComplete.peakLiveBytes = _stats.peakLiveBytes;
        evt.data.phaseComplete.preReclaimLiveCount = _stats.preReclaimLiveCount;
        evt.data.phaseComplete.preReclaimLiveBytes = _stats.preReclaimLiveBytes;
        evt.data.phaseComplete.finalLiveCount = _stats.finalLiveCount;
        evt.data.phaseComplete.finalLiveBytes = _stats.finalLiveBytes;
        evt.data.phaseComplete.internalFreeCount = _stats.internalFreeCount;
        evt.data.phaseComplete.internalBytesFreed = _stats.internalBytesFreed;
        evt.data.phaseComplete.reclaimFreeCount = _stats.reclaimFreeCount;
        evt.data.phaseComplete.reclaimBytesFreed = _stats.reclaimBytesFreed;
        evt.data.phaseComplete.totalFreeCount = _stats.totalFreeCount;
        evt.data.phaseComplete.totalBytesFreed = _stats.totalBytesFreed;
        evt.data.phaseComplete.opsPerSec = durationNs > 0 ? static_cast<f64>(totalIssuedOperations) * 1e9 / static_cast<f64>(durationNs) : 0.0;
        evt.data.phaseComplete.throughput = durationNs > 0 ? static_cast<f64>(_stats.bytesAllocated) * 1e9 / static_cast<f64>(durationNs) : 0.0;
        _eventSink->OnEvent(evt);
    }

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
    core::AllocationRequest req{};
    req.size = op.size;
    req.alignment = op.alignment;
    req.tag = op.tag;
    req.flags = op.flags;

    void* ptr = _ctx.allocator->Allocate(req);
    if (!ptr) return;

    if (_needsTracker && (!_tracker || !_tracker->isValid())) {
        core::AllocationInfo info{};
        info.ptr = ptr;
        info.size = op.size;
        info.alignment = op.alignment;
        info.tag = op.tag;
        _ctx.allocator->Deallocate(info);
        FATAL("Invariant violated: _needsTracker is true, but LifetimeTracker is null/invalid");
        return;
    }

    _ctx.allocCount++;
    _ctx.bytesAllocated += op.size;

    if (_tracker && _tracker->isValid()) {
        auto result = _tracker->Track(ptr, op.size, op.alignment, op.tag, opIndex);
        if (!result.tracked) {
            core::AllocationInfo info{};
            info.ptr = ptr;
            info.size = op.size;
            info.alignment = op.alignment;
            info.tag = op.tag;
            _ctx.allocator->Deallocate(info);
            _ctx.internalFreeCount++;
            _ctx.internalBytesFreed += op.size;
        }
        if (result.forcedFree) {
            _ctx.internalFreeCount++;
            _ctx.internalBytesFreed += result.freedInfo.size;
        }
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
        case ReclaimMode::None: {
            ASSERT(!_ownsTracker);
            ASSERT(_tracker == _ctx.externalLifetimeTracker);
            ASSERT(_tracker && _tracker->isValid());
            break;
        }
        case ReclaimMode::FreeAll: {
            ASSERT(_tracker && _tracker->isValid());
            u64 freedCount = 0;
            u64 freedBytes = 0;
            _tracker->FreeAll(&freedCount, &freedBytes);
            _ctx.reclaimFreeCount += freedCount;
            _ctx.reclaimBytesFreed += freedBytes;
            ASSERT(_tracker->GetLiveCount() == 0);
            ASSERT(_tracker->GetLiveBytes() == 0);
            break;
        }
        case ReclaimMode::Custom: {
            if (_desc.reclaimCallback) {
                _desc.reclaimCallback(_ctx);
            }
            break;
        }
        default: {
            FATAL("Unknown ReclaimMode encountered in ExecuteReclaim");
        }
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

void PhaseExecutor::SetupLifetimeTracker() noexcept {
    // Reset tracker state for this execution.
    if (_ownsTracker && _tracker) {
        delete _tracker;
    }
    _tracker = nullptr;
    _ownsTracker = false;
    _ctx.lifetimeTracker = nullptr;

    if (_desc.params.lifetimeModel == LifetimeModel::LongLived && 
        _desc.params.allocFreeRatio < 1.0f) {
        FATAL("LongLived lifetime model requires allocFreeRatio == 1.0");
    }

    _needsTracker =
        (_desc.params.operationCount > 0) &&
        ((_desc.params.allocFreeRatio < 1.0f) ||
         (_desc.params.lifetimeModel != LifetimeModel::LongLived));

   
    const bool requiresTracker = _needsTracker || 
                                 (_desc.reclaimMode == ReclaimMode::FreeAll);

    if (_desc.reclaimMode == ReclaimMode::None) {
        if (!_ctx.externalLifetimeTracker) {
            FATAL("ReclaimMode::None requires externalLifetimeTracker");
        }
    } else {
        if (_ctx.externalLifetimeTracker) {
            FATAL("externalLifetimeTracker is only allowed with ReclaimMode::None");
        }
    }

    if (_desc.reclaimMode == ReclaimMode::None) {
        // Contract: external tracker must already be fully configured; isValid() == ready.
        if (!_ctx.externalLifetimeTracker->isValid()) {
            FATAL("externalLifetimeTracker is invalid (allocation failed?)");
        }

        _tracker = _ctx.externalLifetimeTracker;
        _ownsTracker = false;
        _ctx.lifetimeTracker = _tracker;
    } else {
        // Non-None reclaim modes: tracker is per-phase and owned here if needed.
        if (requiresTracker) {
            u32 trackerCapacity = 0;
            if (_desc.params.maxLiveObjects > 0) {
                trackerCapacity = _desc.params.maxLiveObjects;
            } else if (_desc.params.operationCount > 0 && _desc.params.operationCount < kMaxTrackerCapacity) {
                trackerCapacity = static_cast<u32>(_desc.params.operationCount);
            } else if (_desc.params.operationCount > 0) {
                trackerCapacity = kMaxTrackerCapacity;
            }
            if (trackerCapacity == 0) {
                trackerCapacity = 1;
            }

            _tracker = new LifetimeTracker(trackerCapacity, _desc.params.lifetimeModel, *_ctx.rng, _ctx.allocator);
            if (!_tracker || !_tracker->isValid()) {
                FATAL("phase requires LifetimeTracker, but allocation/initialization failed");
            }

            _tracker->Clear(); // ensure empty live-set for this phase
            _ownsTracker = true;
            _ctx.lifetimeTracker = _tracker;
        } else {
            _tracker = nullptr;
            _ownsTracker = false;
            _ctx.lifetimeTracker = nullptr;
        }
    }
}

} // namespace core::bench

