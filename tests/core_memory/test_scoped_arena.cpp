#include "core/memory/scoped_arena.hpp"
#include "core/memory/arena.hpp"
#include "core/memory/core_memory.hpp"
#include "core/memory/memory_ops.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

namespace {

using SimpleTestArena = core::test::SimpleTestArena;

// =============================================================================
// ScopedArena Basic Tests
// =============================================================================

TEST(ScopedArena, Construction) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena scoped(arena);
    
    EXPECT_TRUE(scoped.IsValid());
    EXPECT_EQ(&scoped.GetArena(), &arena);
}

TEST(ScopedArena, AutomaticRewindOnScopeExit) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size initial_used = arena.Used();
    
    {
        core::ScopedArena scoped(arena);
        void* ptr = scoped.Allocate(256);
        ASSERT_NE(ptr, nullptr);
        EXPECT_GT(arena.Used(), initial_used);
    }
    
    // After scope exit, arena should rewind
    EXPECT_EQ(arena.Used(), initial_used);
}

TEST(ScopedArena, AllocateForwardsToArena) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ScopedArena scoped(arena);
    
    void* ptr = scoped.Allocate(128, 16);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 16));
    EXPECT_TRUE(arena.Owns(ptr));
}

TEST(ScopedArena, MultipleAllocations) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size initial = arena.Used();
    
    {
        core::ScopedArena scoped(arena);
        
        void* p1 = scoped.Allocate(100);
        void* p2 = scoped.Allocate(200);
        void* p3 = scoped.Allocate(300);
        
        ASSERT_NE(p1, nullptr);
        ASSERT_NE(p2, nullptr);
        ASSERT_NE(p3, nullptr);
        
        EXPECT_NE(p1, p2);
        EXPECT_NE(p2, p3);
        EXPECT_NE(p1, p3);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

// =============================================================================
// Nested Scopes Tests
// =============================================================================

TEST(ScopedArena, NestedScopes_TwoLevels) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size level0 = arena.Used();
    
    {
        core::ScopedArena outer(arena);
        void* p1 = outer.Allocate(100);
        ASSERT_NE(p1, nullptr);
        core::memory_size level1 = arena.Used();
        EXPECT_GT(level1, level0);
        
        {
            core::ScopedArena inner(arena);
            void* p2 = inner.Allocate(200);
            ASSERT_NE(p2, nullptr);
            core::memory_size level2 = arena.Used();
            EXPECT_GT(level2, level1);
        }
        
        // After inner scope, should rewind to level1
        EXPECT_EQ(arena.Used(), level1);
    }
    
    // After outer scope, should rewind to level0
    EXPECT_EQ(arena.Used(), level0);
}

TEST(ScopedArena, NestedScopes_ThreeLevels) {
    alignas(16) core::u8 buffer[2048];
    SimpleTestArena arena(buffer, 2048);
    
    core::memory_size l0 = arena.Used();
    
    {
        core::ScopedArena s1(arena);
        ASSERT_NE(s1.Allocate(100), nullptr);
        core::memory_size l1 = arena.Used();
        
        {
            core::ScopedArena s2(arena);
            ASSERT_NE(s2.Allocate(200), nullptr);
            core::memory_size l2 = arena.Used();
            
            {
                core::ScopedArena s3(arena);
                ASSERT_NE(s3.Allocate(300), nullptr);
                core::memory_size l3 = arena.Used();
                
                EXPECT_GT(l3, l2);
            }
            
            EXPECT_EQ(arena.Used(), l2);
        }
        
        EXPECT_EQ(arena.Used(), l1);
    }
    
    EXPECT_EQ(arena.Used(), l0);
}

TEST(ScopedArena, NestedScopes_MemoryReuse) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    void* first_allocation = nullptr;
    
    {
        core::ScopedArena outer(arena);
        
        {
            core::ScopedArena inner(arena);
            first_allocation = inner.Allocate(128);
            ASSERT_NE(first_allocation, nullptr);
        }
        
        // After inner rewind, allocate again
        void* second_allocation = outer.Allocate(128);
        ASSERT_NE(second_allocation, nullptr);
        
        // Should reuse the same memory
        EXPECT_EQ(first_allocation, second_allocation);
    }
}

// =============================================================================
// Move Semantics Tests
// =============================================================================

TEST(ScopedArena, MoveConstructor_TransfersOwnership) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena s1(arena);
    EXPECT_TRUE(s1.IsValid());
    
    core::ScopedArena s2(std::move(s1));
    
    EXPECT_FALSE(s1.IsValid());
    EXPECT_TRUE(s2.IsValid());
    EXPECT_EQ(&s2.GetArena(), &arena);
}

TEST(ScopedArena, MoveConstructor_NoRewindOnSource) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size initial = arena.Used();
    
    {
        core::ScopedArena s1(arena);
        ASSERT_NE(s1.Allocate(256), nullptr);
        EXPECT_GT(arena.Used(), initial);
        
        core::ScopedArena s2(std::move(s1));
        
        // s1 is moved-from (invalid), s2 owns the marker
        EXPECT_FALSE(s1.IsValid());
        EXPECT_TRUE(s2.IsValid());
        EXPECT_GT(arena.Used(), initial);
    }
    
    // s2 destructs and rewinds
    EXPECT_EQ(arena.Used(), initial);
}

TEST(ScopedArena, MoveAssignment_TransfersOwnership) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena s1(arena);
    core::ScopedArena s2(arena);
    
    EXPECT_TRUE(s1.IsValid());
    EXPECT_TRUE(s2.IsValid());
    
    s2 = std::move(s1);
    
    EXPECT_FALSE(s1.IsValid());
    EXPECT_TRUE(s2.IsValid());
    EXPECT_EQ(&s2.GetArena(), &arena);
}

TEST(ScopedArena, MoveAssignment_RewindsTargetBeforeTransfer) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size m0 = arena.Used();
    
    {
        core::ScopedArena s1(arena);
        ASSERT_NE(s1.Allocate(100), nullptr);
        core::memory_size m1 = arena.Used();
        
        core::ScopedArena s2(arena);
        ASSERT_NE(s2.Allocate(200), nullptr);
        core::memory_size m2 = arena.Used();
        
        EXPECT_GT(m2, m1);
        
        // Move s1 to s2: should rewind to m1 (s2's old marker), then take s1's marker (m0)
        s2 = std::move(s1);
        
        // After move-assignment: s2's old marker was rewound, now arena is at m1
        EXPECT_EQ(arena.Used(), m1);
        
        // s1 is invalid, s2 now holds marker m0
        EXPECT_FALSE(s1.IsValid());
        EXPECT_TRUE(s2.IsValid());
    }
    
    // After scope exit: s2 rewinds to its transferred marker (m0)
    EXPECT_EQ(arena.Used(), m0);
}

TEST(ScopedArena, MoveAssignment_SelfAssignment) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena scoped(arena);
    ASSERT_NE(scoped.Allocate(100), nullptr);
    
    core::memory_size before = arena.Used();
    
    // Self-assignment should be no-op: no rewind, object remains valid
    scoped = std::move(scoped);
    
    EXPECT_TRUE(scoped.IsValid());
    EXPECT_EQ(arena.Used(), before); // Arena state unchanged
}

TEST(ScopedArena, MovedFromObject_DestructorIsNoOp) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size initial = arena.Used();
    
    {
        core::ScopedArena s1(arena);
        ASSERT_NE(s1.Allocate(256), nullptr);
        
        core::ScopedArena s2(std::move(s1));
        // At scope exit: s2 destructs first (rewinds), then s1 (no-op, moved-from)
    }
    
    // Only s2 should have rewound, s1 destructor was no-op
    EXPECT_EQ(arena.Used(), initial);
}

// =============================================================================
// IsValid() Tests
// =============================================================================

TEST(ScopedArena, IsValid_TrueAfterConstruction) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena scoped(arena);
    
    EXPECT_TRUE(scoped.IsValid());
}

TEST(ScopedArena, IsValid_FalseAfterMoveConstruction) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena s1(arena);
    core::ScopedArena s2(std::move(s1));
    
    EXPECT_FALSE(s1.IsValid());
    EXPECT_TRUE(s2.IsValid());
}

TEST(ScopedArena, IsValid_FalseAfterMoveAssignment) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena s1(arena);
    core::ScopedArena s2(arena);
    
    s2 = std::move(s1);
    
    EXPECT_FALSE(s1.IsValid());
    EXPECT_TRUE(s2.IsValid());
}

// =============================================================================
// GetArena() Tests
// =============================================================================

TEST(ScopedArena, GetArena_ReturnsCorrectArena) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena scoped(arena);
    
    EXPECT_EQ(&scoped.GetArena(), &arena);
}

TEST(ScopedArena, GetArena_AllowsDirectArenaAccess) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::ScopedArena scoped(arena);
    
    core::IArena& arena_ref = scoped.GetArena();
    void* ptr = arena_ref.Allocate(64);
    
    EXPECT_NE(ptr, nullptr);
    EXPECT_TRUE(arena.Owns(ptr));
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(ScopedArena, MixedWithDirectArenaUsage) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    // Direct allocation
    arena.Allocate(100);
    core::memory_size after_direct = arena.Used();
    
    {
        core::ScopedArena scoped(arena);
        ASSERT_NE(scoped.Allocate(200), nullptr);
        EXPECT_GT(arena.Used(), after_direct);
    }
    
    // Rewinds to state at ScopedArena construction
    EXPECT_EQ(arena.Used(), after_direct);
    
    // Direct allocation still possible
    void* ptr = arena.Allocate(50);
    EXPECT_NE(ptr, nullptr);
}

TEST(ScopedArena, EmptyScopedArena_StillRewinds) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    arena.Allocate(100);
    core::memory_size before = arena.Used();
    
    {
        core::ScopedArena scoped(arena);
        // No allocations through scoped
    }
    
    // Should still capture and rewind to marker
    EXPECT_EQ(arena.Used(), before);
}

TEST(ScopedArena, OutOfMemory_DoesNotCrash) {
    alignas(16) core::u8 buffer[128];
    SimpleTestArena arena(buffer, 128);
    
    core::memory_size initial = arena.Used();
    
    {
        core::ScopedArena scoped(arena);
        
        void* p1 = scoped.Allocate(64);
        ASSERT_NE(p1, nullptr);
        
        void* p2 = scoped.Allocate(128); // Should fail
        EXPECT_EQ(p2, nullptr);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

// =============================================================================
// Alignment Tests
// =============================================================================

TEST(ScopedArena, Allocate_RespectsAlignment) {
    alignas(64) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ScopedArena scoped(arena);
    
    void* p1 = scoped.Allocate(10, 4);
    ASSERT_NE(p1, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p1, 4));
    
    void* p2 = scoped.Allocate(20, 16);
    ASSERT_NE(p2, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p2, 16));
    
    void* p3 = scoped.Allocate(30, 64);
    ASSERT_NE(p3, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p3, 64));
}

TEST(ScopedArena, Allocate_DefaultAlignment) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ScopedArena scoped(arena);
    
    void* ptr = scoped.Allocate(100);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, CORE_DEFAULT_ALIGNMENT));
}

// =============================================================================
// Exception Safety (if exceptions are enabled)
// =============================================================================

#if !defined(CORE_NO_EXCEPTIONS)
TEST(ScopedArena, ExceptionSafety_RewindsOnException) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size initial = arena.Used();
    
    try {
        core::ScopedArena scoped(arena);
        ASSERT_NE(scoped.Allocate(256), nullptr);
        EXPECT_GT(arena.Used(), initial);
        
        throw std::runtime_error("test exception");
    } catch (...) {
        // Exception caught
    }
    
    // Arena should still rewind despite exception
    EXPECT_EQ(arena.Used(), initial);
}
#endif

// =============================================================================
// Use Case Examples
// =============================================================================

TEST(ScopedArena, UseCase_ReturnFromFunction) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    auto MakeScoped = [&arena]() -> core::ScopedArena {
        return core::ScopedArena(arena);
    };
    
    core::memory_size initial = arena.Used();
    
    {
        core::ScopedArena scoped = MakeScoped();
        ASSERT_NE(scoped.Allocate(128), nullptr);
        EXPECT_GT(arena.Used(), initial);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

TEST(ScopedArena, UseCase_ConditionalScope) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    
    core::memory_size initial = arena.Used();
    
    bool use_scoped = true;
    
    if (use_scoped) {
        core::ScopedArena scoped(arena);
        ASSERT_NE(scoped.Allocate(256), nullptr);
        EXPECT_GT(arena.Used(), initial);
    }
    
    EXPECT_EQ(arena.Used(), initial);
}

} // anonymous namespace

