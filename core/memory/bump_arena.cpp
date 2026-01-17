#include "bump_arena.hpp"
#include <cstdint>

namespace core {

BumpArena::BumpArena(void* buffer, memory_size size) noexcept
    : _allocator(buffer, size)
{}

BumpArena::BumpArena(memory_size capacity, IAllocator& upstream) noexcept
    : _allocator(capacity, upstream)
{}

BumpArena::BumpArena(memory_size capacity, IAllocator& upstream, const char* name) noexcept
    : _allocator(capacity, upstream)
    , _name(name)
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
    memory_size used = _allocator.Used();
    marker.internal_state = reinterpret_cast<void*>(static_cast<uintptr_t>(used));
    return marker;
}

void BumpArena::RewindTo(ArenaMarker marker) noexcept {
    memory_size targetUsed = static_cast<memory_size>(reinterpret_cast<uintptr_t>(marker.internal_state));
    _allocator.RewindToUsed(targetUsed);
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

const char* BumpArena::Name() const noexcept {
    return _name ? _name : "bump_arena";
}

} // namespace core

