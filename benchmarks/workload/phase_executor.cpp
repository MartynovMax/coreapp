// =============================================================================
// phase_executor.cpp
// Implementation of PhaseExecutor for workload phase execution
// =============================================================================

#include "phase_executor.hpp"

#include "tick_manager.hpp"
#include "core/base/core_assert.hpp"

#include "events/event_sink.hpp"
#include "events/event_types.hpp"
#include "events/event_bus.hpp"
#include "events/allocation_event_adapter.hpp"
#include "../common/high_res_timer.hpp"
#include "lifetime_tracker.hpp"
#include <new>

namespace core::bench {

namespace {
constexpr u32 kMaxTrackerCapacity = 1000000;

inline u64 SaturatingAdd(u64 a, u64 b) noexcept {
    u64 result = a + b;
    if (result < a) return UINT64_MAX;
    return result;
}

inline f32 NormalizeAllocFreeRatio(f32 ratio) noexcept {
    if (!(ratio >= 0.0f)) return 0.0f;
    if (ratio > 1.0f) return 1.0f;
    return ratio;
}
}

PhaseExecutor::PhaseExecutor(const PhaseDescriptor& desc,
                             PhaseContext& ctx,
                             IEventSink* eventSink) noexcept
    : _desc(desc), _ctx(ctx), _eventSink(eventSink), _stats{}, _tracker(nullptr), _ownsTracker(false), _needsTracker(false)
{
    ASSERT(_ctx.callbackRng != nullptr);
    ASSERT(_ctx.allocator != nullptr);
}

PhaseExecutor::~PhaseExecutor() noexcept {
    if (_ownsTracker && _tracker) {
        _tracker->~LifetimeTracker();
        
        // Deallocate via IAllocator
        core::AllocationInfo info{};
        info.ptr = _tracker;
        info.size = sizeof(LifetimeTracker);
        info.alignment = static_cast<core::memory_alignment>(alignof(LifetimeTracker));
        info.tag = 0;
        _ctx.allocator->Deallocate(info);
        
        _tracker = nullptr;
    }
    _ownsTracker = false;
}

void PhaseExecutor::Execute() {
    ASSERT(_ctx.callbackRng != nullptr);
    ASSERT(_ctx.allocator != nullptr);
    HighResTimer timer;
    u64 startTimestamp = timer.Now();

    _stats = PhaseStats{};

    SetupLifetimeTracker();

    EventBusSink* busSink = nullptr;
    EventBusSink busSinkStorage(_eventSink);
    if (_eventSink) {
        busSink = &busSinkStorage;
    }

    _ctx.eventSink = busSink;
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
    _ctx.failedAllocCount = 0;

    if (busSink) {
        Event evt{};
        evt.type = EventType::PhaseBegin;
        evt.experimentName = _ctx.experimentName;
        evt.phaseName = _ctx.phaseName;
        evt.repetitionId = _ctx.repetitionId;
        evt.timestamp = startTimestamp;
        busSink->OnEvent(evt);
    }

    AllocationEventAdapter* allocAdapter = nullptr;
    AllocationEventAdapter allocAdapterStorage(busSink, _ctx.experimentName, _ctx.phaseName, _ctx.repetitionId);
    if (busSink) {
        allocAdapter = &allocAdapterStorage;
        allocAdapter->Attach();
    }

    // Use stack allocation for TickManager (it's very small - just a u64)
    TickManager* tickManager = nullptr;
    TickManager tickMgrStorage(_desc.params.tickInterval); // Stack allocation
    if (_desc.params.tickInterval > 0) {
        tickManager = &tickMgrStorage;
    }

    u64 totalIssuedOperations = 0;
    if (_desc.params.operationCount > 0) {
        OperationStream opStream(_desc.params);

        u64 opIndex = 0;
        while (opStream.HasNext()) {
            const u64 liveCount = _tracker ? _tracker->GetLiveCount() : 0;

            Operation op = opStream.Next(liveCount);
            _ctx.currentOpIndex = opIndex;

            if (_desc.customOperation) {
                u64 allocCountBefore = _ctx.allocCount;
                u64 freeCountBefore = _ctx.freeCount;
                u64 bytesAllocatedBefore = _ctx.bytesAllocated;
                u64 bytesFreedBefore = _ctx.bytesFreed;
                
                _desc.customOperation(_ctx, op);
                
                // Strict validation: verify metrics were updated
                if (_desc.strictMetricsValidation) {
                    bool metricsChanged = (_ctx.allocCount != allocCountBefore) ||
                                         (_ctx.freeCount != freeCountBefore) ||
                                         (_ctx.bytesAllocated != bytesAllocatedBefore) ||
                                         (_ctx.bytesFreed != bytesFreedBefore);
                    
                    if (!metricsChanged && opIndex < _desc.params.operationCount - 1) {
                        ASSERT(metricsChanged && 
                               "strictMetricsValidation: customOperation should update metrics");
                    }
                }
            } else {
                switch (op.reason) {
                    case OpReason::ForcedAllocEmptyLive:
                        _stats.forcedAllocCount++;
                        ExecuteOperationAlloc(op, opIndex);
                        break;
                    case OpReason::NoopFreeEmptyLive:
                        _stats.noopFreeCount++;
                        break;
                    case OpReason::Normal:
                        if (op.type == OpType::Alloc) {
                            ExecuteOperationAlloc(op, opIndex);
                        } else { // OpType::Free
                            ExecuteOperationFree(opIndex);
                        }
                        break;
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
        }
        _stats.issuedOpCount = totalIssuedOperations;
    } else {
        // operationCount == 0: loop-until-complete mode (do-while)
        if (!_desc.customOperation) {
            if (busSink) {
                HighResTimer failTimer;
                Event evt{};
                evt.type = EventType::PhaseFailure;
                evt.experimentName = _ctx.experimentName;
                evt.phaseName = _ctx.phaseName;
                evt.repetitionId = _ctx.repetitionId;
                evt.timestamp = failTimer.Now();
                evt.data.failure.reason = FailureReason::InvalidState;
                evt.data.failure.message = "operationCount == 0 requires customOperation callback";
                evt.data.failure.opIndex = ~0ULL;
                evt.data.failure.isRecoverable = false;
                busSink->OnEvent(evt);
            }
            FATAL("operationCount == 0 requires customOperation callback");
        }
        if (!_desc.completionCheck) {
            if (busSink) {
                HighResTimer failTimer;
                Event evt{};
                evt.type = EventType::PhaseFailure;
                evt.experimentName = _ctx.experimentName;
                evt.phaseName = _ctx.phaseName;
                evt.repetitionId = _ctx.repetitionId;
                evt.timestamp = failTimer.Now();
                evt.data.failure.reason = FailureReason::InvalidState;
                evt.data.failure.message = "operationCount == 0 requires completionCheck callback";
                evt.data.failure.opIndex = ~0ULL;
                evt.data.failure.isRecoverable = false;
                busSink->OnEvent(evt);
            }
            FATAL("operationCount == 0 requires completionCheck callback");
        }

        const u64 maxIters = _desc.maxIterations;
        u64 opIndex = 0;
        bool completed = false;

        do {
            _ctx.currentOpIndex = opIndex;

            Operation defaultOp{};
            
            u64 allocCountBefore = _ctx.allocCount;
            u64 freeCountBefore = _ctx.freeCount;
            u64 bytesAllocatedBefore = _ctx.bytesAllocated;
            u64 bytesFreedBefore = _ctx.bytesFreed;
            
            _desc.customOperation(_ctx, defaultOp);
            completed = _desc.completionCheck(_ctx);
            
            // Strict validation: verify metrics were updated
            if (_desc.strictMetricsValidation && !completed) {
                bool metricsChanged = (_ctx.allocCount != allocCountBefore) ||
                                     (_ctx.freeCount != freeCountBefore) ||
                                     (_ctx.bytesAllocated != bytesAllocatedBefore) ||
                                     (_ctx.bytesFreed != bytesFreedBefore);
                
                ASSERT(metricsChanged && 
                       "strictMetricsValidation: customOperation in loop mode should update metrics");
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
        } while (!completed && opIndex < maxIters);

        if (opIndex >= maxIters && !completed) {
            if (busSink) {
                HighResTimer failTimer;
                Event evt{};
                evt.type = EventType::PhaseFailure;
                evt.experimentName = _ctx.experimentName;
                evt.phaseName = _ctx.phaseName;
                evt.repetitionId = _ctx.repetitionId;
                evt.timestamp = failTimer.Now();
                evt.data.failure.reason = FailureReason::MaxIterationsReached;
                evt.data.failure.message = "operationCount == 0 phase exceeded maxIterations (infinite loop protection)";
                evt.data.failure.opIndex = opIndex;
                evt.data.failure.isRecoverable = false;
                busSink->OnEvent(evt);
            }
            FATAL("operationCount == 0 phase exceeded maxIterations (infinite loop protection)");
        }
        _stats.issuedOpCount = totalIssuedOperations;
    }

    if (allocAdapter) {
        allocAdapter->Detach();
    }

    LifetimeTracker* effectiveTracker = GetEffectiveTracker();
    u64 peakLiveCount = effectiveTracker ? effectiveTracker->GetPeakCount() : 0;
    u64 peakLiveBytes = effectiveTracker ? effectiveTracker->GetPeakBytes() : 0;

    u64 preReclaimLiveCount = effectiveTracker ? effectiveTracker->GetLiveCount() : 0;
    u64 preReclaimLiveBytes = effectiveTracker ? effectiveTracker->GetLiveBytes() : 0;

    ExecuteReclaim();

    effectiveTracker = GetEffectiveTracker();
    u64 finalLiveCount = effectiveTracker ? effectiveTracker->GetLiveCount() : 0;
    u64 finalLiveBytes = effectiveTracker ? effectiveTracker->GetLiveBytes() : 0;

    u64 endTimestamp = timer.Now();
    u64 durationNs = endTimestamp - startTimestamp;

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
    _stats.failedAllocCount = _ctx.failedAllocCount;

    _stats.totalFreeCount =
        SaturatingAdd(_stats.freeCount, SaturatingAdd(_stats.internalFreeCount, _stats.reclaimFreeCount));
    _stats.totalBytesFreed =
        SaturatingAdd(_stats.bytesFreed, SaturatingAdd(_stats.internalBytesFreed, _stats.reclaimBytesFreed));

    if (busSink) {
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
        evt.data.phaseComplete.totalOperations = _stats.issuedOpCount;
        evt.data.phaseComplete.issuedOpCount = _stats.issuedOpCount;
        evt.data.phaseComplete.forcedAllocCount = _stats.forcedAllocCount;
        evt.data.phaseComplete.noopFreeCount = _stats.noopFreeCount;
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
        evt.data.phaseComplete.failedAllocCount = _stats.failedAllocCount;
        evt.data.phaseComplete.opsPerSec = durationNs > 0 ? static_cast<f64>(_stats.issuedOpCount) * 1e9 / static_cast<f64>(durationNs) : 0.0;
        evt.data.phaseComplete.throughput = durationNs > 0 ? static_cast<f64>(_stats.bytesAllocated) * 1e9 / static_cast<f64>(durationNs) : 0.0;
        busSink->OnEvent(evt);
    }

    if (busSink) {
        Event evt{};
        evt.type = EventType::PhaseEnd;
        evt.experimentName = _ctx.experimentName;
        evt.phaseName = _ctx.phaseName;
        evt.repetitionId = _ctx.repetitionId;
        evt.timestamp = endTimestamp;
        busSink->OnEvent(evt);
    }
}

LifetimeTracker* PhaseExecutor::GetEffectiveTracker() const noexcept {
    if (_tracker) return _tracker;
    if (_ctx.liveSetTracker) return _ctx.liveSetTracker;
    if (_ctx.externalLifetimeTracker) return _ctx.externalLifetimeTracker;
    return nullptr;
}

void PhaseExecutor::ExecuteOperationAlloc(const Operation& op, u64 opIndex) const {
    core::AllocationRequest req{};
    req.size = op.size;
    req.alignment = op.alignment;
    req.tag = op.tag;
    req.flags = op.flags;

    void* ptr = _ctx.allocator->Allocate(req);
    if (!ptr) {
        _ctx.failedAllocCount++;
        return;
    }

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
    _ctx.bytesAllocated = SaturatingAdd(_ctx.bytesAllocated, op.size);

    if (_tracker && _tracker->isValid()) {
        auto result = _tracker->Track(ptr, op.size, op.alignment, op.tag, opIndex);
        if (!result.tracked) {
            if (result.error == TrackError::TrackerInvalid) {
                FATAL("Invariant violated: tracker became invalid during execution");
            }
            core::AllocationInfo info{};
            info.ptr = ptr;
            info.size = op.size;
            info.alignment = op.alignment;
            info.tag = op.tag;
            _ctx.allocator->Deallocate(info);
            _ctx.internalFreeCount++;
            _ctx.internalBytesFreed = SaturatingAdd(_ctx.internalBytesFreed, op.size);
        }
        if (result.forcedFree) {
            _ctx.internalFreeCount++;
            _ctx.internalBytesFreed = SaturatingAdd(_ctx.internalBytesFreed, result.freedInfo.size);
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
    _ctx.bytesFreed = SaturatingAdd(_ctx.bytesFreed, info.size);
}

void PhaseExecutor::ExecuteReclaim() {
    switch (_desc.reclaimMode) {
        case ReclaimMode::None: {
            ASSERT(!_ownsTracker);
            LifetimeTracker* effectiveExternal = _ctx.liveSetTracker ? _ctx.liveSetTracker : _ctx.externalLifetimeTracker;
            ASSERT(_tracker == effectiveExternal);
            if (_tracker) {
                ASSERT(_tracker->isValid());
            }
            break;
        }
        case ReclaimMode::FreeAll: {
            ASSERT(_tracker && _tracker->isValid());
            u64 freedCount = 0;
            u64 freedBytes = 0;
            _tracker->FreeAll(&freedCount, &freedBytes);
            _ctx.reclaimFreeCount += freedCount;
            _ctx.reclaimBytesFreed = SaturatingAdd(_ctx.reclaimBytesFreed, freedBytes);
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
    if (_ctx.liveSetTracker && _ctx.externalLifetimeTracker) {
        FATAL("PhaseContext violation: both liveSetTracker and externalLifetimeTracker are set.");
    }

    if (_ownsTracker && _tracker) {
        _tracker->~LifetimeTracker();
        core::AllocationInfo info{};
        info.ptr = _tracker;
        info.size = sizeof(LifetimeTracker);
        info.alignment = static_cast<core::memory_alignment>(alignof(LifetimeTracker));
        info.tag = 0;
        _ctx.allocator->Deallocate(info);
    }
    _tracker = nullptr;
    _ownsTracker = false;

    f32 normalizedRatio = NormalizeAllocFreeRatio(_desc.params.allocFreeRatio);

    if (_desc.params.lifetimeModel == LifetimeModel::LongLived && 
        normalizedRatio < 1.0f) {
        FATAL("LongLived lifetime model requires allocFreeRatio == 1.0");
    }

    const bool canAllocate = (_desc.params.operationCount > 0) && (normalizedRatio > 0.0f);
    const bool usesStandardPath = !_desc.customOperation;

    _needsTracker = canAllocate && usesStandardPath;

    const bool requiresTracker = _needsTracker || 
                                 (_desc.reclaimMode == ReclaimMode::FreeAll);

    if (_desc.reclaimMode == ReclaimMode::None) {
        const bool hasExternalTracker = (_ctx.liveSetTracker != nullptr) || (_ctx.externalLifetimeTracker != nullptr);
        if (requiresTracker && !hasExternalTracker) {
            FATAL("ReclaimMode::None with allocating phase requires external tracker");
        }
    } else {
        if (_ctx.liveSetTracker || _ctx.externalLifetimeTracker) {
            FATAL("liveSetTracker/externalLifetimeTracker are only allowed with ReclaimMode::None");
        }
    }

    if (_desc.reclaimMode == ReclaimMode::None) {
        LifetimeTracker* externalTracker = _ctx.liveSetTracker ? _ctx.liveSetTracker : _ctx.externalLifetimeTracker;
        
        if (externalTracker) {
            if (!externalTracker->isValid()) {
                FATAL("External tracker is invalid (allocation failed?)");
            }
            _tracker = externalTracker;
            _ownsTracker = false;
        } else {
            _tracker = nullptr;
            _ownsTracker = false;
        }
    } else {
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

            core::AllocationRequest req{};
            req.size = sizeof(LifetimeTracker);
            req.alignment = static_cast<core::memory_alignment>(alignof(LifetimeTracker));
            req.tag = 0;
            req.flags = core::AllocationFlags::None;

            void* mem = _ctx.allocator->Allocate(req);
            if (!mem) {
                FATAL("Failed to allocate memory for LifetimeTracker");
            }

            _tracker = new (mem) LifetimeTracker(trackerCapacity, _desc.params.lifetimeModel, *_ctx.callbackRng, _ctx.allocator);
            if (!_tracker || !_tracker->isValid()) {
                core::AllocationInfo info{};
                info.ptr = mem;
                info.size = sizeof(LifetimeTracker);
                info.alignment = static_cast<core::memory_alignment>(alignof(LifetimeTracker));
                info.tag = 0;
                _ctx.allocator->Deallocate(info);
                FATAL("phase requires LifetimeTracker, but allocation/initialization failed");
            }

            _tracker->Clear(); // ensure empty live-set for this phase
            _ownsTracker = true;
        } else {
            _tracker = nullptr;
            _ownsTracker = false;
        }
    }
}

} // namespace core::bench

