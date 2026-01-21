#include "pattern_matcher.hpp"

namespace core {
namespace bench {

namespace {

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
            starPos = p++;
            strBacktrack = s;
        } else if (CharMatch(*p, *s)) {
            ++p;
            ++s;
        } else if (starPos != nullptr) {
            p = starPos + 1;
            s = ++strBacktrack;
        } else {
            return false;
        }
    }

    while (*p == '*') {
        ++p;
    }

    return *p == '\0';
}

} // namespace bench
} // namespace core
