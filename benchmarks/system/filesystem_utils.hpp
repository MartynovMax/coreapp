#pragma once

// =============================================================================
// filesystem_utils.hpp
// Cross-platform filesystem helpers (directory creation, path manipulation).
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// Create directory and all parent directories recursively.
// Returns true on success or if directory already exists.
bool CreateDirectoriesRecursive(const char* path) noexcept;

// Generate a timestamp string in YYYYMMDD_HHMMSS format.
// Buffer must be at least 16 bytes.
void FormatTimestamp(char* buffer, usize bufferSize) noexcept;

// Sanitize a scenario name for use as a directory name.
// Replaces '/' with '__', strips leading prefix up to first '/'.
// e.g. "myexp/malloc/fifo/fixed_small" -> "malloc__fifo__fixed_small"
void SanitizeScenarioName(const char* scenarioName, char* buffer, usize bufferSize) noexcept;

// Build a path by joining components with the platform path separator.
// e.g. BuildPath(buf, 512, "runs/myexp", "20260419_143000") -> "runs/myexp/20260419_143000"
void BuildPath(char* buffer, usize bufferSize, const char* base, const char* child) noexcept;

} // namespace bench
} // namespace core

