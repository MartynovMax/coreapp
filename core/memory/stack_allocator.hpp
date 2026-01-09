#pragma once

// =============================================================================
// stack_allocator.hpp
// LIFO (stack-style) allocator with per-allocation deallocation support.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"
#include "memory_traits.hpp"

namespace core {

class StackAllocator final : public IAllocator {
public:
    StackAllocator(void* buffer, memory_size size) noexcept;
    StackAllocator(memory_size capacity, IAllocator& upstream) noexcept;
    
    ~StackAllocator() noexcept override;

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;

    bool Owns(const void* ptr) const noexcept override;

private:
    u8* _begin;
    u8* _current;
    u8* _end;
    IAllocator* _upstream;
};

} // namespace core

