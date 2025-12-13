#pragma once

// =============================================================================
// core_fail.hpp
// Fail-fast / trap primitive for Core.
//
// Goals:
// - Header-only, tiny, predictable.
// - No STL, no exceptions, no CRT (no abort/exit).
// - Safe to use during early initialization.
// - Centralized termination behavior for asserts / unreachable / invariants.
//
// NOTE:
// This header may include compiler-provided intrinsic headers (e.g.
// <intrin.h>), which are not part of the C runtime.
// =============================================================================

#include "core_config.hpp"

#if CORE_COMPILER_MSVC
// Provides __debugbreak / __fastfail intrinsics.
#include <intrin.h>
#endif

namespace core {
// Immediately terminates execution.
//
// Properties:
// - [[noreturn]] and noexcept
// - tries to break into an attached debugger first (where supported)
// - then triggers a trap / fastfail that stops execution predictably
[[noreturn]] CORE_NO_INLINE inline void FailFast() noexcept {
#if CORE_COMPILER_MSVC
  // Break into debugger if attached.
  __debugbreak();

  // Fast-fail: raises a non-continuable exception that terminates the process.
  // This is an intrinsic (no CRT) and works very early in program lifetime.
  __fastfail(0);

  // Satisfy [[noreturn]] for static analysis / optimizer.
  __assume(0);

#elif CORE_COMPILER_CLANG || CORE_COMPILER_GCC
  // Compiler intrinsic trap emits an illegal instruction.
  __builtin_trap();
  __builtin_unreachable();

#else
  // Unknown compiler: best-effort hard stop without CRT/OS APIs.
  // Volatile null write is a common way to trigger an immediate crash.
  *static_cast<volatile int *>(nullptr) = 0;

  // If the above is somehow optimized away, keep the function non-returning.
  for (;;) {
  }
#endif
}
} // namespace core