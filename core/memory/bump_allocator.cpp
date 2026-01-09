#include "bump_allocator.hpp"
#include "memory_ops.hpp"

namespace core {

BumpAllocator::BumpAllocator(void* buffer, memory_size size) noexcept
    : _begin(static_cast<u8*>(buffer))
    , _current(static_cast<u8*>(buffer))
    , _end(static_cast<u8*>(buffer) + size)
    , _upstream(nullptr)
{
}

BumpAllocator::BumpAllocator(memory_size capacity, IAllocator& upstream) noexcept
    : _upstream(&upstream)
{
    _begin = static_cast<u8*>(AllocateBytes(upstream, capacity));
    _current = _begin;
    _end = _begin + capacity;
}

BumpAllocator::~BumpAllocator() noexcept {
    if (_upstream && _begin) {
        DeallocateBytes(*_upstream, _begin, Capacity());
    }
}

void* BumpAllocator::Allocate(const AllocationRequest& request) noexcept {
    if (request.size == 0) {
        return nullptr;
    }

    const memory_alignment alignment = detail::NormalizeAlignment(request.alignment);

#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(detail::IsValidAlignment(alignment));
#endif

    u8* aligned = AlignPtrUp(_current, alignment);
    u8* newCurrent = aligned + request.size;
    
    if (newCurrent > _end) {
        return nullptr;
    }

    _current = newCurrent;

    if (Any(request.flags & AllocationFlags::ZeroInitialize)) {
        memory_zero(aligned, request.size);
    }

    return aligned;
}

void BumpAllocator::Deallocate(const AllocationInfo& info) noexcept {
    CORE_UNUSED(info);
}

void BumpAllocator::Reset() noexcept {
    _current = _begin;
}

memory_size BumpAllocator::Used() const noexcept {
    return static_cast<memory_size>(_current - _begin);
}

memory_size BumpAllocator::Capacity() const noexcept {
    return static_cast<memory_size>(_end - _begin);
}

memory_size BumpAllocator::Remaining() const noexcept {
    return static_cast<memory_size>(_end - _current);
}

} // namespace core

