#pragma once

// =============================================================================
// pattern_matcher.hpp
// Wildcard pattern matching for experiment filtering.
//
// Supported wildcards:
//   - '*' matches zero or more characters
//   - '?' matches exactly one character
// =============================================================================

namespace core {
namespace bench {

// Match string against wildcard pattern
// Returns true if str matches pattern
bool PatternMatch(const char* pattern, const char* str) noexcept;

} // namespace bench
} // namespace core
