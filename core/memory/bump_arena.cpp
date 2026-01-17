#include "bump_arena.hpp"

namespace core {

BumpArena::BumpArena(void* buffer, memory_size size) noexcept
    : _allocator(buffer, size)
{}

BumpArena::BumpArena(memory_size capacity, IAllocator& upstream) noexcept
    : _allocator(capacity, upstream)
{}

void* BumpArena::Allocate(memory_size size, memory_alignment alignment) noexcept {
    AllocationRequest request;
    request.size = size;
    request.alignment = alignment;
    request.flags = AllocationFlags::None;
    return _allocator.Allocate(request);
}

void BumpArena::Reset() noexcept {
    _allocator.Reset();
}

ArenaMarker BumpArena::GetMarker() const noexcept {
    ArenaMarker marker;
    marker.internal_state = reinterpret_cast<void*>(_allocator.Used());
    return marker;
}

void BumpArena::RewindTo(ArenaMarker marker) noexcept {
    memory_size targetUsed = reinterpret_cast<memory_size>(marker.internal_state);
    memory_size currentUsed = _allocator.Used();
    
    if (targetUsed < currentUsed) {
        _allocator.Reset();
    }
}

memory_size BumpArena::Capacity() const noexcept {
    return _allocator.Capacity();
}

memory_size BumpArena::Used() const noexcept {
    return _allocator.Used();
}

memory_size BumpArena::Remaining() const noexcept {
    return _allocator.Remaining();
}

bool BumpArena::Owns(const void* ptr) const noexcept {
    return _allocator.Owns(ptr);
}

} // namespace core

