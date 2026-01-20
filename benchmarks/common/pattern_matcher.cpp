#include "pattern_matcher.hpp"

namespace core {
namespace bench {

namespace {

// Helper: check if character matches (handles '?' wildcard)
bool CharMatch(char patternChar, char strChar) noexcept {
    return (patternChar == '?') || (patternChar == strChar);
}

} // anonymous namespace

bool PatternMatch(const char* pattern, const char* str) noexcept {
    if (pattern == nullptr || str == nullptr) {
        return pattern == str;
    }

    const char* p = pattern;
    const char* s = str;
    const char* starPos = nullptr;
    const char* strBacktrack = nullptr;

    while (*s != '\0') {
        if (*p == '*') {
            // Remember star position for backtracking
            starPos = p++;
            strBacktrack = s;
        } else if (CharMatch(*p, *s)) {
            // Characters match, advance both
            ++p;
            ++s;
        } else if (starPos != nullptr) {
            // Backtrack: try matching star with one more character
            p = starPos + 1;
            s = ++strBacktrack;
        } else {
            // No match and no star to backtrack to
            return false;
        }
    }

    // Handle remaining pattern (must be all stars)
    while (*p == '*') {
        ++p;
    }

    return *p == '\0';
}

} // namespace bench
} // namespace core
