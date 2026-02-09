#pragma once

// =============================================================================
// operation_stream.hpp
// Deterministic operation stream generator.
//
// Generates deterministic sequences of allocation/free operations with
// configurable size distributions and operation ratios.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "core/memory/core_allocator.hpp"
#include "workload_params.hpp"
#include "common/seeded_rng.hpp"

namespace core::bench {

// ----------------------------------------------------------------------------
// OpType - Operation type
// ----------------------------------------------------------------------------

enum class OpType {
    Alloc,              // Allocation operation
    Free,               // Free operation
};

// ----------------------------------------------------------------------------
// OpReason - Operation generation reason
// ----------------------------------------------------------------------------

enum class OpReason {
    Normal,                  // Normal operation based on ratio
    ForcedAllocEmptyLive,   // Forced alloc when live-set empty
    NoopFreeEmptyLive,      // No-op free when live-set empty
};

// ----------------------------------------------------------------------------
// Operation - Single operation in the workload
// ----------------------------------------------------------------------------

struct Operation {
    OpType type;
    OpReason reason = OpReason::Normal;
    u32 size = 0;
    core::memory_alignment alignment = 0;
    core::memory_tag tag = 0;
    core::AllocationFlags flags = core::AllocationFlags::None;
    void* ptr = nullptr;
};

// ----------------------------------------------------------------------------
// OperationStream - Deterministic operation generator
// ----------------------------------------------------------------------------

class OperationStream {
public:
    explicit OperationStream(const WorkloadParams& params) noexcept;
    ~OperationStream() noexcept = default;

    OperationStream(const OperationStream&) = delete;
    OperationStream& operator=(const OperationStream&) = delete;

    OperationStream(OperationStream&&) noexcept = default;
    OperationStream& operator=(OperationStream&&) noexcept = default;

    Operation Next(u64 liveCount) noexcept;

    // Check if more operations available
    bool HasNext() const noexcept;

private:
    WorkloadParams _params;
    SeededRNG _ownedRng;
    u64 _currentOp = 0;
    
    // Normalized storage for custom buckets
    static constexpr u32 kMaxSizeBuckets = 32;
    static constexpr u32 kMaxAlignmentBuckets = 16;
    
    u32 _normalizedSizeBuckets[kMaxSizeBuckets];
    f32 _normalizedSizeWeights[kMaxSizeBuckets];
    u32 _normalizedSizeBucketCount = 0;
    
    core::memory_alignment _normalizedAlignmentBuckets[kMaxAlignmentBuckets];
    f32 _normalizedAlignmentWeights[kMaxAlignmentBuckets];
    u32 _normalizedAlignmentBucketCount = 0;

    u32 GenerateSize() noexcept;
    core::memory_alignment GenerateAlignment(u32 size) noexcept;
    OpType DecideOperation() noexcept;
    f32 NextUnitFloat01() noexcept;

    static core::memory_alignment NextPow2(core::memory_alignment v) noexcept;
};

} // namespace core::bench
