#pragma once

// =============================================================================
// lifetime_tracker.hpp
// Tracking of live objects and implementation of lifetime models.
// =============================================================================

#include "../../core/base/core_types.hpp"
#include "workload_params.hpp"

namespace core {
namespace bench {

struct AllocInfo {
    void* ptr;
    u32 size;
    u64 allocTime; // Operation index when allocated
};

} // namespace bench
} // namespace core
