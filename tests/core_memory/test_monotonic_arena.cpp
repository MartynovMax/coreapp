#include "core/memory/monotonic_arena.hpp"
#include "core/memory/scoped_arena.hpp"
#include "core/memory/system_allocator.hpp"
#include <gtest/gtest.h>
#include <type_traits>

namespace {

using namespace core;

// =============================================================================
// Alias verification tests
// =============================================================================

TEST(MonotonicArena, IsAliasForBumpArena) {
    static_assert(std::is_same_v<MonotonicArena, BumpArena>, 
                  "MonotonicArena must be an alias for BumpArena");
}

TEST(MonotonicArena, Construction_Owning) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    EXPECT_EQ(arena.Capacity(), 1024);
    EXPECT_EQ(arena.Used(), 0);
    EXPECT_EQ(arena.Remaining(), 1024);
}

TEST(MonotonicArena, Construction_NonOwning) {
    alignas(16) u8 buffer[1024];
    MonotonicArena arena(buffer, sizeof(buffer));
    
    EXPECT_EQ(arena.Capacity(), 1024);
    EXPECT_EQ(arena.Used(), 0);
    EXPECT_EQ(arena.Remaining(), 1024);
}

TEST(MonotonicArena, Construction_WithName) {
    MonotonicArena arena(1024, SystemAllocator::Instance(), "monotonic_test");
    
    const char* name = arena.Name();
    ASSERT_NE(name, nullptr);
    EXPECT_STREQ(name, "monotonic_test");
}

// =============================================================================
// Monotonic growth semantics
// =============================================================================

TEST(MonotonicArena, MonotonicGrowth) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    memory_size used0 = arena.Used();
    EXPECT_EQ(used0, 0);
    
    arena.Allocate(64);
    memory_size used1 = arena.Used();
    EXPECT_GT(used1, used0);
    
    arena.Allocate(128);
    memory_size used2 = arena.Used();
    EXPECT_GT(used2, used1);
}

TEST(MonotonicArena, NoIndividualFrees) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    void* ptr1 = arena.Allocate(64);
    ASSERT_NE(ptr1, nullptr);
    memory_size usedAfter1 = arena.Used();
    
    void* ptr2 = arena.Allocate(64);
    ASSERT_NE(ptr2, nullptr);
    memory_size usedAfter2 = arena.Used();
    
    EXPECT_GT(usedAfter2, usedAfter1);
}

TEST(MonotonicArena, BulkReclamation_Reset) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    arena.Allocate(256);
    arena.Allocate(128);
    
    EXPECT_GT(arena.Used(), 0);
    
    arena.Reset();
    
    EXPECT_EQ(arena.Used(), 0);
    EXPECT_EQ(arena.Remaining(), arena.Capacity());
}

TEST(MonotonicArena, BulkReclamation_Rewind) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    void* ptr = arena.Allocate(100);
    ASSERT_NE(ptr, nullptr);
    
    ArenaMarker mark = arena.GetMarker();
    memory_size usedAtMark = arena.Used();
    
    arena.Allocate(200);
    arena.Allocate(150);
    
    EXPECT_GT(arena.Used(), usedAtMark);
    
    arena.RewindTo(mark);
    
    EXPECT_EQ(arena.Used(), usedAtMark);
}

// =============================================================================
// Sanity checks
// =============================================================================

TEST(MonotonicArena, ManySmallAllocations) {
    MonotonicArena arena(1024 * 1024, SystemAllocator::Instance());
    
    constexpr int iterations = 10000;
    
    for (int i = 0; i < iterations; ++i) {
        void* ptr = arena.Allocate(32);
        ASSERT_NE(ptr, nullptr);
    }
    
    EXPECT_GT(arena.Used(), 0);
}

TEST(MonotonicArena, AllocationsAreSequential) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    void* ptr1 = arena.Allocate(64);
    void* ptr2 = arena.Allocate(64);
    void* ptr3 = arena.Allocate(64);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    
    uintptr_t addr1 = reinterpret_cast<uintptr_t>(ptr1);
    uintptr_t addr2 = reinterpret_cast<uintptr_t>(ptr2);
    uintptr_t addr3 = reinterpret_cast<uintptr_t>(ptr3);
    
    EXPECT_LT(addr1, addr2);
    EXPECT_LT(addr2, addr3);
}

// =============================================================================
// Integration with other Core components
// =============================================================================

TEST(MonotonicArena, WorksWithScopedArena) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    memory_size initial = arena.Used();
    
    {
        ScopedArena scope(arena);
        void* ptr = scope.Allocate(256);
        ASSERT_NE(ptr, nullptr);
        EXPECT_GT(arena.Used(), initial);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

TEST(MonotonicArena, WorksWithNestedScopes) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    arena.Reset();
    
    memory_size initial = arena.Used();
    
    {
        ScopedArena outer(arena);
        outer.Allocate(64);
        memory_size afterOuter = arena.Used();
        
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

TEST(MonotonicArena, ConformsToIArenaInterface) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    IArena& iarena = arena;
    
    void* ptr = iarena.Allocate(256);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_GT(iarena.Used(), 0);
    EXPECT_EQ(iarena.Capacity(), 1024);
    
    ArenaMarker mark = iarena.GetMarker();
    iarena.Allocate(128);
    iarena.RewindTo(mark);
    
    iarena.Reset();
    EXPECT_EQ(iarena.Used(), 0);
}

// =============================================================================
// Use case examples
// =============================================================================

TEST(MonotonicArena, UseCase_PerFrameAllocation) {
    MonotonicArena frameArena(64 * 1024, SystemAllocator::Instance(), "frame_arena");
    
    for (int frame = 0; frame < 10; ++frame) {
        void* particleData = frameArena.Allocate(1024);
        void* animationData = frameArena.Allocate(2048);
        
        ASSERT_NE(particleData, nullptr);
        ASSERT_NE(animationData, nullptr);
        
        frameArena.Reset();
        EXPECT_EQ(frameArena.Used(), 0);
    }
}

TEST(MonotonicArena, UseCase_ParserWithRewind) {
    MonotonicArena parserArena(4096, SystemAllocator::Instance(), "parser_arena");
    
    auto TryParse = [&](bool shouldSucceed) -> bool {
        ArenaMarker mark = parserArena.GetMarker();
        
        void* tokenBuffer = parserArena.Allocate(512);
        void* astNodes = parserArena.Allocate(1024);
        
        EXPECT_NE(tokenBuffer, nullptr);
        EXPECT_NE(astNodes, nullptr);
        
        if (!shouldSucceed) {
            parserArena.RewindTo(mark);
            return false;
        }
        
        return true;
    };
    
    memory_size initial = parserArena.Used();
    
    EXPECT_FALSE(TryParse(false));
    EXPECT_EQ(parserArena.Used(), initial);
    
    EXPECT_TRUE(TryParse(true));
    EXPECT_GT(parserArena.Used(), initial);
}

TEST(MonotonicArena, UseCase_ScopedTemporaryBuffer) {
    MonotonicArena arena(1024, SystemAllocator::Instance());
    
    auto ProcessData = [&](int size) {
        ScopedArena scope(arena);
        
        void* tempBuffer = scope.Allocate(size);
        EXPECT_NE(tempBuffer, nullptr);
    };
    
    memory_size initial = arena.Used();
    
    ProcessData(256);
    EXPECT_EQ(arena.Used(), initial);
    ProcessData(512);
    EXPECT_EQ(arena.Used(), initial);
}

} // namespace

