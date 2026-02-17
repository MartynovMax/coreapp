#include "environment_metadata.hpp"
#include "../common/string_utils.hpp"
#include "../common/file_utils.hpp"
#include "../../core/base/core_macros.hpp"
#include "../../core/base/core_platform.hpp"
#include "../../core/base/core_build.hpp"
#include "../../core/memory/memory_ops.hpp"

#if CORE_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#elif CORE_PLATFORM_LINUX
#include <sys/utsname.h>
#include <unistd.h>
#elif CORE_PLATFORM_MACOS
#include <sys/utsname.h>
#include <sys/sysctl.h>
#include <unistd.h>
#endif

namespace core {
namespace bench {

namespace {

// Static buffers for string data (thread-safe: filled once per run)
static char s_timestampBuffer[64];
static char s_osVersionBuffer[256];
static char s_buildFlagsBuffer[256];
static char s_cpuModelBuffer[256];

// Format timestamp as string representation (nanoseconds since epoch)
void FormatTimestampUtc(char* buffer, usize bufferSize, u64 timestampNs) noexcept {
    U64ToString(timestampNs, buffer, bufferSize);
}

// Collect OS version (platform-specific)
const char* CollectOsVersion() noexcept {
#if CORE_PLATFORM_WINDOWS
    OSVERSIONINFOEXW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    // Modern Windows: RtlGetVersion (GetVersionEx is deprecated)
    typedef LONG(WINAPI* RtlGetVersionFunc)(OSVERSIONINFOEXW*);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll != nullptr) {
        RtlGetVersionFunc pRtlGetVersion =
            (RtlGetVersionFunc)GetProcAddress(ntdll, "RtlGetVersion");
        if (pRtlGetVersion != nullptr) {
            if (pRtlGetVersion(&osvi) == 0) {
                // Format: "major.minor.build"
                char temp[32];
                usize pos = 0;
                
                pos = U32ToString(osvi.dwMajorVersion, temp, sizeof(temp));
                if (pos > 0) {
                    pos = AppendString(s_osVersionBuffer, sizeof(s_osVersionBuffer), 0, temp);
                    pos = AppendChar(s_osVersionBuffer, sizeof(s_osVersionBuffer), pos, '.');
                    
                    U32ToString(osvi.dwMinorVersion, temp, sizeof(temp));
                    pos = AppendString(s_osVersionBuffer, sizeof(s_osVersionBuffer), pos, temp);
                    pos = AppendChar(s_osVersionBuffer, sizeof(s_osVersionBuffer), pos, '.');
                    
                    U32ToString(osvi.dwBuildNumber, temp, sizeof(temp));
                    AppendString(s_osVersionBuffer, sizeof(s_osVersionBuffer), pos, temp);
                    
                    return s_osVersionBuffer;
                }
            }
        }
    }
    return "unknown";

#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        SafeStringCopy(s_osVersionBuffer, unameData.release, sizeof(s_osVersionBuffer));
        return s_osVersionBuffer;
    }
    return "unknown";

#else
    return "unknown";
#endif
}

// Collect compiler version string
const char* CollectCompilerVersion() noexcept {
#ifdef CORE_COMPILER_VERSION_STRING
    // Use human-readable version from CMake if available
    return CORE_COMPILER_VERSION_STRING;
#else
    // Fallback to numeric version from compiler macros
    return CORE_STRINGIFY(CORE_COMPILER_VERSION);
#endif
}

// Collect normalized build flags
const char* CollectBuildFlags() noexcept {
    usize pos = FormatBuildFlags(s_buildFlagsBuffer, sizeof(s_buildFlagsBuffer),
                                  CORE_BUILD_DEBUG,
                                  CORE_BUILD_WITH_OPTIMIZATIONS,
                                  CORE_BUILD_WITH_ASSERTS);

    // Add memory tracking flag if enabled
#ifdef CORE_MEMORY_TRACKING
    if (pos > 0 && pos < sizeof(s_buildFlagsBuffer)) {
        char temp[32];
        pos = AppendString(s_buildFlagsBuffer, sizeof(s_buildFlagsBuffer), pos, ";tracking=");
        U32ToString(CORE_MEMORY_TRACKING, temp, sizeof(temp));
        pos = AppendString(s_buildFlagsBuffer, sizeof(s_buildFlagsBuffer), pos, temp);
    }
#endif

    // Append CMake build flags if available (additional context)
#ifdef CORE_BUILD_FLAGS_CMAKE
    if (pos > 0 && pos < sizeof(s_buildFlagsBuffer) - 10) {
        AppendString(s_buildFlagsBuffer, sizeof(s_buildFlagsBuffer), pos, ";cmake_flags=");
        // Note: Full CMAKE flags in separate field would be better, but keeping simple for now
    }
#endif

    return s_buildFlagsBuffer;
}

// Collect CPU model string (platform-specific)
const char* CollectCpuModel() noexcept {
#if CORE_PLATFORM_WINDOWS && (CORE_CPU_X86 || CORE_CPU_X64)
    i32 cpuInfo[4] = {0};
    char* ptr = s_cpuModelBuffer;

    // Get CPU brand string (requires 3 CPUID calls)
    __cpuid(cpuInfo, 0x80000000);
    u32 maxExtended = cpuInfo[0];

    if (maxExtended >= 0x80000004) {
        __cpuid((i32*)(ptr + 0), 0x80000002);
        __cpuid((i32*)(ptr + 16), 0x80000003);
        __cpuid((i32*)(ptr + 32), 0x80000004);
        s_cpuModelBuffer[48] = '\0';

        // Trim leading spaces
        ptr = s_cpuModelBuffer;
        while (*ptr == ' ') ptr++;
        if (ptr != s_cpuModelBuffer) {
            usize len = StringLength(ptr);
            core::memory_move(s_cpuModelBuffer, ptr, len + 1);
        }

        return s_cpuModelBuffer;
    }
    return "unknown";

#elif CORE_PLATFORM_LINUX
    FileHandle* cpuinfo = OpenFileForReading("/proc/cpuinfo");
    if (cpuinfo != nullptr) {
        char line[512];
        while (ReadLine(cpuinfo, line, sizeof(line))) {
            if (StringStartsWith(line, "model name", 10)) {
                char* colon = FindChar(line, ':');
                if (colon != nullptr) {
                    colon++; // Skip ':'
                    while (*colon == ' ' || *colon == '\t') colon++; // Trim

                    // Copy to buffer and remove newline
                    SafeStringCopy(s_cpuModelBuffer, colon, sizeof(s_cpuModelBuffer));
                    char* newline = FindChar(s_cpuModelBuffer, '\n');
                    if (newline) *newline = '\0';

                    CloseFile(cpuinfo);
                    return s_cpuModelBuffer;
                }
            }
        }
        CloseFile(cpuinfo);
    }
    return "unknown";

#elif CORE_PLATFORM_MACOS
    usize size = sizeof(s_cpuModelBuffer);
    if (sysctlbyname("machdep.cpu.brand_string", s_cpuModelBuffer, &size, nullptr, 0) == 0) {
        return s_cpuModelBuffer;
    }
    return "unknown";

#else
    return "unknown";
#endif
}

// Collect logical CPU core count
u32 CollectCpuCoresLogical() noexcept {
#if CORE_PLATFORM_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return static_cast<u32>(sysInfo.dwNumberOfProcessors);

#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return (count > 0) ? static_cast<u32>(count) : 0;

#else
    return 0;
#endif
}

// Collect physical CPU core count (best-effort)
u32 CollectCpuCoresPhysical() noexcept {
#if CORE_PLATFORM_WINDOWS
    DWORD length = 0;
    GetLogicalProcessorInformation(nullptr, &length);

    if (length > 0) {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer =
            (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(length);

        if (buffer != nullptr) {
            if (GetLogicalProcessorInformation(buffer, &length)) {
                u32 physicalCores = 0;
                DWORD count = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

                for (DWORD i = 0; i < count; ++i) {
                    if (buffer[i].Relationship == RelationProcessorCore) {
                        physicalCores++;
                    }
                }

                free(buffer);
                return physicalCores;
            }
            free(buffer);
        }
    }
    return 0;

#elif CORE_PLATFORM_LINUX
    FileHandle* cpuinfo = OpenFileForReading("/proc/cpuinfo");
    if (cpuinfo != nullptr) {
        u32 coreCount = 0;
        u32 lastPhysicalId = 0xFFFFFFFF;
        u32 lastCoreId = 0xFFFFFFFF;
        bool foundCore = false;

        char line[256];
        while (ReadLine(cpuinfo, line, sizeof(line))) {
            u32 value;
            if (ParseKeyValueU32(line, "physical id", value)) {
                lastPhysicalId = value;
            } else if (ParseKeyValueU32(line, "core id", value)) {
                if (value != lastCoreId || !foundCore) {
                    lastCoreId = value;
                    foundCore = true;
                    coreCount++;
                }
            }
        }
        CloseFile(cpuinfo);

        return (coreCount > 0) ? coreCount : 0;
    }
    return 0;

#elif CORE_PLATFORM_MACOS
    u32 physicalCores = 0;
    usize size = sizeof(physicalCores);
    if (sysctlbyname("hw.physicalcpu", &physicalCores, &size, nullptr, 0) == 0) {
        return physicalCores;
    }
    return 0;

#else
    return 0;
#endif
}

} // anonymous namespace

void CollectEnvironmentMetadata(RunMetadata& metadata) noexcept {
    // Timestamp (use existing startTimestampNs from RunMetadata)
    FormatTimestampUtc(s_timestampBuffer, sizeof(s_timestampBuffer), metadata.startTimestampNs);
    metadata.runTimestampUtc = s_timestampBuffer;

    // OS info (compile-time + runtime)
    metadata.osName = CORE_PLATFORM_NAME;
    metadata.osVersion = CollectOsVersion();

    // Architecture (compile-time)
    metadata.arch = CORE_CPU_NAME;

    // Compiler info (compile-time + formatted version)
    metadata.compilerName = CORE_COMPILER_NAME;
    metadata.compilerVersion = CollectCompilerVersion();

    // Build info (compile-time)
    metadata.buildType = CORE_BUILD_CONFIG_NAME;
    metadata.buildFlags = CollectBuildFlags();

    // CPU info (runtime)
    metadata.cpuModel = CollectCpuModel();
    metadata.cpuCoresLogical = CollectCpuCoresLogical();
    metadata.cpuCoresPhysical = CollectCpuCoresPhysical();
}

} // namespace bench
} // namespace core
