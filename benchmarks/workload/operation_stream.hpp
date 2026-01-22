#pragma once

// =============================================================================
// operation_stream.hpp
// Deterministic operation stream generator.
//
// Generates deterministic sequences of allocation/free operations with
// configurable size distributions and operation ratios.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "workload_params.hpp"

namespace core {
namespace bench {

// Forward declarations
class SeededRNG;

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
    u32 size;           // For Alloc: size to allocate
    void* ptr;          // For Free: pointer to free (filled by LifetimeTracker)
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

    u32 GenerateSize() noexcept;
    OpType DecideOperation() noexcept;
};

} // namespace bench
} // namespace core
