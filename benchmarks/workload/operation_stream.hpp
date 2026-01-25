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
// Operation - Single operation in the workload
// ----------------------------------------------------------------------------

struct Operation {
    OpType type;
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
    OperationStream(const WorkloadParams& params, SeededRNG& rng) noexcept;
    ~OperationStream() noexcept = default;

    // Generate next operation
    Operation Next() noexcept;

    // Check if more operations available
    bool HasNext() const noexcept;
private:
    const WorkloadParams& _params;
    SeededRNG& _rng;
    u64 _currentOp = 0;
    u32 GenerateSize() const noexcept;
    core::memory_alignment GenerateAlignment(u32 size) const noexcept;
    OpType DecideOperation() const noexcept;

    static core::memory_alignment NextPow2(core::memory_alignment v) noexcept;
};

} // namespace core::bench
