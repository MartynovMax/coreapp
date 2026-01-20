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

} // namespace bench
} // namespace core
