#include "core/memory/memory_traits.hpp"
#include <gtest/gtest.h>

namespace {

struct TrivialPod {
    int a;
    float b;
};

struct NonTrivialDtor {
    ~NonTrivialDtor() {}
    int x;
};

TEST(MemoryTraits_Alignment, AlignUpDownAndIsAligned) {
    using core::AlignUp;
    using core::AlignDown;
    using core::IsAlignedValue;

    EXPECT_EQ(AlignUp(0u, 8u), 0u);
    EXPECT_EQ(AlignUp(1u, 8u), 8u);
    EXPECT_EQ(AlignUp(8u, 8u), 8u);
    EXPECT_EQ(AlignUp(9u, 8u), 16u);

    EXPECT_EQ(AlignDown(0u, 8u), 0u);
    EXPECT_EQ(AlignDown(1u, 8u), 0u);
    EXPECT_EQ(AlignDown(8u, 8u), 8u);
    EXPECT_EQ(AlignDown(15u, 8u), 8u);
    EXPECT_EQ(AlignDown(16u, 8u), 16u);

    EXPECT_TRUE(IsAlignedValue(static_cast<core::usize>(0), 16u));
    EXPECT_TRUE(IsAlignedValue(static_cast<core::usize>(16), 16u));
    EXPECT_FALSE(IsAlignedValue(static_cast<core::usize>(24), 16u));
}

TEST(MemoryTraits_Alignment, PointerAlignment) {
    alignas(64) unsigned char buf[256]{};

    unsigned char* p0 = &buf[0];
    unsigned char* p8 = &buf[8];
    unsigned char* p64 = &buf[64];

    EXPECT_TRUE(core::IsAlignedPtr(p0, 64u));
    EXPECT_FALSE(core::IsAlignedPtr(p8, 64u));
    EXPECT_TRUE(core::IsAlignedPtr(p64, 64u));
}

TEST(MemoryTraits_PointerMath, AddSubBytesRoundTrip) {
    alignas(16) unsigned char buf[128]{};

    unsigned char* base = &buf[0];
    unsigned char* p = core::AddBytes(base, 37u);
    EXPECT_EQ(p, base + 37);

    unsigned char* back = core::SubBytes(p, 37u);
    EXPECT_EQ(back, base);

    const unsigned char* cp = core::AddBytes(static_cast<const unsigned char*>(base), 12u);
    EXPECT_EQ(cp, base + 12);

    const unsigned char* cback = core::SubBytes(cp, 12u);
    EXPECT_EQ(cback, base);
}

TEST(MemoryTraits_PointerMath, PtrDiffBytes) {
    alignas(16) unsigned char buf[128]{};

    unsigned char* base = &buf[0];
    unsigned char* p = &buf[50];

    core::usize diff = core::PtrDiffBytes(p, base);
    EXPECT_EQ(diff, 50u);
}

TEST(MemoryTraits_Size, BytesForAndSafeMulNoOverflow) {
    EXPECT_EQ(core::BytesFor<int>(0u), 0u);
    EXPECT_EQ(core::BytesFor<int>(1u), sizeof(int));
    EXPECT_EQ(core::BytesFor<int>(10u), sizeof(int) * 10u);

    EXPECT_EQ(core::SafeMulSize(0u, 999u), 0u);
    EXPECT_EQ(core::SafeMulSize(7u, 6u), 42u);
}

TEST(MemoryTraits_Size, StorageSizeFor) {
    EXPECT_EQ(core::StorageSizeFor<core::u64>(5u), sizeof(core::u64) * 5u);
    EXPECT_EQ(core::StorageSizeFor<core::u32>(10u), sizeof(core::u32) * 10u);
}

#if CORE_MEMORY_DEBUG && GTEST_HAS_DEATH_TEST
TEST(MemoryTraits_Size, SafeMulOverflow_Death) {
    const core::usize maxv = core::UsizeMax();
    EXPECT_DEATH({ (void)core::SafeMulSize(maxv, 2u); }, ".*");
}
#endif

TEST(MemoryTraits_Traits, TrivialityAndPolicies) {
    EXPECT_TRUE(core::is_trivially_copyable_v<int>());
    EXPECT_TRUE(core::is_trivially_copyable_v<TrivialPod>());

#if CORE_DETAIL_HAS_INTRINSIC_TRAITS
    EXPECT_FALSE(core::is_trivially_copyable_v<NonTrivialDtor>());
#endif

    EXPECT_EQ(core::is_trivially_relocatable_v<TrivialPod>(),
              core::is_trivially_copyable_v<TrivialPod>());

    EXPECT_EQ(core::is_bitwise_serializable_v<TrivialPod>(),
              core::is_trivially_copyable_v<TrivialPod>());

    EXPECT_EQ(core::max_align_for_v<int>(), static_cast<core::usize>(alignof(int)));
    EXPECT_EQ(core::max_align_for_v<TrivialPod>(), static_cast<core::usize>(alignof(TrivialPod)));
}

TEST(MemoryTraits_Config, DefaultAlignmentAndCacheLineSize) {
    EXPECT_GT(core::DefaultAlignment(), 0u);
    EXPECT_TRUE(core::IsPowerOfTwo(core::DefaultAlignment()));

    EXPECT_GT(core::CacheLineSize(), 0u);
    EXPECT_TRUE(core::IsPowerOfTwo(core::CacheLineSize()));
}

TEST(MemoryTraits_Alignment, PaddingFor) {
    EXPECT_EQ(core::PaddingFor(0u, 16u), 0u);
    EXPECT_EQ(core::PaddingFor(1u, 16u), 15u);
    EXPECT_EQ(core::PaddingFor(16u, 16u), 0u);
    EXPECT_EQ(core::PaddingFor(17u, 16u), 15u);
}

TEST(MemoryTraits_Alignment, AlignPtrUp) {
    alignas(64) unsigned char buf[256]{};

    unsigned char* p7 = &buf[7];
    unsigned char* aligned = core::AlignPtrUp(p7, 16u);
    
    EXPECT_TRUE(core::IsAlignedPtr(aligned, 16u));
    EXPECT_GE(aligned, p7);
    EXPECT_LT(core::PtrDiffBytes(aligned, p7), 16u);
}

} // namespace

