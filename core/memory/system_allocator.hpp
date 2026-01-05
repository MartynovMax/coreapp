#pragma once

// =============================================================================
// system_allocator.hpp
// Low-level system memory allocator using OS-specific APIs.
//
// This is the foundational allocator that directly interfaces with the OS.
// All other allocators should use SystemAllocator as their backing allocator.
//
// Platform APIs:
//   Windows: VirtualAlloc / VirtualFree
//   POSIX:   mmap / munmap
//
// Alignment guarantees:
//   - All allocations are aligned to system page size (typically 4KB on x64)
//   - Requested alignment must be <= page size
//   - Requesting alignment > page size returns nullptr (asserts in debug)
//   - Page size is obtained once and cached (GetSystemInfo / sysconf)
//
// AllocationFlags:
//   - ZeroInitialize: Always satisfied (OS guarantees zero-initialized pages)
//   - Other flags: Currently ignored
//
// Thread-safety:
//   - Thread-safe (OS calls are thread-safe)
//   - Stateless, safe to use from multiple threads concurrently
//   - Instance() returns a singleton
//
// Deallocation:
//   - Windows: size parameter is ignored (MEM_RELEASE releases entire region)
//   - POSIX: size parameter is required and must match allocation size
//   - Deallocate(nullptr) is a no-op
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

