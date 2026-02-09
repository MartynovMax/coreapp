// =============================================================================
// lifetime_tracker.cpp
// Implementation for LifetimeTracker
// =============================================================================

#include "lifetime_tracker.hpp"
#include "core/base/core_assert.hpp"

namespace core::bench {

namespace {
    inline u32 NextPowerOfTwo(u32 v) noexcept {
        if (v == 0) return 1;
        if ((v & (v - 1)) == 0) return v;
        if (v > 0x80000000u) return 0x80000000u;
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }
}

LifetimeTracker::LifetimeTracker(u32 capacity, LifetimeModel model, SeededRNG& rng, IAllocator* allocator) noexcept
    : _model(model)
    , _rng(rng)
    , _allocator(allocator)
    , _buffer(nullptr)
    , _bufferTag(0)
    , _capacity(NextPowerOfTwo(capacity))
    , _count(0)
    , _head(0)
    , _tail(0)
    , _ringMode(model == LifetimeModel::Fifo || model == LifetimeModel::Bounded)
    , _totalLiveBytes(0)
    , _peakLiveBytes(0)
    , _peakLiveCount(0)
{
    ASSERT(_allocator != nullptr);
    ASSERT(_capacity > 0);
    if (_allocator && _capacity > 0) {
        core::AllocationRequest req{};
        req.size = sizeof(AllocInfo) * _capacity;
        req.alignment = static_cast<core::memory_alignment>(alignof(AllocInfo));
        req.tag = 0;
        _bufferTag = req.tag;
        if (void* mem = _allocator->Allocate(req)) {
            _buffer = static_cast<AllocInfo*>(mem);
        }
    }
}

LifetimeTracker::~LifetimeTracker() noexcept {

    if (_count > 0 && _allocator) {
        FreeAll();
    }
    
    if (_allocator && _buffer) {
        core::AllocationInfo info{};
        info.ptr = _buffer;
        info.size = sizeof(AllocInfo) * _capacity;
        info.alignment = static_cast<core::memory_alignment>(alignof(AllocInfo));
        info.tag = _bufferTag;
        _allocator->Deallocate(info);
        _buffer = nullptr;
        _capacity = 0;
        _count = 0;
    }
}

LifetimeTracker::TrackResult LifetimeTracker::Track(void* ptr, u32 size, core::memory_alignment alignment, core::memory_tag tag, u64 opIndex) noexcept {
    TrackResult result{};
    
    if (!ptr || size == 0) {
        result.error = TrackError::InvalidInput;
        return result;
    }
    
    if (!isValid()) {
        result.error = TrackError::TrackerInvalid;
        return result;
    }
    
    if (_count >= _capacity) {
        if (_model == LifetimeModel::Bounded) {
            // Forced free oldest (head)
            const AllocInfo& toFree = _buffer[_head];
            if (_allocator && toFree.ptr) {
                core::AllocationInfo info{};
                info.ptr = toFree.ptr;
                info.size = toFree.size;
                info.alignment = toFree.alignment;
                info.tag = toFree.tag;
                _allocator->Deallocate(info);
            }
            result.forcedFree = true;
            result.freedInfo = _buffer[_head];
            RemoveIndex(_head);
        } else {
            result.error = TrackError::TrackerFull;
            FATAL("LifetimeTracker overflow: increase maxLiveObjects or adjust workload");
            return result; // Unreachable
        }
    }
    
    u32 idx = _ringMode ? _tail : _count;
    AllocInfo& info = _buffer[idx];
    info.ptr = ptr;
    info.size = size;
    info.alignment = alignment;
    info.tag = tag;
    info.allocTime = opIndex;
    if (_ringMode) {
        _tail = WrapIndex(_tail + 1);
        if (_count < _capacity) _count++;
    } else {
        _count++;
    }

    if (_totalLiveBytes > core::kU64Max - size) {
        FATAL("Peak live bytes overflow - workload too large");
    }
    _totalLiveBytes += size;
    if (_totalLiveBytes > _peakLiveBytes) _peakLiveBytes = _totalLiveBytes;
    if (_count > _peakLiveCount) _peakLiveCount = _count;
    result.tracked = true;
    result.error = TrackError::None;
    return result;
}

void LifetimeTracker::RemoveIndex(u32 idx) noexcept {
    if (!isValid() || _count == 0) return;
    
    if (_ringMode && idx != _head) {
        return;
    }
    
    _totalLiveBytes -= _buffer[idx].size;
    
    if (_ringMode) {
        _head = WrapIndex(_head + 1);
        _count--;
    } else {
        if (idx != _count - 1) {
            _buffer[idx] = _buffer[_count - 1];
        }
        --_count;
    }
}

bool LifetimeTracker::PopForFree(AllocInfo& out_info) noexcept {
    if (!isValid() || _count == 0) {
        return false;
    }
    u32 idx = 0;
    if (_ringMode) {
        switch (_model) {
            case LifetimeModel::Fifo:
            case LifetimeModel::Bounded:
                idx = _head;
                break;
            case LifetimeModel::LongLived:
                return false;
            default:
                ASSERT(false && "Unsupported lifetime model for ring mode");
                return false;
        }
    } else {
        switch (_model) {
            case LifetimeModel::Lifo:
                idx = _count - 1;
                break;
            case LifetimeModel::Fifo:
                idx = 0;
                break;
            case LifetimeModel::Random:
                idx = _rng.NextRange(0u, _count - 1);
                break;
            case LifetimeModel::Bounded:
                if (_capacity > 0 && _count >= _capacity) {
                    idx = 0;
                } else {
                    return false;
                }
                break;
            case LifetimeModel::LongLived:
            default:
                return false;
        }
    }
    out_info = _buffer[idx];
    RemoveIndex(idx);
    return true;
}

void LifetimeTracker::FreeAll() noexcept {
    if (!isValid()) {
        return;
    }
    if (_allocator) {
        u32 n = _count;
        for (u32 i = 0; i < n; ++i) {
            u32 idx = _ringMode ? WrapIndex(_head + i) : i;
            const AllocInfo& a = _buffer[idx];
            core::AllocationInfo info{};
            info.ptr = a.ptr;
            info.size = a.size;
            info.alignment = a.alignment;
            info.tag = a.tag;
            _allocator->Deallocate(info);
        }
    }
    _count = 0;
    _head = 0;
    _tail = 0;
    _totalLiveBytes = 0;
    _peakLiveBytes = 0;
    _peakLiveCount = 0;
}

void LifetimeTracker::FreeAll(core::u64* outCount, core::u64* outBytes) noexcept {
    if (!isValid()) {
        if (outCount) *outCount = 0;
        if (outBytes) *outBytes = 0;
        return;
    }
    core::u64 freedCount = 0;
    core::u64 freedBytes = 0;
    if (_allocator) {
        u32 n = _count;
        for (u32 i = 0; i < n; ++i) {
            u32 idx = _ringMode ? WrapIndex(_head + i) : i;
            const AllocInfo& a = _buffer[idx];
            core::AllocationInfo info{};
            info.ptr = a.ptr;
            info.size = a.size;
            info.alignment = a.alignment;
            info.tag = a.tag;
            _allocator->Deallocate(info);
            freedCount++;
            freedBytes += a.size;
        }
    }
    if (outCount) *outCount = freedCount;
    if (outBytes) *outBytes = freedBytes;
    _count = 0;
    _head = 0;
    _tail = 0;
    _totalLiveBytes = 0;
    _peakLiveBytes = 0;
    _peakLiveCount = 0;
}

void LifetimeTracker::GetAllLive(AllocInfo** outArray, u32* outCount, u32* outHead, bool* outRingMode) const noexcept {
    // WARNING: In ringMode (FIFO/Bounded), buffer is not contiguous and not ordered by allocation time.
    // Use ForEachLive() for correct iteration over live objects.
    if (outArray) *outArray = _buffer;
    if (outCount) *outCount = _count;
    if (outHead) *outHead = _head;
    if (outRingMode) *outRingMode = _ringMode;
}

void LifetimeTracker::ForEachLive(LiveObjectCallback callback, void* userData) const noexcept {
    if (!callback) return;
    for (u32 i = 0; i < _count; ++i) {
        u32 idx = _ringMode ? (_head + i) % _capacity : i;
        const AllocInfo& info = _buffer[idx];
        callback(info, userData);
    }
}

u32 LifetimeTracker::GetLiveCount() const noexcept { return _count; }
u64 LifetimeTracker::GetLiveBytes() const noexcept { return _totalLiveBytes; }
u64 LifetimeTracker::GetPeakBytes() const noexcept { return _peakLiveBytes; }
u32 LifetimeTracker::GetPeakCount() const noexcept { return _peakLiveCount; }

void LifetimeTracker::Clear() noexcept {
    // Reset live-set without freeing objects (for cross-phase carry-over)
    _count = 0;
    _head = 0;
    _tail = 0;
    _totalLiveBytes = 0;
    _peakLiveBytes = 0;
    _peakLiveCount = 0;
}


} // namespace core::bench
