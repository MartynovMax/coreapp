// =============================================================================
// lifetime_tracker.cpp
// Implementation for LifetimeTracker
// =============================================================================

#include "lifetime_tracker.hpp"
#include "core/base/core_assert.hpp"

namespace core::bench {

LifetimeTracker::LifetimeTracker(u32 capacity, LifetimeModel model, SeededRNG& rng, IAllocator* allocator) noexcept
    : _model(model)
    , _rng(rng)
    , _allocator(allocator)
    , _buffer(nullptr)
    , _capacity(capacity)
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
        if (void* mem = _allocator->Allocate(req)) {
            _buffer = static_cast<AllocInfo*>(mem);
        }
    }
}

LifetimeTracker::~LifetimeTracker() noexcept {
    // NOTE: We intentionally do NOT FreeAll() here, because LifetimeTracker is a
    // component of an experiment phase. The phase executor decides reclaim policy.
    // We only release the internal tracking buffer itself.
    if (_allocator && _buffer) {
        core::AllocationInfo info{};
        info.ptr = _buffer;
        info.size = sizeof(AllocInfo) * _capacity;
        info.alignment = static_cast<core::memory_alignment>(alignof(AllocInfo));
        info.tag = 0;
        _allocator->Deallocate(info);
        _buffer = nullptr;
        _capacity = 0;
        _count = 0;
    }
}

LifetimeTracker::TrackResult LifetimeTracker::Track(void* ptr, u32 size, core::memory_alignment alignment, core::memory_tag tag, u64 opIndex) noexcept {
    TrackResult result{};
    if (!isValid() || !ptr || size == 0) {
        return result;
    }
    if (_count >= _capacity) {
        if (_ringMode && _model == LifetimeModel::Bounded) {
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
            return result;
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
        _tail = (_tail + 1) % _capacity;
        if (_count < _capacity) _count++;
    } else {
        _count++;
    }
    _totalLiveBytes += size;
    if (_totalLiveBytes > _peakLiveBytes) _peakLiveBytes = _totalLiveBytes;
    if (_count > _peakLiveCount) _peakLiveCount = _count;
    result.tracked = true;
    return result;
}

void LifetimeTracker::RemoveIndex(u32 idx) noexcept {
    if (!isValid() || _count == 0) return;
    _totalLiveBytes -= _buffer[idx].size;
    if (_ringMode) {
        // Only support remove head (FIFO/Bounded)
        if (idx == _head) {
            _head = (_head + 1) % _capacity;
            _count--;
        }
        // else: ignore (only head can be removed in ring mode)
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
            case LifetimeModel::Lifo:
                idx = (_tail + _capacity - 1) % _capacity;
                break;
            case LifetimeModel::Random:
                idx = (_head + (_rng.NextU32() % _count)) % _capacity;
                break;
            case LifetimeModel::LongLived:
            default:
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
                idx = _rng.NextU32() % _count;
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
            u32 idx = _ringMode ? (_head + i) % _capacity : i;
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

void LifetimeTracker::GetAllLive(AllocInfo** outArray, u32* outCount) const noexcept {
    if (outArray) *outArray = _buffer;
    if (outCount) *outCount = _count;
}

void LifetimeTracker::Clear() noexcept {
    _count = 0;
    _head = 0;
    _tail = 0;
    _totalLiveBytes = 0;
    _peakLiveBytes = 0;
    _peakLiveCount = 0;
}

u32 LifetimeTracker::GetLiveCount() const noexcept { return _count; }
u64 LifetimeTracker::GetLiveBytes() const noexcept { return _totalLiveBytes; }
u64 LifetimeTracker::GetPeakBytes() const noexcept { return _peakLiveBytes; }
u32 LifetimeTracker::GetPeakCount() const noexcept { return _peakLiveCount; }

} // namespace core::bench
