#include "core/memory/malloc_allocator.hpp"
#include "core/memory/system_allocator.hpp"
#include "core/memory/core_allocator.hpp"
#include <gtest/gtest.h>

#if CORE_HAS_THREADS
    #include <thread>
    #include <vector>
#endif

namespace {

class TestBackingAllocator final : public core::IAllocator {
public:
    void* Allocate(const core::AllocationRequest& request) noexcept override {
        alloc_count_++;
        last_request_ = request;
        core::SystemAllocator& sys = core::SystemAllocator::Instance();
        return sys.Allocate(request);
    }

    void Deallocate(const core::AllocationInfo& info) noexcept override {
        dealloc_count_++;
        core::SystemAllocator& sys = core::SystemAllocator::Instance();
        sys.Deallocate(info);
    }

    core::memory_size alloc_count() const noexcept { return alloc_count_; }
    core::memory_size dealloc_count() const noexcept { return dealloc_count_; }
    const core::AllocationRequest& last_request() const noexcept { return last_request_; }

private:
    core::memory_size alloc_count_ = 0;
    core::memory_size dealloc_count_ = 0;
    core::AllocationRequest last_request_{};
};

TEST(MallocAllocator, BasicAllocation) {
    core::MallocAllocator allocator;

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

TEST(MallocAllocator, MultipleBlocks) {
    core::MallocAllocator allocator;

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

    core::AllocationInfo info1{ptr1, req1.size, req1.alignment, 0};
    core::AllocationInfo info2{ptr2, req2.size, req2.alignment, 0};
    core::AllocationInfo info3{ptr3, req3.size, req3.alignment, 0};

    allocator.Deallocate(info1);
    allocator.Deallocate(info2);
    allocator.Deallocate(info3);
}

TEST(MallocAllocator, DeallocateNullptr) {
    core::MallocAllocator allocator;

    core::AllocationInfo info{};
    info.ptr = nullptr;
    info.size = 1024;

    allocator.Deallocate(info);
}

TEST(MallocAllocator, DefaultConstructor) {
    core::MallocAllocator allocator;

    core::IAllocator& backing = allocator.GetBacking();
    core::SystemAllocator& sys = core::SystemAllocator::Instance();

    EXPECT_EQ(&backing, &sys);
}

TEST(MallocAllocator, CustomBacking) {
    TestBackingAllocator backing;
    core::MallocAllocator allocator(backing);

    EXPECT_EQ(&allocator.GetBacking(), &backing);

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    EXPECT_EQ(backing.alloc_count(), 1u);

    core::AllocationInfo info{ptr, req.size, req.alignment, 0};
    allocator.Deallocate(info);

    EXPECT_EQ(backing.dealloc_count(), 1u);
}

TEST(MallocAllocator, GetBacking) {
    TestBackingAllocator backing;
    core::MallocAllocator allocator(backing);

    core::IAllocator& retrieved = allocator.GetBacking();

    EXPECT_EQ(&retrieved, &backing);
}

TEST(MallocAllocator, DelegatesAlignment) {
    TestBackingAllocator backing;
    core::MallocAllocator allocator(backing);

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 64;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    const core::AllocationRequest& last = backing.last_request();
    EXPECT_EQ(last.size, req.size);
    EXPECT_EQ(last.alignment, req.alignment);

    core::AllocationInfo info{ptr, req.size, req.alignment, 0};
    allocator.Deallocate(info);
}

TEST(MallocAllocator, DelegatesFlags) {
    TestBackingAllocator backing;
    core::MallocAllocator allocator(backing);

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 16;
    req.flags = core::AllocationFlags::ZeroInitialize | core::AllocationFlags::Persistent;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    const core::AllocationRequest& last = backing.last_request();
    EXPECT_TRUE(core::Any(last.flags & core::AllocationFlags::ZeroInitialize));
    EXPECT_TRUE(core::Any(last.flags & core::AllocationFlags::Persistent));

    core::AllocationInfo info{ptr, req.size, req.alignment, 0};
    allocator.Deallocate(info);
}

TEST(MallocAllocator, GetDefaultAllocator) {
    core::IAllocator& allocator = core::GetDefaultAllocator();

    core::AllocationRequest req{};
    req.size = 1024;
    req.alignment = 16;

    void* ptr = allocator.Allocate(req);
    ASSERT_NE(ptr, nullptr);

    core::AllocationInfo info{ptr, req.size, req.alignment, 0};
    allocator.Deallocate(info);
}

TEST(MallocAllocator, DefaultAllocatorSingleton) {
    core::IAllocator& alloc1 = core::GetDefaultAllocator();
    core::IAllocator& alloc2 = core::GetDefaultAllocator();

    EXPECT_EQ(&alloc1, &alloc2);
}

TEST(MallocAllocator, AllocateObject) {
    core::MallocAllocator allocator;

    int* ptr = core::AllocateObject<int>(allocator);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, static_cast<core::memory_alignment>(alignof(int))));

    *ptr = 42;
    EXPECT_EQ(*ptr, 42);

    core::DeallocateObject<int>(allocator, ptr);
}

TEST(MallocAllocator, AllocateArray) {
    core::MallocAllocator allocator;

    constexpr core::memory_size count = 10;
    double* arr = core::AllocateArray<double>(allocator, count);
    ASSERT_NE(arr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(arr, static_cast<core::memory_alignment>(alignof(double))));

    for (core::memory_size i = 0; i < count; ++i) {
        arr[i] = static_cast<double>(i) * 3.14;
    }

    EXPECT_DOUBLE_EQ(arr[0], 0.0);
    EXPECT_DOUBLE_EQ(arr[5], 5.0 * 3.14);

    core::DeallocateArray<double>(allocator, arr, count);
}

TEST(MallocAllocator, AllocateObjectWithTag) {
    core::MallocAllocator allocator;

    constexpr core::memory_tag tag = 123;
    int* ptr = core::AllocateObject<int>(allocator, tag);
    ASSERT_NE(ptr, nullptr);

    core::DeallocateObject<int>(allocator, ptr, tag);
}

TEST(MallocAllocator, AllocateArrayWithTag) {
    core::MallocAllocator allocator;

    constexpr core::memory_size count = 5;
    constexpr core::memory_tag tag = 456;

    float* arr = core::AllocateArray<float>(allocator, count, tag);
    ASSERT_NE(arr, nullptr);

    core::DeallocateArray<float>(allocator, arr, count, tag);
}

TEST(MallocAllocator, AllocateObjectWithFlags) {
    core::MallocAllocator allocator;

    auto flags = core::AllocationFlags::ZeroInitialize;
    int* ptr = core::AllocateObject<int>(allocator, 0, flags);
    ASSERT_NE(ptr, nullptr);

    EXPECT_EQ(*ptr, 0);

    core::DeallocateObject<int>(allocator, ptr);
}

#if CORE_HAS_THREADS
TEST(MallocAllocator, MultithreadedSmokeTest) {
    core::MallocAllocator allocator;
    
    constexpr int thread_count = 4;
    constexpr int allocations_per_thread = 100;
    constexpr core::memory_size alloc_size = 512;
    
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

