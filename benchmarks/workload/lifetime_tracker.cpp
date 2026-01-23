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
LifetimeTracker::~LifetimeTracker() noexcept {}

void LifetimeTracker::Track(void* /*ptr*/, u32 /*size*/, u64 /*opIndex*/) noexcept {}

void* LifetimeTracker::SelectForFree() noexcept { return nullptr; }

void LifetimeTracker::GetAllLive(AllocInfo** /*outArray*/, u32* /*outCount*/) noexcept {}

void LifetimeTracker::Clear() noexcept {}

u32 LifetimeTracker::GetLiveCount() const noexcept { return 0; }
u64 LifetimeTracker::GetLiveBytes() const noexcept { return 0; }
u64 LifetimeTracker::GetPeakBytes() const noexcept { return 0; }

} // namespace bench
} // namespace core
