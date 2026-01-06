#include "core/memory/core_allocator.hpp"
#include "core/memory/malloc_allocator.hpp"
#include <gtest/gtest.h>

namespace {

struct HookTestData {
    int allocateBeginCount = 0;
    int allocateEndCount = 0;
    int deallocateBeginCount = 0;
    int deallocateEndCount = 0;
    void* lastPtr = nullptr;
    core::memory_size lastSize = 0;
};

void TestHookCallback(
    core::AllocationEvent event,
    const core::IAllocator* allocator,
    const core::AllocationRequest* request,
    const core::AllocationInfo* info,
    void* user) noexcept
{
    auto* data = static_cast<HookTestData*>(user);
    
    switch (event) {
        case core::AllocationEvent::AllocateBegin:
            data->allocateBeginCount++;
            if (request) {
                data->lastSize = request->size;
            }
            break;
            
        case core::AllocationEvent::AllocateEnd:
            data->allocateEndCount++;
            if (info) {
                data->lastPtr = info->ptr;
                data->lastSize = info->size;
            }
            break;
            
        case core::AllocationEvent::DeallocateBegin:
            data->deallocateBeginCount++;
            if (info) {
                data->lastPtr = info->ptr;
                data->lastSize = info->size;
            }
            break;
            
        case core::AllocationEvent::DeallocateEnd:
            data->deallocateEndCount++;
            break;
    }
}

// Second hook callback for testing multiple hooks
// Note: Must be different implementation to prevent compiler from aliasing
void SecondHookCallback(
    core::AllocationEvent event,
    const core::IAllocator* allocator,
    const core::AllocationRequest* request,
    const core::AllocationInfo* info,
    void* user) noexcept
{
    CORE_UNUSED(allocator);
    
    auto* data = static_cast<HookTestData*>(user);
    
    // Same logic as TestHookCallback but with unique implementation
    // to ensure different function pointer address
    switch (event) {
        case core::AllocationEvent::AllocateBegin:
            data->allocateBeginCount++;
            data->lastSize = request ? request->size : 0;
            break;
            
        case core::AllocationEvent::AllocateEnd:
            data->allocateEndCount++;
            if (info) {
                data->lastPtr = info->ptr;
                data->lastSize = info->size;
            }
            break;
            
        case core::AllocationEvent::DeallocateBegin:
            data->deallocateBeginCount++;
            if (info) {
                data->lastPtr = info->ptr;
                data->lastSize = info->size;
            }
            break;
            
        case core::AllocationEvent::DeallocateEnd:
            data->deallocateEndCount++;
            break;
    }
}

TEST(AllocationHooks, RegisterAndUnregister) {
    EXPECT_FALSE(core::HasAllocationHooks());
    
    HookTestData data;
    bool registered = core::AddAllocationHook(TestHookCallback, &data);
    EXPECT_TRUE(registered);
    EXPECT_TRUE(core::HasAllocationHooks());
    
    bool removed = core::RemoveAllocationHook(TestHookCallback);
    EXPECT_TRUE(removed);
    EXPECT_FALSE(core::HasAllocationHooks());
}

TEST(AllocationHooks, PreventDuplicateRegistration) {
    core::ClearAllocationHooks();
    
    HookTestData data;
    bool registered1 = core::AddAllocationHook(TestHookCallback, &data);
    EXPECT_TRUE(registered1);
    
    bool registered2 = core::AddAllocationHook(TestHookCallback, &data);
    EXPECT_FALSE(registered2);
    
    core::ClearAllocationHooks();
}

TEST(AllocationHooks, ClearHooks) {
    core::ClearAllocationHooks();
    
    HookTestData data;
    core::AddAllocationHook(TestHookCallback, &data);
    EXPECT_TRUE(core::HasAllocationHooks());
    
    core::ClearAllocationHooks();
    EXPECT_FALSE(core::HasAllocationHooks());
}

TEST(AllocationHooks, HooksReceiveAllocateEvents) {
    core::ClearAllocationHooks();
    
    HookTestData data;
    core::AddAllocationHook(TestHookCallback, &data);
    
    core::MallocAllocator allocator;
    
    void* ptr = core::AllocateBytes(allocator, 128);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_EQ(data.allocateBeginCount, 1);
    EXPECT_EQ(data.allocateEndCount, 1);
    EXPECT_EQ(data.lastPtr, ptr);
    EXPECT_EQ(data.lastSize, 128u);
    
    core::DeallocateBytes(allocator, ptr, 128);
    
    core::ClearAllocationHooks();
}

TEST(AllocationHooks, HooksReceiveDeallocateEvents) {
    core::ClearAllocationHooks();
    
    HookTestData data;
    core::AddAllocationHook(TestHookCallback, &data);
    
    core::MallocAllocator allocator;
    
    void* ptr = core::AllocateBytes(allocator, 256);
    ASSERT_NE(ptr, nullptr);
    
    data = {}; // Reset counters
    
    core::DeallocateBytes(allocator, ptr, 256);
    
    EXPECT_EQ(data.deallocateBeginCount, 1);
    EXPECT_EQ(data.deallocateEndCount, 1);
    EXPECT_EQ(data.lastPtr, ptr);
    EXPECT_EQ(data.lastSize, 256u);
    
    core::ClearAllocationHooks();
}

TEST(AllocationHooks, MultipleHooks) {
    core::ClearAllocationHooks();
    
    HookTestData data1;
    HookTestData data2;
    
    // Register two different callbacks with different user data
    bool registered1 = core::AddAllocationHook(TestHookCallback, &data1);
    ASSERT_TRUE(registered1);
    
    bool registered2 = core::AddAllocationHook(SecondHookCallback, &data2);
    ASSERT_TRUE(registered2);
    
    core::MallocAllocator allocator;
    void* ptr = core::AllocateBytes(allocator, 64);
    ASSERT_NE(ptr, nullptr);
    
    // Both hooks should have been called
    EXPECT_EQ(data1.allocateBeginCount, 1);
    EXPECT_EQ(data1.allocateEndCount, 1);
    EXPECT_EQ(data1.lastPtr, ptr);
    
    EXPECT_EQ(data2.allocateBeginCount, 1);
    EXPECT_EQ(data2.allocateEndCount, 1);
    EXPECT_EQ(data2.lastPtr, ptr);
    
    core::DeallocateBytes(allocator, ptr, 64);
    
    EXPECT_EQ(data1.deallocateBeginCount, 1);
    EXPECT_EQ(data1.deallocateEndCount, 1);
    
    EXPECT_EQ(data2.deallocateBeginCount, 1);
    EXPECT_EQ(data2.deallocateEndCount, 1);
    
    core::ClearAllocationHooks();
}

TEST(AllocationHooks, HooksWithTypedAllocation) {
    core::ClearAllocationHooks();
    
    HookTestData data;
    core::AddAllocationHook(TestHookCallback, &data);
    
    core::MallocAllocator allocator;
    
    int* ptr = core::AllocateObject<int>(allocator);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_EQ(data.allocateBeginCount, 1);
    EXPECT_EQ(data.allocateEndCount, 1);
    EXPECT_EQ(data.lastSize, sizeof(int));
    
    core::DeallocateObject<int>(allocator, ptr);
    
    EXPECT_EQ(data.deallocateBeginCount, 1);
    EXPECT_EQ(data.deallocateEndCount, 1);
    
    core::ClearAllocationHooks();
}

} // namespace

