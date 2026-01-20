#include "string_utils.hpp"

namespace core {
namespace bench {

bool StringsEqual(const char* a, const char* b) noexcept {
    if (a == nullptr || b == nullptr) {
        return a == b;
    }
    while (*a && *b) {
        if (*a != *b) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == *b;
}

bool StartsWith(const char* str, const char* prefix) noexcept {
    if (str == nullptr || prefix == nullptr) {
        return false;
    }
    while (*prefix) {
        if (*str != *prefix) {
            return false;
        }
        ++str;
        ++prefix;
    }
    return true;
}

const char* ExtractValue(const char* arg, const char* flagPrefix) noexcept {
    if (!StartsWith(arg, flagPrefix)) {
        return nullptr;
    }
    const char* pos = arg;
    while (*pos && *pos != '=') {
        ++pos;
    }
    if (*pos == '=') {
        return pos + 1;
    }
    return nullptr;
}

} // namespace bench
} // namespace core
