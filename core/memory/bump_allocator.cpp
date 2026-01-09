#include "bump_allocator.hpp"
#include "memory_ops.hpp"

namespace core {

BumpAllocator::BumpAllocator(void* buffer, memory_size size) noexcept
    : _upstream(nullptr)
{
    if (buffer == nullptr || size == 0) {
        _begin = nullptr;
        _current = nullptr;
        _end = nullptr;
    } else {
        _begin = static_cast<u8*>(buffer);
        _current = _begin;
        _end = _begin + size;
    }
}

BumpAllocator::BumpAllocator(memory_size capacity, IAllocator& upstream) noexcept
    : _upstream(&upstream)
{
    _begin = static_cast<u8*>(AllocateBytes(upstream, capacity));
    if (_begin == nullptr) {
        _current = nullptr;
        _end = nullptr;
    } else {
        _current = _begin;
        _end = _begin + capacity;
    }
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
    
    if (_begin == nullptr) {
        return nullptr;
    }

    const memory_alignment alignment = detail::NormalizeAlignment(request.alignment);

#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(detail::IsValidAlignment(alignment));
#endif

    u8* aligned = AlignPtrUp(_current, alignment);
    
    if (aligned < _current || aligned > _end) {
        return nullptr;
    }
    
    memory_size available = static_cast<memory_size>(_end - aligned);
    if (request.size > available) {
#if CORE_MEMORY_DEBUG
        if (Any(request.flags & AllocationFlags::NoFail)) {
            CORE_MEM_ASSERT(false && "BumpAllocator: out of memory with NoFail flag");
        }
#endif
        return nullptr;
    }

    _current = aligned + request.size;

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
    if (_begin == nullptr) {
        return 0;
    }
    return static_cast<memory_size>(_current - _begin);
}

memory_size BumpAllocator::Capacity() const noexcept {
    if (_begin == nullptr) {
        return 0;
    }
    return static_cast<memory_size>(_end - _begin);
}

memory_size BumpAllocator::Remaining() const noexcept {
    if (_begin == nullptr) {
        return 0;
    }
    return static_cast<memory_size>(_end - _current);
}

bool BumpAllocator::Owns(const void* ptr) const noexcept {
    if (ptr == nullptr || _begin == nullptr) {
        return false;
    }
    const u8* p = static_cast<const u8*>(ptr);
    return p >= _begin && p < _end;
}

} // namespace core

