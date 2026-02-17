#pragma once

// =============================================================================
// file_utils.hpp
// Minimal file reading utilities for benchmarks (isolates stdio.h usage).
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// Opaque file handle (hides implementation details)
struct FileHandle;

// Open file for reading (returns nullptr on failure)
FileHandle* OpenFileForReading(const char* path) noexcept;

// Close file handle
void CloseFile(FileHandle* handle) noexcept;

// Read one line from file (returns false on EOF or error)
// Line buffer must be pre-allocated; bufferSize includes null terminator
// Newline characters are included in the output
bool ReadLine(FileHandle* handle, char* buffer, usize bufferSize) noexcept;

} // namespace bench
} // namespace core
