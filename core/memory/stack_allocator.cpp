#include "stack_allocator.hpp"
#include "memory_ops.hpp"

namespace core {

namespace detail {

#if CORE_MEMORY_DEBUG
constexpr u32 kStackHeaderMagic = 0xDEADBEEF;
#endif

CORE_FORCE_INLINE StackAllocationHeader* GetHeaderFromUserPtr(void* user_ptr) noexcept {
    u8* ptr = static_cast<u8*>(user_ptr);
    return reinterpret_cast<StackAllocationHeader*>(ptr - sizeof(StackAllocationHeader));
}

} // namespace detail

StackAllocator::StackAllocator(void* buffer, memory_size size) noexcept
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

StackAllocator::StackAllocator(memory_size capacity, IAllocator& upstream) noexcept
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

StackAllocator::~StackAllocator() noexcept {
    if (_upstream && _begin) {
        memory_size capacity = static_cast<memory_size>(_end - _begin);
        DeallocateBytes(*_upstream, _begin, capacity);
    }
}

void* StackAllocator::Allocate(const AllocationRequest& request) noexcept {
    if (request.size == 0) {
        return nullptr;
    }
    
    if (_begin == nullptr) {
        return nullptr;
    }

    memory_alignment alignment = detail::NormalizeAlignment(request.alignment);

#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(detail::IsValidAlignment(alignment));
#endif

    // Ensure user ptr alignment is at least header alignment to avoid unaligned header access
    const memory_alignment header_align = 
        static_cast<memory_alignment>(alignof(detail::StackAllocationHeader));
    if (alignment < header_align) {
        alignment = header_align;
    }

    // Step 1: Calculate where user data should start (aligned)
    u8* block_start = _current;
    u8* header_pos = block_start;
    u8* user_pos_unaligned = header_pos + sizeof(detail::StackAllocationHeader);
    u8* user_pos = AlignPtrUp(user_pos_unaligned, alignment);
    
    header_pos = user_pos - sizeof(detail::StackAllocationHeader);
    
    // Step 2: Calculate total size needed from block_start to end of user data
    u8* end_pos = user_pos + request.size;
    memory_size total_size = static_cast<memory_size>(end_pos - block_start);
    
    // Step 3: Check if we have enough space
    if (block_start + total_size > _end) {
#if CORE_MEMORY_DEBUG
        if (Any(request.flags & AllocationFlags::NoFail)) {
            CORE_MEM_ASSERT(false && "StackAllocator: out of memory with NoFail flag");
        }
#endif
        return nullptr;
    }
    
    // Step 4: Write allocation header
    auto* header = reinterpret_cast<detail::StackAllocationHeader*>(header_pos);
    header->total_size = total_size;
    header->user_size = request.size;
    
#if CORE_MEMORY_DEBUG
    header->magic = detail::kStackHeaderMagic;
    header->padding_bytes = 0;
#endif
    
    // Step 5: Update current pointer
    _current = end_pos;
    
    // Step 6: Zero-initialize if requested
    if (Any(request.flags & AllocationFlags::ZeroInitialize)) {
        memory_zero(user_pos, request.size);
    }
    
    return user_pos;
}

void StackAllocator::Deallocate(const AllocationInfo& info) noexcept {
    if (info.ptr == nullptr) {
        return;
    }
    
    u8* user_ptr = static_cast<u8*>(info.ptr);
    
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(Owns(info.ptr) && 
                    "StackAllocator: pointer does not belong to this allocator");
#endif
    
    // Step 1: Get header from user pointer
    auto* header = detail::GetHeaderFromUserPtr(user_ptr);
    
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(header->magic == detail::kStackHeaderMagic && 
                    "StackAllocator: corrupted or invalid allocation header");
    
    if (info.size != 0) {
        CORE_MEM_ASSERT(info.size == header->user_size &&
                        "StackAllocator: size mismatch between info and header");
    }
    
    u8* expected_end = user_ptr + header->user_size;
    CORE_MEM_ASSERT(expected_end == _current && 
                    "StackAllocator: LIFO violation - allocation is not at top of stack");
#endif
    
    // Step 2: Calculate block start from header metadata
    u8* block_start = user_ptr + header->user_size - header->total_size;
    
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(block_start >= _begin && block_start < _current &&
                    "StackAllocator: invalid block_start calculation");
#endif
    
    // Step 3: Rewind current pointer to block start
    _current = block_start;
}

bool StackAllocator::Owns(const void* ptr) const noexcept {
    if (ptr == nullptr || _begin == nullptr) {
        return false;
    }
    const u8* p = static_cast<const u8*>(ptr);
    return p >= _begin && p < _end;
}

StackAllocator::Marker StackAllocator::GetMarker() const noexcept {
    return Marker{_current};
}

void StackAllocator::RewindToMarker(Marker marker) noexcept {
#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(_begin != nullptr && 
                    "StackAllocator: cannot rewind uninitialized allocator");
    CORE_MEM_ASSERT(marker.position >= _begin && marker.position <= _current &&
                    "StackAllocator: invalid marker position");
#endif
    
    _current = marker.position;
}

void StackAllocator::Reset() noexcept {
    if (_begin == nullptr) {
        return;
    }
    _current = _begin;
}

memory_size StackAllocator::Used() const noexcept {
    if (_begin == nullptr) {
        return 0;
    }
    return static_cast<memory_size>(_current - _begin);
}

memory_size StackAllocator::Capacity() const noexcept {
    if (_begin == nullptr) {
        return 0;
    }
    return static_cast<memory_size>(_end - _begin);
}

memory_size StackAllocator::Remaining() const noexcept {
    if (_begin == nullptr) {
        return 0;
    }
    return static_cast<memory_size>(_end - _current);
}

} // namespace core

