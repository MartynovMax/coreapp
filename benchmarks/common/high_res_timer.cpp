#include "high_res_timer.hpp"
#include "../../core/base/core_platform.hpp"

#if CORE_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    #include <time.h>
#else
    #error "HighResTimer: Unsupported platform"
#endif

namespace core {
namespace bench {

HighResTimer::HighResTimer() noexcept {
#if CORE_PLATFORM_WINDOWS
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    _frequency = static_cast<u64>(freq.QuadPart);
#else
    _frequency = 1000000000ull; // 1ns resolution for clock_gettime
#endif
}

u64 HighResTimer::Now() const noexcept {
#if CORE_PLATFORM_WINDOWS
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    // Convert to nanoseconds
    u64 ticks = static_cast<u64>(counter.QuadPart);
    return (ticks * 1000000000ull) / _frequency;
#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_MACOS
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<u64>(ts.tv_sec) * 1000000000ull + static_cast<u64>(ts.tv_nsec);
#endif
}

u64 HighResTimer::Elapsed(u64 start) const noexcept {
    u64 now = Now();
    return (now >= start) ? (now - start) : 0;
}

} // namespace bench
} // namespace core
