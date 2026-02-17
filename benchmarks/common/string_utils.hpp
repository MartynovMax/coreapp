#pragma once

// =============================================================================
// string_utils.hpp
// String utility functions for benchmarks.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// Compare two strings for equality
// Returns true if strings are equal (handles nullptr)
bool StringsEqual(const char* a, const char* b) noexcept;

// Check if string starts with prefix
bool StartsWith(const char* str, const char* prefix) noexcept;

// Extract value after '=' in "--flag=value" format
// Returns nullptr if no '=' found, prefix doesn't match, or flag doesn't end with '='
const char* ExtractValue(const char* arg, const char* flagPrefix) noexcept;

// Parse unsigned 64-bit integer from string
bool ParseU64(const char* str, u64& outValue) noexcept;

// Parse unsigned 32-bit integer from string
bool ParseU32(const char* str, u32& outValue) noexcept;

// Compute string length (custom implementation to avoid <string.h>)
usize StringLength(const char* str) noexcept;

// Find first occurrence of character in string
// Returns pointer to character or nullptr if not found
char* FindChar(char* str, char ch) noexcept;

// Check if string starts with prefix (compare first n characters)
bool StringStartsWith(const char* str, const char* prefix, usize n) noexcept;

// Safe string copy with null termination (copies up to dstSize-1 chars)
void SafeStringCopy(char* dst, const char* src, usize dstSize) noexcept;

// Convert u64 to string (returns number of characters written, excluding null terminator)
// Returns 0 if buffer is too small or invalid
usize U64ToString(u64 value, char* buffer, usize bufferSize) noexcept;

// Convert u32 to string (returns number of characters written, excluding null terminator)
// Returns 0 if buffer is too small or invalid
usize U32ToString(u32 value, char* buffer, usize bufferSize) noexcept;

// Format build flags string like "debug=1;opt=0;asserts=1"
// Returns number of characters written (excluding null terminator)
usize FormatBuildFlags(char* buffer, usize bufferSize, 
                       i32 debug, i32 opt, i32 asserts) noexcept;

// Append string to buffer at given position (returns new position)
// Returns 0 if buffer is too small
usize AppendString(char* buffer, usize bufferSize, usize position, const char* str) noexcept;

// Append character to buffer at given position (returns new position)
// Returns 0 if buffer is too small
usize AppendChar(char* buffer, usize bufferSize, usize position, char ch) noexcept;

// Parse "key : value" line and extract u32 value
// Returns true if successfully parsed (e.g., "physical id : 0" -> 0)
bool ParseKeyValueU32(const char* line, const char* key, u32& outValue) noexcept;

} // namespace bench
} // namespace core
