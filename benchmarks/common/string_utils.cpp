#include "string_utils.hpp"

namespace core {
namespace bench {

namespace {

bool IsDigit(char c) noexcept {
    return c >= '0' && c <= '9';
}

} // anonymous namespace

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

bool ParseU64(const char* str, u64& outValue) noexcept {
    if (str == nullptr || *str == '\0') {
        return false;
    }

    u64 result = 0;
    const char* p = str;

    while (*p) {
        if (!IsDigit(*p)) {
            return false;
        }

        u64 digit = static_cast<u64>(*p - '0');
        if (result > (0xFFFFFFFFFFFFFFFFull - digit) / 10) {
            return false; // Overflow
        }

        result = result * 10 + digit;
        ++p;
    }

    outValue = result;
    return true;
}

bool ParseU32(const char* str, u32& outValue) noexcept {
    u64 temp = 0;
    if (!ParseU64(str, temp)) {
        return false;
    }
    if (temp > 0xFFFFFFFFu) {
        return false;
    }
    outValue = static_cast<u32>(temp);
    return true;
}

} // namespace bench
} // namespace core
