#include "bump_allocator.hpp"
#include "memory_ops.hpp"

namespace core {

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

} // namespace core

