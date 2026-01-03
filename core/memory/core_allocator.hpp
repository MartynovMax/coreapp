#pragma once

// =============================================================================
// core_allocator.hpp
// Base allocator interface and allocation helpers.
//
// Provides:
//   - IAllocator interface
//   - AllocationRequest / AllocationInfo structures
//   - Typed helpers: AllocateObject, AllocateArray, DeallocateObject, DeallocateArray
//   - Allocator adapters: AllocatorRef, TypedAllocator<T>
//   - Default allocator accessors: DefaultAllocator(), SystemAllocator()
//   - Allocation hooks for instrumentation
// =============================================================================

#include "../base/core_config.hpp"
#include "../base/core_assert.hpp"
#include "../base/core_types.hpp"
#include "../base/core_inline.hpp"

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
// Base allocator interface
// ----------------------------------------------------------------------------
//
// Contract:
//   - Allocate() must return memory aligned to request.alignment (or default)
//   - Allocate() returns nullptr on failure (unless NoFail flag is set)
//   - Deallocate() must accept nullptr (no-op)
//   - Deallocate() info must match original Allocate() request
//   - tag is optional metadata for tracking/debugging (0 is valid)
//   - Thread-safety is allocator-specific (document in derived classes)

class IAllocator {
public:
    virtual ~IAllocator() = default;

    virtual void* Allocate(const AllocationRequest& request) noexcept = 0;
    virtual void Deallocate(const AllocationInfo& info) noexcept = 0;

    virtual bool TryGetStats(AllocatorStats& out_stats) const noexcept {
        CORE_UNUSED(out_stats);
        return false;
    }
};

// ----------------------------------------------------------------------------
// Typed allocation helpers
// ----------------------------------------------------------------------------

template <typename T>
CORE_FORCE_INLINE T* AllocateObject(
    IAllocator& allocator,
    memory_tag tag = 0,
    AllocationFlags flags = AllocationFlags::None) noexcept
{
    AllocationRequest req;
    req.size = static_cast<memory_size>(sizeof(T));
    req.alignment = static_cast<memory_alignment>(alignof(T));
    req.tag = tag;
    req.flags = flags;
    
    void* ptr = allocator.Allocate(req);
    return static_cast<T*>(ptr);
}

template <typename T>
CORE_FORCE_INLINE void DeallocateObject(
    IAllocator& allocator,
    T* ptr,
    memory_tag tag = 0) noexcept
{
    if (ptr == nullptr) {
        return;
    }
    
    AllocationInfo info;
    info.ptr = ptr;
    info.size = static_cast<memory_size>(sizeof(T));
    info.alignment = static_cast<memory_alignment>(alignof(T));
    info.tag = tag;
    
    allocator.Deallocate(info);
}

template <typename T>
CORE_FORCE_INLINE T* AllocateArray(
    IAllocator& allocator,
    memory_size count,
    memory_tag tag = 0,
    AllocationFlags flags = AllocationFlags::None) noexcept
{
    const memory_size total = BytesFor<T>(count);
    
    AllocationRequest req;
    req.size = total;
    req.alignment = static_cast<memory_alignment>(alignof(T));
    req.tag = tag;
    req.flags = flags;
    
    void* ptr = allocator.Allocate(req);
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
    
    AllocationInfo info;
    info.ptr = ptr;
    info.size = BytesFor<T>(count);
    info.alignment = static_cast<memory_alignment>(alignof(T));
    info.tag = tag;
    
    allocator.Deallocate(info);
}

// ----------------------------------------------------------------------------
// Default allocator accessors
// ----------------------------------------------------------------------------

IAllocator& DefaultAllocator() noexcept;
IAllocator& SystemAllocator() noexcept;

#if CORE_MEMORY_DEBUG
IAllocator& DebugAllocator() noexcept;
#endif

// ----------------------------------------------------------------------------
// Memory hook declarations
// ----------------------------------------------------------------------------

using AllocationHookFn = void (*)(const AllocationRequest& req,
                                   const AllocationInfo& info,
                                   void* user) noexcept;

using DeallocationHookFn = void (*)(const AllocationInfo& info,
                                     void* user) noexcept;

bool RegisterAllocationHook(AllocationHookFn hook, void* user) noexcept;
bool UnregisterAllocationHook(AllocationHookFn hook, void* user) noexcept;

bool RegisterDeallocationHook(DeallocationHookFn hook, void* user) noexcept;
bool UnregisterDeallocationHook(DeallocationHookFn hook, void* user) noexcept;

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

