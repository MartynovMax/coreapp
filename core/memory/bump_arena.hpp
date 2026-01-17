#pragma once

// =============================================================================
// bump_arena.hpp
// IArena implementation using bump allocation strategy.
//
// BumpArena wraps BumpAllocator to provide the IArena interface.
// Ideal for temporary/scoped memory with bulk deallocation.
// =============================================================================

#include "arena.hpp"
#include "bump_allocator.hpp"

namespace core {

class BumpArena final : public IArena {
public:
    BumpArena(void* buffer, memory_size size) noexcept;
    BumpArena(memory_size capacity, IAllocator& upstream) noexcept;
    BumpArena(memory_size capacity, IAllocator& upstream, const char* name) noexcept;
    
    ~BumpArena() noexcept override = default;

    BumpArena(const BumpArena&) = delete;
    BumpArena& operator=(const BumpArena&) = delete;

    void* Allocate(memory_size size, memory_alignment alignment = CORE_DEFAULT_ALIGNMENT) noexcept override;
    void Reset() noexcept override;
    
    ArenaMarker GetMarker() const noexcept override;
    void RewindTo(ArenaMarker marker) noexcept override;
    
    memory_size Capacity() const noexcept override;
    memory_size Used() const noexcept override;
    memory_size Remaining() const noexcept override;
    
    bool Owns(const void* ptr) const noexcept override;
    const char* Name() const noexcept override;

private:
    BumpAllocator _allocator;
    const char* _name = nullptr;
};

} // namespace core

