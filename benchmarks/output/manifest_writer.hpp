#pragma once

// =============================================================================
// manifest_writer.hpp
// Writes a JSON environment manifest for each batch-run (Task 8).
// =============================================================================

#include "run_metadata.hpp"

namespace core {
namespace bench {

// Write environment manifest to "<basePath>_manifest.json".
// Returns true on success.
bool WriteManifest(const char* basePath, const RunMetadata& metadata) noexcept;

} // namespace bench
} // namespace core

