#pragma once

// =============================================================================
// arena.hpp
// Unified arena interface for linear/stack-style memory allocation.
//
// Provides fast temporary memory allocation with bulk deallocation.
// Ideal for per-frame allocations, parsing, scratch buffers, etc.
//
// Ownership and Lifetime:
//   - Arenas may own backing memory (allocated from upstream) or
//     operate on external buffers (non-owning)
//   - Owning arenas deallocate backing memory in destructor
//   - Non-owning arenas require external buffer lifetime > arena lifetime
//   - Reset/RewindTo affect only arena state, never backing allocator
//
// Marker Lifetime:
//   - Markers valid only for the arena that created them
//   - Markers invalidated by Reset() or arena destruction
//   - Using invalid markers is undefined behavior
//   - Concrete arenas may enforce additional ordering constraints
//
// Thread-safety:
//   - NOT thread-safe by default
//   - Requires external synchronization or per-thread instances
//
// Integration with IAllocator and Tracking:
//   - Arenas can be wrapped as IAllocator via ArenaAllocatorAdapter
//   - Tracking hooks are guaranteed only when using IAllocator path
//   - Direct IArena::Allocate() calls may bypass tracking (implementation-specific)
//   - Use adapter for generic code requiring IAllocator interface
// =============================================================================

#include "core_memory.hpp"
#include "memory_traits.hpp"
#include "core_allocator.hpp"

namespace core {

// ----------------------------------------------------------------------------
// ArenaMarker - Checkpoint for arena state
// ----------------------------------------------------------------------------

struct ArenaMarker {
    void* internal_state = nullptr;
    
#if CORE_MEMORY_DEBUG
    u32 debug_arena_id = 0;
    u32 debug_sequence = 0;
#endif
    
    bool operator==(const ArenaMarker& other) const noexcept {
        return internal_state == other.internal_state;
    }
    
    bool operator!=(const ArenaMarker& other) const noexcept {
        return internal_state != other.internal_state;
    }
};

static_assert(sizeof(ArenaMarker) <= 16, "ArenaMarker should be small");
static_assert(is_trivially_copyable_v<ArenaMarker>(), "ArenaMarker must be trivially copyable");

// ----------------------------------------------------------------------------
// IArena - Arena allocator interface
// ----------------------------------------------------------------------------

class IArena {
public:
    virtual ~IArena() = default;
    
    // Allocate memory from arena
    // Returns nullptr on failure
    virtual void* Allocate(
        memory_size size,
        memory_alignment alignment = CORE_DEFAULT_ALIGNMENT
    ) noexcept = 0;
    
    // Release all arena allocations at once
    virtual void Reset() noexcept = 0;
    
    // Capture current arena state
    virtual ArenaMarker GetMarker() const noexcept = 0;
    
    // Rewind allocations back to marker
    virtual void RewindTo(ArenaMarker marker) noexcept = 0;
    
    // Introspection
    virtual memory_size Capacity() const noexcept = 0;
    virtual memory_size Used() const noexcept = 0;
    virtual memory_size Remaining() const noexcept = 0;
    
    virtual const char* Name() const noexcept { return nullptr; }
    virtual bool Owns(const void* ptr) const noexcept { CORE_UNUSED(ptr); return false; }
};

// ----------------------------------------------------------------------------
// ArenaAllocatorAdapter - Wrap IArena as IAllocator
// ----------------------------------------------------------------------------
//
// Allows arenas to be used with generic code expecting IAllocator interface.
// Enables automatic integration with Core allocation tracking hooks.
//
// Semantics:
//   - Allocate() forwards to arena.Allocate()
//   - Deallocate() is NO-OP (arenas don't support individual frees)
//   - Owns() forwards to arena.Owns()
//   - TryGetStats() returns false (arena stats not compatible with IAllocator)
//
// Note: Individual deallocations are silently ignored.
//       Use Reset() on underlying arena to free all memory.

class ArenaAllocatorAdapter final : public IAllocator {
public:
    explicit ArenaAllocatorAdapter(IArena& arena) noexcept
        : arena_(&arena)
    {}
    
    void* Allocate(const AllocationRequest& request) noexcept override {
        return arena_->Allocate(request.size, request.alignment);
    }
    
    void Deallocate(const AllocationInfo& info) noexcept override {
        CORE_UNUSED(info);
        // Arena doesn't support individual deallocation - NO-OP
    }
    
    bool Owns(const void* ptr) const noexcept override {
        return arena_->Owns(ptr);
    }
    
    bool TryGetStats(AllocatorStats& out_stats) const noexcept override {
        CORE_UNUSED(out_stats);
        // Arena stats (Used/Capacity) don't map cleanly to allocator stats
        // (peak/total/counts). Return false to indicate stats unavailable.
        return false;
    }
    
    IArena& GetArena() const noexcept {
        return *arena_;
    }
    
private:
    IArena* arena_;
};

} // namespace core

