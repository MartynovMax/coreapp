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
    if (arg == nullptr || flagPrefix == nullptr) {
        return nullptr;
    }

    const char* a = arg;
    const char* p = flagPrefix;
    while (*p) {
        if (*a != *p) {
            return nullptr;
        }
        ++a;
        ++p;
    }

    if (*a == '=') {
        return a + 1;
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

usize StringLength(const char* str) noexcept {
    if (str == nullptr) {
        return 0;
    }
    usize len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

char* FindChar(char* str, char ch) noexcept {
    if (str == nullptr) {
        return nullptr;
    }
    while (*str != '\0') {
        if (*str == ch) {
            return str;
        }
        ++str;
    }
    return nullptr;
}

bool StringStartsWith(const char* str, const char* prefix, usize n) noexcept {
    if (str == nullptr || prefix == nullptr) {
        return false;
    }
    for (usize i = 0; i < n; ++i) {
        if (str[i] == '\0' || str[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

void SafeStringCopy(char* dst, const char* src, usize dstSize) noexcept {
    if (dst == nullptr || src == nullptr || dstSize == 0) {
        return;
    }
    usize i = 0;
    while (i < dstSize - 1 && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

usize U64ToString(u64 value, char* buffer, usize bufferSize) noexcept {
    if (buffer == nullptr || bufferSize == 0) {
        return 0;
    }

    // Handle zero case
    if (value == 0) {
        if (bufferSize < 2) {
            return 0;
        }
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }

    // Count digits
    u64 temp = value;
    usize digitCount = 0;
    while (temp > 0) {
        temp /= 10;
        ++digitCount;
    }

    // Check buffer size
    if (digitCount + 1 > bufferSize) {
        return 0;
    }

    // Write digits in reverse
    buffer[digitCount] = '\0';
    usize pos = digitCount;
    while (value > 0) {
        --pos;
        buffer[pos] = '0' + static_cast<char>(value % 10);
        value /= 10;
    }

    return digitCount;
}

usize U32ToString(u32 value, char* buffer, usize bufferSize) noexcept {
    return U64ToString(static_cast<u64>(value), buffer, bufferSize);
}

usize AppendString(char* buffer, usize bufferSize, usize position, const char* str) noexcept {
    if (buffer == nullptr || str == nullptr || position >= bufferSize) {
        return 0;
    }

    usize i = 0;
    while (str[i] != '\0' && position + i < bufferSize - 1) {
        buffer[position + i] = str[i];
        ++i;
    }

    buffer[position + i] = '\0';
    return position + i;
}

usize AppendChar(char* buffer, usize bufferSize, usize position, char ch) noexcept {
    if (buffer == nullptr || position >= bufferSize - 1) {
        return 0;
    }

    buffer[position] = ch;
    buffer[position + 1] = '\0';
    return position + 1;
}

usize FormatBuildFlags(char* buffer, usize bufferSize, 
                       i32 debug, i32 opt, i32 asserts) noexcept {
    if (buffer == nullptr || bufferSize == 0) {
        return 0;
    }

    usize pos = 0;

    // "debug="
    pos = AppendString(buffer, bufferSize, pos, "debug=");
    if (pos == 0) return 0;

    // debug value
    char temp[16];
    usize len = U32ToString(static_cast<u32>(debug), temp, sizeof(temp));
    if (len == 0) return 0;
    pos = AppendString(buffer, bufferSize, pos, temp);
    if (pos == 0) return 0;

    // ";opt="
    pos = AppendString(buffer, bufferSize, pos, ";opt=");
    if (pos == 0) return 0;

    // opt value
    len = U32ToString(static_cast<u32>(opt), temp, sizeof(temp));
    if (len == 0) return 0;
    pos = AppendString(buffer, bufferSize, pos, temp);
    if (pos == 0) return 0;

    // ";asserts="
    pos = AppendString(buffer, bufferSize, pos, ";asserts=");
    if (pos == 0) return 0;

    // asserts value
    len = U32ToString(static_cast<u32>(asserts), temp, sizeof(temp));
    if (len == 0) return 0;
    pos = AppendString(buffer, bufferSize, pos, temp);
    if (pos == 0) return 0;

    return pos;
}

bool ParseKeyValueU32(const char* line, const char* key, u32& outValue) noexcept {
    if (line == nullptr || key == nullptr) {
        return false;
    }

    // Check if line starts with key
    const char* p = line;
    const char* k = key;
    while (*k != '\0') {
        if (*p != *k) {
            return false;
        }
        ++p;
        ++k;
    }

    // Skip spaces
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    // Expect ':'
    if (*p != ':') {
        return false;
    }
    ++p;

    // Skip spaces after ':'
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    // Parse number
    return ParseU32(p, outValue);
}

} // namespace bench
} // namespace core
