#include "core/memory/arena.hpp"
#include "core/memory/core_allocator.hpp"
#include "core/memory/core_memory.hpp"
#include "core/memory/memory_ops.hpp"
#include "test_helpers.hpp"
#include <gtest/gtest.h>

namespace {

using SimpleTestArena = core::test::SimpleTestArena;

// =============================================================================
// ArenaMarker Tests
// =============================================================================

TEST(ArenaMarker, DefaultConstruction) {
    core::ArenaMarker marker;
    EXPECT_EQ(marker.internal_state, nullptr);
}

TEST(ArenaMarker, Comparison) {
    core::ArenaMarker m1;
    core::ArenaMarker m2;
    
    m1.internal_state = reinterpret_cast<void*>(0x1000);
    m2.internal_state = reinterpret_cast<void*>(0x1000);
    
    EXPECT_EQ(m1, m2);
    
    m2.internal_state = reinterpret_cast<void*>(0x2000);
    EXPECT_NE(m1, m2);
}

TEST(ArenaMarker, SizeConstraint) {
    EXPECT_LE(sizeof(core::ArenaMarker), 16);
}

// =============================================================================
// IArena Interface Tests (Generic, no knowledge of concrete type)
// =============================================================================

TEST(IArena, BasicAllocation) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    void* ptr = iface.Allocate(64, 16);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 16));
    EXPECT_GE(iface.Used(), 64);
    EXPECT_LE(iface.Used(), 64 + 15);
}

TEST(IArena, AllocationZeroSize) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    void* ptr = iface.Allocate(0, 16);
    
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(iface.Used(), 0);
}

TEST(IArena, AllocationOutOfMemory) {
    alignas(16) core::u8 buffer[128];
    SimpleTestArena arena(buffer, 128);
    core::IArena& iface = arena;
    
    void* ptr = iface.Allocate(256, 16);
    
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(iface.Used(), 0);
}

TEST(IArena, MultipleAllocations) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    void* p1 = iface.Allocate(100, 8);
    ASSERT_NE(p1, nullptr);
    core::memory_size used1 = iface.Used();
    
    void* p2 = iface.Allocate(200, 8);
    ASSERT_NE(p2, nullptr);
    core::memory_size used2 = iface.Used();
    
    EXPECT_GT(used2, used1);
    EXPECT_NE(p1, p2);
}

// =============================================================================
// Reset Tests
// =============================================================================

TEST(IArena, Reset_EmptiesArena) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    iface.Allocate(100, 8);
    iface.Allocate(200, 8);
    EXPECT_GT(iface.Used(), 0);
    
    iface.Reset();
    
    EXPECT_EQ(iface.Used(), 0);
    EXPECT_EQ(iface.Remaining(), iface.Capacity());
}

TEST(IArena, Reset_AllowsReuse) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    void* p1 = iface.Allocate(512, 16);
    ASSERT_NE(p1, nullptr);
    
    iface.Reset();
    
    void* p2 = iface.Allocate(512, 16);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p1, p2); // Should reuse same memory
}

TEST(IArena, Reset_MultipleResets) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    iface.Reset();
    EXPECT_EQ(iface.Used(), 0);
    
    iface.Reset();
    EXPECT_EQ(iface.Used(), 0);
    
    iface.Reset();
    EXPECT_EQ(iface.Used(), 0);
}

// =============================================================================
// Marker Tests
// =============================================================================

TEST(IArena, GetMarker_InitialState) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    iface.Allocate(123, 8);
    core::memory_size stateBefore = iface.Used();
    
    core::ArenaMarker m1 = iface.GetMarker();
    core::ArenaMarker m2 = iface.GetMarker();
    
    iface.Allocate(77, 8);
    EXPECT_GT(iface.Used(), stateBefore);
    
    iface.RewindTo(m1);
    core::memory_size afterM1 = iface.Used();
    
    iface.Allocate(77, 8);
    iface.RewindTo(m2);
    core::memory_size afterM2 = iface.Used();
    
    EXPECT_EQ(afterM1, stateBefore);
    EXPECT_EQ(afterM2, stateBefore);
}

TEST(IArena, GetMarker_AfterAllocation) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    core::ArenaMarker m1 = iface.GetMarker();
    iface.Allocate(100, 8);
    core::ArenaMarker m2 = iface.GetMarker();
    
    EXPECT_NE(m1, m2);
}

TEST(IArena, RewindTo_SingleAllocation) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    core::ArenaMarker m = iface.GetMarker();
    iface.Allocate(100, 8);
    EXPECT_GT(iface.Used(), 0);
    
    iface.RewindTo(m);
    
    EXPECT_EQ(iface.Used(), 0);
}

TEST(IArena, RewindTo_MultipleAllocations) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    core::ArenaMarker m1 = iface.GetMarker();
    iface.Allocate(100, 8);
    
    core::ArenaMarker m2 = iface.GetMarker();
    core::memory_size used_at_m2 = iface.Used();
    
    iface.Allocate(200, 8);
    iface.Allocate(300, 8);
    
    iface.RewindTo(m2);
    EXPECT_EQ(iface.Used(), used_at_m2);
    
    iface.RewindTo(m1);
    EXPECT_EQ(iface.Used(), 0);
}

TEST(IArena, RewindTo_ToCurrentState) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    iface.Allocate(100, 8);
    core::ArenaMarker m = iface.GetMarker();
    core::memory_size used = iface.Used();
    
    iface.RewindTo(m);
    
    EXPECT_EQ(iface.Used(), used);
}

TEST(IArena, RewindTo_AllowsReallocation) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    core::ArenaMarker m = iface.GetMarker();
    void* p1 = iface.Allocate(128, 16);
    
    iface.RewindTo(m);
    
    void* p2 = iface.Allocate(128, 16);
    EXPECT_EQ(p1, p2);
}

// =============================================================================
// Introspection Tests
// =============================================================================

TEST(IArena, Capacity_ReturnsCorrectValue) {
    constexpr core::memory_size kSize = 2048;
    alignas(16) core::u8 buffer[kSize];
    SimpleTestArena arena(buffer, kSize);
    core::IArena& iface = arena;
    
    EXPECT_EQ(iface.Capacity(), kSize);
}

TEST(IArena, Used_TracksAllocations) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    EXPECT_EQ(iface.Used(), 0);
    
    iface.Allocate(100, 8);
    core::memory_size used1 = iface.Used();
    EXPECT_GT(used1, 0);
    EXPECT_LE(used1, 100 + 8); // Account for alignment padding
    
    iface.Allocate(200, 8);
    EXPECT_GT(iface.Used(), used1);
}

TEST(IArena, Remaining_MatchesCapacityMinusUsed) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    EXPECT_EQ(iface.Remaining(), iface.Capacity() - iface.Used());
    
    iface.Allocate(100, 8);
    EXPECT_EQ(iface.Remaining(), iface.Capacity() - iface.Used());
    
    iface.Allocate(200, 8);
    EXPECT_EQ(iface.Remaining(), iface.Capacity() - iface.Used());
}

TEST(IArena, Name_ReturnsValue) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    const char* name = iface.Name();
    EXPECT_NE(name, nullptr);
    EXPECT_STREQ(name, "SimpleTestArena");
}

TEST(IArena, Owns_RecognizesOwnedPointers) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    void* ptr = iface.Allocate(64, 16);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(iface.Owns(ptr));
}

TEST(IArena, Owns_RejectsForeignPointers) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    int external = 42;
    EXPECT_FALSE(iface.Owns(&external));
}

TEST(IArena, Owns_HandlesNullptr) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    EXPECT_FALSE(iface.Owns(nullptr));
}

// =============================================================================
// ArenaAllocatorAdapter Tests
// =============================================================================

TEST(ArenaAllocatorAdapter, Allocate_ForwardsToArena) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ArenaAllocatorAdapter adapter(arena);
    
    core::AllocationRequest req{};
    req.size = 64;
    req.alignment = 16;
    
    void* ptr = adapter.Allocate(req);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_GE(arena.Used(), 64);
    EXPECT_LE(arena.Used(), 64 + (16 - 1));
}

TEST(ArenaAllocatorAdapter, Deallocate_IsNoOp) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ArenaAllocatorAdapter adapter(arena);
    
    void* ptr = core::AllocateBytes(adapter, 128, 16);
    ASSERT_NE(ptr, nullptr);
    core::memory_size used = arena.Used();
    
    core::AllocationInfo info{};
    info.ptr = ptr;
    info.size = 128;
    adapter.Deallocate(info);
    
    // Used should not change
    EXPECT_EQ(arena.Used(), used);
}

TEST(ArenaAllocatorAdapter, Owns_ForwardsToArena) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ArenaAllocatorAdapter adapter(arena);
    
    void* ptr = core::AllocateBytes(adapter, 64, 16);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(adapter.Owns(ptr));
    
    int external = 42;
    EXPECT_FALSE(adapter.Owns(&external));
}

TEST(ArenaAllocatorAdapter, TryGetStats_ReturnsFalse) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ArenaAllocatorAdapter adapter(arena);
    
    core::AllocatorStats stats;
    bool result = adapter.TryGetStats(stats);
    
    EXPECT_FALSE(result);
}

TEST(ArenaAllocatorAdapter, GetArena_ReturnsUnderlyingArena) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ArenaAllocatorAdapter adapter(arena);
    
    core::IArena& retrieved = adapter.GetArena();
    
    EXPECT_EQ(&retrieved, &arena);
}

TEST(ArenaAllocatorAdapter, WorksWithCoreHelpers) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::ArenaAllocatorAdapter adapter(arena);
    
    // Use Core allocation helpers
    void* p1 = core::AllocateBytes(adapter, 100, 8);
    ASSERT_NE(p1, nullptr);
    
    void* p2 = core::AllocateBytes(adapter, 200, 16);
    ASSERT_NE(p2, nullptr);
    
    EXPECT_GT(arena.Used(), 0);
    
    // Deallocate is no-op
    core::DeallocateBytes(adapter, p1, 100, 8);
    core::DeallocateBytes(adapter, p2, 200, 16);
    
    // Used unchanged
    EXPECT_GT(arena.Used(), 0);
    
    // Reset via underlying arena
    arena.Reset();
    EXPECT_EQ(arena.Used(), 0);
}

// =============================================================================
// Advanced Tests - Alignment Padding, Stale Markers, Edge Cases
// =============================================================================

TEST(IArena, AllocationWithPadding_MisalignedStart) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer + 3, 1024 - 3);
    core::IArena& iface = arena;
    
    void* ptr = iface.Allocate(64, 16);
    
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(ptr, 16));
    EXPECT_GT(iface.Used(), 64);
    EXPECT_LE(iface.Used(), 64 + 15);
}

#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST
TEST(IArena, RewindTo_StaleMarkerAfterReset_AssertsInDebug) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    core::ArenaMarker staleMarker = iface.GetMarker();
    iface.Allocate(100, 8);
    
    iface.Reset();
    
    EXPECT_DEATH(iface.RewindTo(staleMarker), "");
}
#endif

TEST(IArena, Owns_BoundaryAddresses) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    EXPECT_TRUE(iface.Owns(buffer));
    EXPECT_TRUE(iface.Owns(buffer + iface.Capacity() - 1));
    EXPECT_FALSE(iface.Owns(buffer + iface.Capacity()));
}

TEST(IArena, Owns_MiddleOfAllocation) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    void* ptr = iface.Allocate(100, 8);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_TRUE(iface.Owns(ptr));
    
    core::u8* middle = static_cast<core::u8*>(ptr) + 50;
    EXPECT_TRUE(iface.Owns(middle));
    
    core::u8* end = static_cast<core::u8*>(ptr) + 99;
    EXPECT_TRUE(iface.Owns(end));
}

TEST(IArena, MultipleAllocations_DifferentAlignments) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    void* p1 = iface.Allocate(10, 4);
    ASSERT_NE(p1, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p1, 4));
    
    void* p2 = iface.Allocate(20, 16);
    ASSERT_NE(p2, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p2, 16));
    
    void* p3 = iface.Allocate(30, 8);
    ASSERT_NE(p3, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p3, 8));
    
    EXPECT_NE(p1, p2);
    EXPECT_NE(p2, p3);
    EXPECT_NE(p1, p3);
}

TEST(IArena, RewindTo_PartialRollback) {
    alignas(16) core::u8 buffer[1024];
    SimpleTestArena arena(buffer, 1024);
    core::IArena& iface = arena;
    
    iface.Allocate(100, 8);
    iface.Allocate(100, 8);
    core::ArenaMarker m2 = iface.GetMarker();
    
    void* p3 = iface.Allocate(100, 8);
    
    iface.RewindTo(m2);
    
    void* p4 = iface.Allocate(100, 8);
    ASSERT_NE(p4, nullptr);
    EXPECT_EQ(p4, p3);
}

TEST(IArena, CapacityRemaining_ExhaustMemory) {
    alignas(16) core::u8 buffer[256];
    SimpleTestArena arena(buffer, 256);
    core::IArena& iface = arena;
    
    core::memory_size capacity = iface.Capacity();
    EXPECT_EQ(capacity, 256);
    
    void* ptr = iface.Allocate(200, 8);
    ASSERT_NE(ptr, nullptr);
    
    core::memory_size remaining = iface.Remaining();
    
    void* fail = iface.Allocate(remaining + 10, 8);
    EXPECT_EQ(fail, nullptr);
}

} // anonymous namespace

