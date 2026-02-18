#pragma once

// =============================================================================
// optimization_barriers.hpp
// Compiler optimization barriers for benchmark code.
//
// Prevents compilers from optimizing away measured operations (DCE).
// Required by timing noise-control protocol.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

template<typename T>
inline void DoNotOptimize(const T& value) noexcept {
#if defined(_MSC_VER)
    _ReadWriteBarrier();
    (void)value;
#elif defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "r,m"(value) : "memory");
#else
    volatile const T* ptr = &value;
    (void)ptr;
#endif
}

} // namespace bench
} // namespace core
