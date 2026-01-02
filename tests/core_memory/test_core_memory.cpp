#include "core/memory/core_allocator.hpp"
#include <gtest/gtest.h>

namespace {

class TestAllocator final : public core::IAllocator {
public:
    void* Allocate(const core::AllocationRequest& request) noexcept override {
        // Trivial fake: return an aligned pointer inside a local buffer.
        // This is ONLY for interface smoke testing; not a real allocator.
        buffer_used_ = core::AlignUp(buffer_used_, request.alignment);
        if (buffer_used_ + request.size > kBufferSize) {
            return nullptr;
        }
        void* p = core::AddBytes(buffer_, buffer_used_);
        buffer_used_ += request.size;
        last_tag_ = request.tag;
        return p;
    }

    void Deallocate(const core::AllocationInfo& /*info*/) noexcept override {
        // no-op (linear)
    }

    bool TryGetStats(core::AllocatorStats& out) const noexcept override {
        out = {};
        out.bytes_allocated_current = buffer_used_;
        out.bytes_allocated_peak = buffer_used_;
        out.bytes_allocated_total = buffer_used_;
        return true;
    }

    core::memory_tag last_tag() const noexcept { return last_tag_; }

private:
    static constexpr core::memory_size kBufferSize = 1024;
    alignas(64) core::byte buffer_[kBufferSize]{};
    core::memory_size buffer_used_ = 0;
    core::memory_tag last_tag_ = 0;
};

} // namespace

TEST(CoreMemory, AlignUpDownValues) {
    EXPECT_EQ(core::AlignUp(0u, 16u), 0u);
    EXPECT_EQ(core::AlignUp(1u, 16u), 16u);
    EXPECT_EQ(core::AlignUp(16u, 16u), 16u);
    EXPECT_EQ(core::AlignUp(17u, 16u), 32u);

    EXPECT_EQ(core::AlignDown(0u, 16u), 0u);
    EXPECT_EQ(core::AlignDown(1u, 16u), 0u);
    EXPECT_EQ(core::AlignDown(16u, 16u), 16u);
    EXPECT_EQ(core::AlignDown(31u, 16u), 16u);

    EXPECT_TRUE(core::IsAlignedValue(32u, 16u));
    EXPECT_FALSE(core::IsAlignedValue(33u, 16u));
}

TEST(CoreMemory, PointerHelpers) {
    alignas(64) core::byte data[128]{};
    void* p = data;

    EXPECT_TRUE(core::IsAlignedPtr(p, 64u));
    void* p2 = core::AddBytes(p, 7);
    EXPECT_FALSE(core::IsAlignedPtr(p2, 64u));

    void* p3 = core::AlignPtrUp(p2, 64u);
    EXPECT_TRUE(core::IsAlignedPtr(p3, 64u));
    EXPECT_LE(core::PtrDiffBytes(p3, p), 64u);
}

TEST(CoreMemory, AllocatorInterfaceSmoke) {
    TestAllocator a;

    core::AllocationRequest req{};
    req.size = 24;
    req.alignment = 16;
    req.tag = 42;

    void* p = a.Allocate(req);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(core::IsAlignedPtr(p, req.alignment));

    core::AllocatorStats stats{};
    EXPECT_TRUE(a.TryGetStats(stats));
    EXPECT_GE(stats.bytes_allocated_current, req.size);

    core::AllocationInfo info{};
    info.ptr = p;
    info.size = req.size;
    info.alignment = req.alignment;
    info.tag = req.tag;
    a.Deallocate(info);

    EXPECT_EQ(a.last_tag(), 42u);
}

