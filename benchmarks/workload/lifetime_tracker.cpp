// =============================================================================
// lifetime_tracker.cpp
// Implementation stub for LifetimeTracker (no logic, only structure for build)
// =============================================================================

#include "lifetime_tracker.hpp"

namespace core {
namespace bench {

// Constructor
LifetimeTracker::LifetimeTracker(LifetimeModel model, u32 maxLiveObjects, SeededRNG& rng, IAllocator* allocator) noexcept
    : _model(model), _maxLiveObjects(maxLiveObjects), _rng(rng), _allocator(allocator),
      _buffer(nullptr), _capacity(0), _count(0), _totalLiveBytes(0), _peakLiveBytes(0)
{
    if (_allocator && _maxLiveObjects > 0) {
        core::AllocationRequest req;
        req.size = sizeof(AllocInfo) * _maxLiveObjects;
        req.alignment = alignof(AllocInfo);
        if (void* mem = _allocator->Allocate(req)) {
            _buffer = static_cast<AllocInfo*>(mem);
            _capacity = _maxLiveObjects;
        } else {
            _buffer = nullptr;
            _capacity = 0;
        }
    }
}

// Destructor
LifetimeTracker::~LifetimeTracker() noexcept {
    if (_allocator && _buffer) {
        core::AllocationInfo info;
        info.ptr = _buffer;
        info.size = sizeof(AllocInfo) * _capacity;
        info.alignment = alignof(AllocInfo);
        info.tag = 0;
        _allocator->Deallocate(info);
        _buffer = nullptr;
        _capacity = 0;
    }
}

void LifetimeTracker::Track(void* ptr, u32 size, u64 opIndex) noexcept {
    if (!_buffer || _count >= _capacity || !ptr || size == 0)
        return;
    AllocInfo& info = _buffer[_count++];
    info.ptr = ptr;
    info.size = size;
    info.allocTime = opIndex;
    _totalLiveBytes += size;
    if (_totalLiveBytes > _peakLiveBytes)
        _peakLiveBytes = _totalLiveBytes;
}

void* LifetimeTracker::SelectForFree() noexcept {
    if (!_buffer || _count == 0)
        return nullptr;
    if (_model == LifetimeModel::Lifo) {
        return _buffer[_count - 1].ptr;
    } else if (_model == LifetimeModel::Fifo) {
        return _buffer[0].ptr;
    } else if (_model == LifetimeModel::Random) {
        const u32 idx = _rng.NextU32() % _count;
        return _buffer[idx].ptr;
    } else if (_model == LifetimeModel::Bounded) {
        if (_count >= _maxLiveObjects && _count > 0) {
            return _buffer[0].ptr;
        }
        return nullptr;
    } else if (_model == LifetimeModel::LongLived) {
        return nullptr;
    }
    return nullptr;
}

void LifetimeTracker::Remove(const void* ptr) noexcept {
    if (!_buffer || _count == 0 || !ptr)
        return;
    for (u32 i = 0; i < _count; ++i) {
        if (_buffer[i].ptr == ptr) {
            _totalLiveBytes -= _buffer[i].size;
            if (i != _count - 1) {
                _buffer[i] = _buffer[_count - 1];
            }
            --_count;
            return;
        }
    }
}

void LifetimeTracker::GetAllLive(AllocInfo** outArray, u32* outCount) const noexcept {
    if (outArray) *outArray = _buffer;
    if (outCount) *outCount = _count;
}

void LifetimeTracker::Clear() noexcept {
    _count = 0;
    _totalLiveBytes = 0;
}

u32 LifetimeTracker::GetLiveCount() const noexcept { return _count; }
u64 LifetimeTracker::GetLiveBytes() const noexcept { return _totalLiveBytes; }
u64 LifetimeTracker::GetPeakBytes() const noexcept { return _peakLiveBytes; }

} // namespace bench
} // namespace core
