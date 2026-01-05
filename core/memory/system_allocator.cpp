#include "system_allocator.hpp"

#if CORE_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    #include <sys/mman.h>
    #include <unistd.h>
#else
    #error "SystemAllocator: Unsupported platform. Supported platforms: Windows, Linux, macOS"
#endif

namespace core {

void* SystemAllocator::Allocate(const AllocationRequest& request) noexcept {
    if (request.size == 0) {
        return nullptr;
    }

#if CORE_PLATFORM_WINDOWS
    void* ptr = VirtualAlloc(
        nullptr,
        request.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    void* ptr = mmap(
        nullptr,
        request.size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (ptr == MAP_FAILED) {
        ptr = nullptr;
    }
#endif
    
    return ptr;
}

void SystemAllocator::Deallocate(const AllocationInfo& info) noexcept {
    if (info.ptr == nullptr) {
        return;
    }

#if CORE_PLATFORM_WINDOWS
    VirtualFree(info.ptr, 0, MEM_RELEASE);
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    munmap(info.ptr, info.size);
#endif
}

SystemAllocator& SystemAllocator::Instance() noexcept {
    static SystemAllocator instance;
    return instance;
}

IAllocator& GetSystemAllocator() noexcept {
    return SystemAllocator::Instance();
}

} // namespace core

