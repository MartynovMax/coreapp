#pragma once

// =============================================================================
// string_utils.hpp
// String utility functions for benchmarks.
// =============================================================================

namespace core {
namespace bench {

// Compare two strings for equality
// Returns true if strings are equal (handles nullptr)
bool StringsEqual(const char* a, const char* b) noexcept;

// Check if string starts with prefix
bool StartsWith(const char* str, const char* prefix) noexcept;

// Extract value after '=' in "--flag=value" format
// Returns nullptr if no '=' found or if prefix doesn't match
const char* ExtractValue(const char* arg, const char* flagPrefix) noexcept;

} // namespace bench
} // namespace core
