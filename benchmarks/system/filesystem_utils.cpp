#include "filesystem_utils.hpp"
#include "../common/string_utils.hpp"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#define PLATFORM_MKDIR(path) _mkdir(path)
#define PLATFORM_STAT(path, st) _stat(path, st)
using PlatformStat = struct _stat;
#else
#include <sys/stat.h>
#include <sys/types.h>
#define PLATFORM_MKDIR(path) mkdir(path, 0755)
#define PLATFORM_STAT(path, st) stat(path, st)
using PlatformStat = struct stat;
#endif

namespace core {
namespace bench {

bool CreateDirectoriesRecursive(const char* path) noexcept {
    if (path == nullptr || *path == '\0') {
        return false;
    }

    // Work on a mutable copy
    char buffer[1024];
    usize len = StringLength(path);
    if (len >= sizeof(buffer)) {
        return false;
    }
    SafeStringCopy(buffer, path, sizeof(buffer));

    // Normalize separators to '/'
    for (usize i = 0; i < len; ++i) {
        if (buffer[i] == '\\') buffer[i] = '/';
    }

    // Remove trailing slash
    if (len > 0 && buffer[len - 1] == '/') {
        buffer[len - 1] = '\0';
        --len;
    }

    // Create each component
    for (usize i = 1; i <= len; ++i) {
        if (i == len || buffer[i] == '/') {
            char saved = buffer[i];
            buffer[i] = '\0';

            PlatformStat st;
            if (PLATFORM_STAT(buffer, &st) != 0) {
                if (PLATFORM_MKDIR(buffer) != 0) {
                    // Check again (race condition / just created)
                    if (PLATFORM_STAT(buffer, &st) != 0) {
                        return false;
                    }
                }
            }

            buffer[i] = saved;
        }
    }

    return true;
}

void FormatTimestamp(char* buffer, usize bufferSize) noexcept {
    if (buffer == nullptr || bufferSize < 16) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }

    time_t now = time(nullptr);
    struct tm local;

#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif

    snprintf(buffer, bufferSize, "%04d%02d%02d_%02d%02d%02d",
             local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
             local.tm_hour, local.tm_min, local.tm_sec);
}

void SanitizeScenarioName(const char* scenarioName, char* buffer, usize bufferSize) noexcept {
    if (buffer == nullptr || bufferSize == 0) return;
    if (scenarioName == nullptr) {
        buffer[0] = '\0';
        return;
    }

    // Skip leading prefix up to first '/' (e.g. "myexp/")
    const char* start = scenarioName;
    const char* firstSlash = nullptr;
    for (const char* p = scenarioName; *p != '\0'; ++p) {
        if (*p == '/') {
            firstSlash = p;
            break;
        }
    }
    if (firstSlash != nullptr) {
        start = firstSlash + 1;
    }

    // Copy, replacing '/' with '__'
    usize pos = 0;
    for (const char* p = start; *p != '\0' && pos < bufferSize - 1; ++p) {
        if (*p == '/') {
            if (pos + 2 < bufferSize) {
                buffer[pos++] = '_';
                buffer[pos++] = '_';
            } else {
                break;
            }
        } else {
            buffer[pos++] = *p;
        }
    }
    buffer[pos] = '\0';
}

void BuildPath(char* buffer, usize bufferSize, const char* base, const char* child) noexcept {
    if (buffer == nullptr || bufferSize == 0) return;

#ifdef _WIN32
    static constexpr char kSep = '\\';
#else
    static constexpr char kSep = '/';
#endif

    snprintf(buffer, bufferSize, "%s%c%s",
             base ? base : "",
             kSep,
             child ? child : "");
}

} // namespace bench
} // namespace core


