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

namespace {

memory_size GetPageSize() noexcept {
    static memory_size page_size = 0;
    if (page_size == 0) {
#if CORE_PLATFORM_WINDOWS
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        page_size = static_cast<memory_size>(si.dwPageSize);
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
        page_size = static_cast<memory_size>(sysconf(_SC_PAGESIZE));
#endif
    }
    return page_size;
}

} // anonymous namespace

void* SystemAllocator::Allocate(const AllocationRequest& request) noexcept {
    if (request.size == 0) {
        return nullptr;
    }

    const memory_size page_size = GetPageSize();
    
    if (request.alignment > page_size) {
#if CORE_MEMORY_DEBUG
        CORE_MEM_ASSERT(false && "SystemAllocator: requested alignment exceeds page size");
#endif
        return nullptr;
    }

    // TODO(epic #88): NotifyAllocationHook(AllocationEvent::AllocateBegin, this, &request, nullptr);

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

    // TODO(epic #88): Build AllocationInfo and call NotifyAllocationHook(AllocateEnd)
    
    return ptr;
}

void SystemAllocator::Deallocate(const AllocationInfo& info) noexcept {
    if (info.ptr == nullptr) {
        return;
    }

    // TODO(epic #88): NotifyAllocationHook(AllocationEvent::DeallocateBegin, this, nullptr, &info);

#if CORE_PLATFORM_WINDOWS
    VirtualFree(info.ptr, 0, MEM_RELEASE);
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    munmap(info.ptr, info.size);
#endif

    // TODO(epic #88): NotifyAllocationHook(AllocationEvent::DeallocateEnd, this, nullptr, &info);
}

SystemAllocator& SystemAllocator::Instance() noexcept {
    static SystemAllocator instance;
    return instance;
}

IAllocator& GetSystemAllocator() noexcept {
    return SystemAllocator::Instance();
}

} // namespace core

