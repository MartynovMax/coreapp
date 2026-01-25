// =============================================================================
// lifetime_tracker.cpp
// Implementation for LifetimeTracker
// =============================================================================

#include "lifetime_tracker.hpp"
#include "core/base/core_assert.hpp"

namespace core {
namespace bench {

LifetimeTracker::LifetimeTracker(u32 capacity, LifetimeModel model, SeededRNG& rng, IAllocator* allocator) noexcept
    : _model(model)
    , _rng(rng)
    , _allocator(allocator)
    , _buffer(nullptr)
    , _capacity(capacity)
    , _count(0)
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

void LifetimeTracker::Track(void* ptr, u32 size, core::memory_alignment alignment, core::memory_tag tag, u64 opIndex) noexcept {
    ASSERT(_buffer != nullptr);
    if (!_buffer || _count >= _capacity || !ptr || size == 0) {
        return;
    }

    AllocInfo& info = _buffer[_count++];
    info.ptr = ptr;
    info.size = size;
    info.alignment = alignment;
    info.tag = tag;
    info.allocTime = opIndex;

    _totalLiveBytes += size;
    if (_totalLiveBytes > _peakLiveBytes) _peakLiveBytes = _totalLiveBytes;
    if (_count > _peakLiveCount) _peakLiveCount = _count;
}

void LifetimeTracker::RemoveIndex(u32 idx) noexcept {
    if (idx >= _count) return;
    _totalLiveBytes -= _buffer[idx].size;
    if ((_model == LifetimeModel::Fifo || _model == LifetimeModel::Bounded) && idx == 0 && _count > 1) {
        // FIFO: shift all elements left
        for (u32 i = 1; i < _count; ++i) {
            _buffer[i - 1] = _buffer[i];
        }
    } else if (idx != _count - 1) {
        // Swap-remove for other models
        _buffer[idx] = _buffer[_count - 1];
    }
    --_count;
}

bool LifetimeTracker::PopForFree(AllocInfo& out_info) noexcept {
    ASSERT(_buffer != nullptr);
    if (!_buffer || _count == 0) {
        return false;
    }

    u32 idx = 0;

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
            // When bounded and we hit/exceed the bound, free something (FIFO semantics by default).
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

    out_info = _buffer[idx];
    RemoveIndex(idx);
    return true;
}

void LifetimeTracker::FreeAll() noexcept {
    ASSERT(_buffer != nullptr);
    if (!_buffer) {
        return;
    }

    if (_allocator) {
        for (u32 i = 0; i < _count; ++i) {
            const AllocInfo& a = _buffer[i];

            core::AllocationInfo info{};
            info.ptr = a.ptr;
            info.size = a.size;
            info.alignment = a.alignment; // 0 is allowed; allocator will normalize if it chooses
            info.tag = a.tag;

            _allocator->Deallocate(info);
        }
    }

    _count = 0;
    _totalLiveBytes = 0;
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
u32 LifetimeTracker::GetPeakCount() const noexcept { return _peakLiveCount; }

} // namespace bench
} // namespace core
