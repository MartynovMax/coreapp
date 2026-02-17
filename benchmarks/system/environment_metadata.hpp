#pragma once

// =============================================================================
// environment_metadata.hpp
// Collection of runtime environment and build metadata for benchmark outputs.
// =============================================================================

#include "../output/run_metadata.hpp"
#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

void CollectEnvironmentMetadata(RunMetadata& metadata) noexcept;

} // namespace bench
} // namespace core
