#pragma once

// =============================================================================
// system_allocator.hpp
// Low-level system memory allocator using OS-specific APIs.
//
// This is the foundational allocator that directly interfaces with the OS.
// All other allocators should use SystemAllocator as their backing allocator.
// =============================================================================

#include "core_memory.hpp"
#include "core_allocator.hpp"

namespace core {

class SystemAllocator : public IAllocator {
public:
    SystemAllocator() noexcept = default;
    ~SystemAllocator() noexcept override = default;

    SystemAllocator(const SystemAllocator&) = delete;
    SystemAllocator& operator=(const SystemAllocator&) = delete;

    void* Allocate(const AllocationRequest& request) noexcept override;
    void Deallocate(const AllocationInfo& info) noexcept override;

    static SystemAllocator& Instance() noexcept;
};

} // namespace core

