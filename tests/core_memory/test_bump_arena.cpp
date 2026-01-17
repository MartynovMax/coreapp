#include "core/memory/bump_arena.hpp"
#include "core/memory/scoped_arena.hpp"
#include "core/memory/system_allocator.hpp"
#include <gtest/gtest.h>

namespace {

using namespace core;

TEST(BumpArena, Construction_Owning) {
    BumpArena arena(1024, SystemAllocator::Instance());
    
    EXPECT_EQ(arena.Capacity(), 1024);
    EXPECT_EQ(arena.Used(), 0);
    EXPECT_EQ(arena.Remaining(), 1024);
}

TEST(BumpArena, Construction_NonOwning) {
    alignas(16) u8 buffer[1024];
    BumpArena arena(buffer, 1024);
    
    EXPECT_EQ(arena.Capacity(), 1024);
    EXPECT_EQ(arena.Used(), 0);
    EXPECT_EQ(arena.Remaining(), 1024);
}

TEST(BumpArena, BasicAllocation) {
    BumpArena arena(1024, SystemAllocator::Instance());
    
    void* ptr = arena.Allocate(128);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_GE(arena.Used(), 128);
    EXPECT_LE(arena.Remaining(), 1024 - 128);
}

TEST(BumpArena, MultipleAllocations) {
    BumpArena arena(1024, SystemAllocator::Instance());
    
    void* p1 = arena.Allocate(64);
    memory_size used1 = arena.Used();
    
    void* p2 = arena.Allocate(128);
    memory_size used2 = arena.Used();
    
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_GT(used2, used1);
    EXPECT_NE(p1, p2);
}

TEST(BumpArena, Reset) {
    BumpArena arena(1024, SystemAllocator::Instance());
    
    arena.Allocate(256);
    EXPECT_GT(arena.Used(), 0);
    
    arena.Reset();
    
    EXPECT_EQ(arena.Used(), 0);
    EXPECT_EQ(arena.Remaining(), arena.Capacity());
}

TEST(BumpArena, GetMarkerAndRewindTo) {
    BumpArena arena(1024, SystemAllocator::Instance());
    
    ArenaMarker mark1 = arena.GetMarker();
    memory_size used1 = arena.Used();
    
    void* p1 = arena.Allocate(128);
    ASSERT_NE(p1, nullptr);
    
    ArenaMarker mark2 = arena.GetMarker();
    memory_size used2 = arena.Used();
    EXPECT_GT(used2, used1);
    
    void* p2 = arena.Allocate(256);
    ASSERT_NE(p2, nullptr);
    
    memory_size usedFinal = arena.Used();
    EXPECT_GT(usedFinal, used2);
    
    arena.RewindTo(mark2);
    EXPECT_EQ(arena.Used(), used2);
    
    arena.RewindTo(mark1);
    EXPECT_EQ(arena.Used(), used1);
}

TEST(BumpArena, NestedScopedArena) {
    BumpArena arena(1024, SystemAllocator::Instance());
    
    memory_size initial = arena.Used();
    
    {
        ScopedArena outer(arena);
        outer.Allocate(64);
        memory_size afterOuter = arena.Used();
        EXPECT_GT(afterOuter, initial);
        
        {
            ScopedArena inner(arena);
            inner.Allocate(128);
            memory_size afterInner = arena.Used();
            EXPECT_GT(afterInner, afterOuter);
        }
        
        EXPECT_EQ(arena.Used(), afterOuter);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

TEST(BumpArena, Naming_WithName) {
    BumpArena arena(1024, SystemAllocator::Instance(), "test_arena");
    
    const char* name = arena.Name();
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "test_arena");
}

TEST(BumpArena, Naming_DefaultName) {
    BumpArena arena(1024, SystemAllocator::Instance());
    
    const char* name = arena.Name();
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "bump_arena");
}

TEST(BumpArena, Owns) {
    alignas(16) u8 buffer[1024];
    BumpArena arena(buffer, 1024);
    
    void* ptr = arena.Allocate(64);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(arena.Owns(ptr));
    EXPECT_FALSE(arena.Owns(nullptr));
    
    u8 external[64];
    EXPECT_FALSE(arena.Owns(external));
}

TEST(BumpArena, OutOfMemory) {
    BumpArena arena(128, SystemAllocator::Instance());
    
    void* p1 = arena.Allocate(64);
    ASSERT_NE(p1, nullptr);
    
    void* p2 = arena.Allocate(128);
    EXPECT_EQ(p2, nullptr);
}

} // namespace

