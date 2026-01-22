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

} // namespace bench
} // namespace core
