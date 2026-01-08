#include "core/memory/system_allocator.hpp"
#include "core/memory/core_allocator.hpp"
#include <gtest/gtest.h>

#if CORE_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    #include <unistd.h>
#endif

#if CORE_HAS_THREADS
    #include <thread>
    #include <vector>
#endif

namespace {

TEST(SystemAllocator, BasicAllocation) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, req.alignment));

    core::AllocationInfo info{};
    info.ptr = ptr;
    info.size = req.size;
    info.alignment = req.alignment;

    allocator.Deallocate(info);
}

TEST(SystemAllocator, MultipleBlocks) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req1{};
    req1.size = 512;
    req1.alignment = 8;

    core::AllocationRequest req2{};
    req2.size = 2048;
    req2.alignment = 16;

    core::AllocationRequest req3{};
    req3.size = 4096;
    req3.alignment = 32;

    void* ptr1 = allocator.Allocate(req1);
    void* ptr2 = allocator.Allocate(req2);
    void* ptr3 = allocator.Allocate(req3);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    EXPECT_NE(ptr1, ptr2);
    EXPECT_NE(ptr2, ptr3);
    EXPECT_NE(ptr1, ptr3);

    EXPECT_TRUE(core::IsAlignedPtr(ptr1, req1.alignment));
    EXPECT_TRUE(core::IsAlignedPtr(ptr2, req2.alignment));
    EXPECT_TRUE(core::IsAlignedPtr(ptr3, req3.alignment));

    core::AllocationInfo info1{ptr1, req1.size, req1.alignment, 0};
    core::AllocationInfo info2{ptr2, req2.size, req2.alignment, 0};
    core::AllocationInfo info3{ptr3, req3.size, req3.alignment, 0};

    allocator.Deallocate(info1);
    allocator.Deallocate(info2);
    allocator.Deallocate(info3);
}

TEST(SystemAllocator, DeallocateNullptr) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationInfo info{};
    info.ptr = nullptr;
    info.size = 1024;

    allocator.Deallocate(info);
}

TEST(SystemAllocator, ZeroSize) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 0;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    EXPECT_EQ(ptr, nullptr);
}

TEST(SystemAllocator, AlignmentValidation) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 0;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    EXPECT_TRUE(core::IsAlignedPtr(ptr, CORE_DEFAULT_ALIGNMENT));

    core::AllocationInfo info{ptr, req.size, CORE_DEFAULT_ALIGNMENT, 0};
    allocator.Deallocate(info);
}

#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST
TEST(SystemAllocator, InvalidAlignmentAsserts) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 3;

    EXPECT_DEATH({
        allocator.Allocate(req);
    }, ".*");
}
#endif

#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST
TEST(SystemAllocator, ExceedsPageSize) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 1024 * 1024;

    EXPECT_DEATH({
        allocator.Allocate(req);
    }, ".*");
}
#endif

TEST(SystemAllocator, PageAligned) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 8192;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

#if CORE_PLATFORM_WINDOWS
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    const core::memory_size page_size = sys_info.dwPageSize;
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    const core::memory_size page_size = static_cast<core::memory_size>(sysconf(_SC_PAGESIZE));
#else
    const core::memory_size page_size = 4096;
#endif

    EXPECT_TRUE(core::IsAlignedPtr(ptr, page_size));

    core::AllocationInfo info{ptr, req.size, req.alignment, 0};
    allocator.Deallocate(info);
}

TEST(SystemAllocator, ZeroInitialize) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 4096;
    req.alignment = 16;
    req.flags = core::AllocationFlags::ZeroInitialize;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    const core::byte* bytes = static_cast<const core::byte*>(ptr);
    bool all_zeros = true;
    for (core::memory_size i = 0; i < req.size; ++i) {
        if (bytes[i] != 0) {
            all_zeros = false;
            break;
        }
    }

    EXPECT_TRUE(all_zeros);

    core::AllocationInfo info{ptr, req.size, req.alignment, 0};
    allocator.Deallocate(info);
}

#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST && (CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS)
TEST(SystemAllocator, POSIXDeallocateSizeZeroAsserts) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 4096;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    core::AllocationInfo info{};
    info.ptr = ptr;
    info.size = 0;

    EXPECT_DEATH({
        allocator.Deallocate(info);
    }, ".*");
}
#endif

#if CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
TEST(SystemAllocator, POSIXRequiresCorrectSize) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();

    core::AllocationRequest req{};
    req.size = 4096;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    core::AllocationInfo info{};
    info.ptr = ptr;
    info.size = req.size;

    allocator.Deallocate(info);
}
#endif

TEST(SystemAllocator, Instance) {
    core::SystemAllocator& inst1 = core::SystemAllocator::Instance();
    core::SystemAllocator& inst2 = core::SystemAllocator::Instance();

    EXPECT_EQ(&inst1, &inst2);
}

TEST(SystemAllocator, GetAccessor) {
    core::IAllocator& allocator = core::GetSystemAllocator();

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    core::AllocationInfo info{ptr, req.size, req.alignment, 0};
    allocator.Deallocate(info);
}

TEST(SystemAllocator, GetAccessorReturnsSameInstance) {
    core::IAllocator& alloc1 = core::GetSystemAllocator();
    core::IAllocator& alloc2 = core::GetSystemAllocator();
    core::SystemAllocator& inst = core::SystemAllocator::Instance();

    EXPECT_EQ(&alloc1, &alloc2);
    EXPECT_EQ(&alloc1, &inst);
}

#if CORE_HAS_THREADS
TEST(SystemAllocator, MultithreadedSmokeTest) {
    core::SystemAllocator& allocator = core::SystemAllocator::Instance();
    
    constexpr int thread_count = 4;
    constexpr int allocations_per_thread = 100;
    constexpr core::memory_size alloc_size = 1024;
    
    auto worker = [&allocator](int thread_id) {
        CORE_UNUSED(thread_id);
        
        for (int i = 0; i < allocations_per_thread; ++i) {
            core::AllocationRequest req{};
            req.size = alloc_size;
            req.alignment = 16;
            
            void* ptr = allocator.Allocate(req);
            ASSERT_NE(ptr, nullptr);
            EXPECT_TRUE(core::IsAlignedPtr(ptr, req.alignment));
            
            static_cast<core::byte*>(ptr)[0] = static_cast<core::byte>(i);
            EXPECT_EQ(static_cast<core::byte*>(ptr)[0], static_cast<core::byte>(i));
            
            core::AllocationInfo info{ptr, req.size, req.alignment, 0};
            allocator.Deallocate(info);
        }
    };
    
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
}
#endif

} // namespace

