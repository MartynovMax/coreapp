#pragma once

// =============================================================================
// core_allocator.hpp
// Base allocator interface and allocation helpers.
//
// Provides:
//   - IAllocator interface
//   - AllocationRequest / AllocationInfo structures
//   - Raw byte helpers: AllocateBytes, DeallocateBytes
//   - Typed helpers: AllocateObject, AllocateArray, DeallocateObject, DeallocateArray
//   - Allocator adapters: AllocatorRef, TypedAllocator<T>
//   - Default allocator accessors: GetDefaultAllocator(), GetSystemAllocator()
//   - Allocation hook system (multiple listeners, zero overhead when unused)
// =============================================================================

#include "core_memory.hpp"
#include "memory_traits.hpp"

namespace core {

// ----------------------------------------------------------------------------
// Allocation flags and structures
// ----------------------------------------------------------------------------

enum class AllocationFlags : u32 {
    None = 0u,
    Transient = 1u << 0,
    Persistent = 1u << 1,
    ZeroInitialize = 1u << 2,
    NoFail = 1u << 3,
};

CORE_FORCE_INLINE constexpr AllocationFlags operator|(AllocationFlags a, AllocationFlags b) noexcept {
    return static_cast<AllocationFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

CORE_FORCE_INLINE constexpr AllocationFlags operator&(AllocationFlags a, AllocationFlags b) noexcept {
    return static_cast<AllocationFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

CORE_FORCE_INLINE constexpr bool Any(AllocationFlags f) noexcept {
    return static_cast<u32>(f) != 0u;
}

struct AllocationRequest {
    memory_size size = 0;
    memory_alignment alignment = CORE_DEFAULT_ALIGNMENT;
    memory_tag tag = 0;
    AllocationFlags flags = AllocationFlags::None;
};

struct AllocationInfo {
    void* ptr = nullptr;
    memory_size size = 0;
    memory_alignment alignment = CORE_DEFAULT_ALIGNMENT;
    memory_tag tag = 0;
};

struct AllocatorStats {
    memory_size bytes_allocated_current = 0;
    memory_size bytes_allocated_peak = 0;
    memory_size bytes_allocated_total = 0;
    memory_size alloc_count_total = 0;
    memory_size free_count_total = 0;
};

// ----------------------------------------------------------------------------
// Internal helpers
// ----------------------------------------------------------------------------

namespace detail {

// Normalize alignment: 0 -> CORE_DEFAULT_ALIGNMENT
CORE_FORCE_INLINE constexpr memory_alignment NormalizeAlignment(memory_alignment a) noexcept {
    return (a == 0) ? CORE_DEFAULT_ALIGNMENT : a;
}

// Check if alignment is valid (non-zero power of two)
CORE_FORCE_INLINE constexpr bool IsValidAlignment(memory_alignment a) noexcept {
    return (a != 0) && IsPowerOfTwo(a);
}

// Detect multiplication overflow
CORE_FORCE_INLINE constexpr bool MulOverflow(memory_size a, memory_size b, memory_size& out) noexcept {
#if (CORE_COMPILER_CLANG || CORE_COMPILER_GCC) && CORE_HAS_BUILTIN(__builtin_mul_overflow)
    bool overflow = __builtin_mul_overflow(a, b, &out);
    if (overflow) {
        out = 0; // Match fallback behavior
    }
    return overflow;
#else
    // Fallback implementation
    if (a == 0 || b == 0) {
        out = 0;
        return false;
    }
    const memory_size maxv = static_cast<memory_size>(~static_cast<memory_size>(0));
    if (a > (maxv / b)) {
        out = 0;
        return true;
    }
    out = a * b;
    return false;
#endif
}

} // namespace detail

// ----------------------------------------------------------------------------
// Base allocator interface
// ----------------------------------------------------------------------------
//
// Contract:
//   - Allocate() must return memory aligned to request.alignment (or default)
//   - Allocate() returns nullptr on failure (unless NoFail flag is set)
//   - Deallocate() must accept nullptr (no-op)
//   - Deallocate() info fields (size/alignment/tag) are hints to the allocator;
//     specific allocators may require exact match with Allocate() - see their docs
//   - Thread-safety is allocator-specific (document in derived classes)

class IAllocator {
public:
    virtual ~IAllocator() = default;

    virtual void* Allocate(const AllocationRequest& request) noexcept = 0;
    virtual void Deallocate(const AllocationInfo& info) noexcept = 0;

    virtual bool Owns(const void* ptr) const noexcept {
        CORE_UNUSED(ptr);
        return false;
    }

    virtual bool TryGetStats(AllocatorStats& out_stats) const noexcept {
        CORE_UNUSED(out_stats);
        return false;
    }
};

// ----------------------------------------------------------------------------
// Memory hook system
// ----------------------------------------------------------------------------

// Hook events
enum class AllocationEvent : u8 {
    AllocateBegin,    // Before allocator.Allocate()
    AllocateEnd,      // After allocator.Allocate()
    DeallocateBegin,  // Before allocator.Deallocate()
    DeallocateEnd,    // After allocator.Deallocate()
};

// Hook callback signature
// Pointer validity depends on event:
//   AllocateBegin:   allocator, request valid; info is nullptr
//   AllocateEnd:     allocator, request, info all valid
//   DeallocateBegin: allocator, info valid; request is nullptr
//   DeallocateEnd:   allocator, info valid; request is nullptr
using AllocationHookFn = void (*)(AllocationEvent event,
                                   const IAllocator* allocator,
                                   const AllocationRequest* request,
                                   const AllocationInfo* info,
                                   void* user) noexcept;

// Hook registration (supports multiple listeners)
// Returns true if successfully registered, false if already registered or no space
bool AddAllocationHook(AllocationHookFn fn, void* user = nullptr) noexcept;
bool RemoveAllocationHook(AllocationHookFn fn) noexcept;

// Query hooks
bool HasAllocationHooks() noexcept;
void ClearAllocationHooks() noexcept;

// Internal dispatch function (implemented in .cpp)
void DispatchAllocationHook(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info) noexcept;

// Inline helper to notify all hooks (zero overhead when no hooks)
CORE_FORCE_INLINE void NotifyAllocationHook(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info) noexcept
{
    // Fast path: no hooks registered (common case)
    if (HasAllocationHooks()) {
        DispatchAllocationHook(event, allocator, request, info);
    }
}

// ----------------------------------------------------------------------------
// Raw byte allocation helpers
// ----------------------------------------------------------------------------

CORE_FORCE_INLINE void* AllocateBytes(
    IAllocator& allocator,
    memory_size size,
    memory_alignment alignment = CORE_DEFAULT_ALIGNMENT,
    memory_tag tag = 0,
    AllocationFlags flags = AllocationFlags::None) noexcept
{
    AllocationRequest req;
    req.size = size;
    req.alignment = detail::NormalizeAlignment(alignment);
    req.tag = tag;
    req.flags = flags;

#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(detail::IsValidAlignment(req.alignment));
#endif

    NotifyAllocationHook(AllocationEvent::AllocateBegin, &allocator, &req, nullptr);
    
    void* ptr = allocator.Allocate(req);
    
    if (ptr != nullptr) {
        AllocationInfo info;
        info.ptr = ptr;
        info.size = req.size;
        info.alignment = req.alignment;
        info.tag = req.tag;
        NotifyAllocationHook(AllocationEvent::AllocateEnd, &allocator, &req, &info);
    }
    
    return ptr;
}

CORE_FORCE_INLINE void DeallocateBytes(
    IAllocator& allocator,
    void* ptr,
    memory_size size = 0,
    memory_alignment alignment = CORE_DEFAULT_ALIGNMENT,
    memory_tag tag = 0) noexcept
{
    if (ptr == nullptr) {
        return;
    }

    AllocationInfo info;
    info.ptr = ptr;
    info.size = size;
    info.alignment = detail::NormalizeAlignment(alignment);
    info.tag = tag;

#if CORE_MEMORY_DEBUG
    CORE_MEM_ASSERT(detail::IsValidAlignment(info.alignment));
#endif

    NotifyAllocationHook(AllocationEvent::DeallocateBegin, &allocator, nullptr, &info);
    allocator.Deallocate(info);
    NotifyAllocationHook(AllocationEvent::DeallocateEnd, &allocator, nullptr, &info);
}

// ----------------------------------------------------------------------------
// Typed allocation helpers
// ----------------------------------------------------------------------------

template <typename T>
CORE_FORCE_INLINE T* AllocateObject(
    IAllocator& allocator,
    memory_tag tag = 0,
    AllocationFlags flags = AllocationFlags::None) noexcept
{
    void* ptr = AllocateBytes(
        allocator,
        static_cast<memory_size>(sizeof(T)),
        static_cast<memory_alignment>(alignof(T)),
        tag,
        flags
    );
    return static_cast<T*>(ptr);
}

template <typename T>
CORE_FORCE_INLINE void DeallocateObject(
    IAllocator& allocator,
    T* ptr,
    memory_tag tag = 0) noexcept
{
    DeallocateBytes(
        allocator,
        ptr,
        static_cast<memory_size>(sizeof(T)),
        static_cast<memory_alignment>(alignof(T)),
        tag
    );
}

template <typename T>
CORE_FORCE_INLINE T* AllocateArray(
    IAllocator& allocator,
    memory_size count,
    memory_tag tag = 0,
    AllocationFlags flags = AllocationFlags::None) noexcept
{
    memory_size total = 0;
    if (detail::MulOverflow(static_cast<memory_size>(sizeof(T)), count, total)) {
        return nullptr;
    }
    
    void* ptr = AllocateBytes(
        allocator,
        total,
        static_cast<memory_alignment>(alignof(T)),
        tag,
        flags
    );
    return static_cast<T*>(ptr);
}

template <typename T>
CORE_FORCE_INLINE void DeallocateArray(
    IAllocator& allocator,
    T* ptr,
    memory_size count,
    memory_tag tag = 0) noexcept
{
    if (ptr == nullptr) {
        return;
    }
    
    memory_size total = 0;
    if (detail::MulOverflow(static_cast<memory_size>(sizeof(T)), count, total)) {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(false && "DeallocateArray: invalid count causes overflow");
#endif
        return; 
    }
    
    DeallocateBytes(
        allocator,
        ptr,
        total,
        static_cast<memory_alignment>(alignof(T)),
        tag
    );
}

// ----------------------------------------------------------------------------
// Default allocator accessors
// ----------------------------------------------------------------------------

IAllocator& GetSystemAllocator() noexcept;
IAllocator& GetDefaultAllocator() noexcept;

#if CORE_MEMORY_DEBUG
IAllocator& GetDebugAllocator() noexcept;
#endif

// ----------------------------------------------------------------------------
// Allocator adapters
// ----------------------------------------------------------------------------

class AllocatorRef {
public:
    CORE_FORCE_INLINE AllocatorRef() noexcept = default;
    CORE_FORCE_INLINE explicit AllocatorRef(IAllocator& a) noexcept : ptr_(&a) {}
    CORE_FORCE_INLINE explicit AllocatorRef(IAllocator* a) noexcept : ptr_(a) {}

    CORE_FORCE_INLINE IAllocator* Ptr() const noexcept { return ptr_; }
    CORE_FORCE_INLINE explicit operator bool() const noexcept { return ptr_ != nullptr; }

    CORE_FORCE_INLINE IAllocator& Get() const noexcept {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(ptr_ != nullptr);
#endif
        return *ptr_;
    }

    CORE_FORCE_INLINE IAllocator* operator->() const noexcept { return ptr_; }

private:
    IAllocator* ptr_ = nullptr;
};

template <typename T>
class TypedAllocator {
public:
    using value_type = T;

    CORE_FORCE_INLINE TypedAllocator() noexcept = default;
    CORE_FORCE_INLINE explicit TypedAllocator(IAllocator& a) noexcept : allocator_(&a) {}
    CORE_FORCE_INLINE explicit TypedAllocator(AllocatorRef a) noexcept : allocator_(a.Ptr()) {}

    CORE_FORCE_INLINE IAllocator* Resource() const noexcept { return allocator_; }

    CORE_FORCE_INLINE T* Allocate(
        memory_size count = 1,
        memory_tag tag = 0,
        AllocationFlags flags = AllocationFlags::None) noexcept
    {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(allocator_ != nullptr);
#endif
        return AllocateArray<T>(*allocator_, count, tag, flags);
    }

    CORE_FORCE_INLINE void Deallocate(
        T* ptr,
        memory_size count = 1,
        memory_tag tag = 0) noexcept
    {
        if (allocator_ == nullptr) {
#if CORE_MEMORY_DEBUG
            CORE_MEM_ASSERT(ptr == nullptr);
#endif
            return;
        }
        DeallocateArray<T>(*allocator_, ptr, count, tag);
    }

private:
    IAllocator* allocator_ = nullptr;
};

} // namespace core

