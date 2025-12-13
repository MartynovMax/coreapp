#pragma once

#include "core/base/core_config.hpp"

#if CORE_COMPILER_MSVC
// Provides __debugbreak / __fastfail intrinsics.
#include <intrin.h>
#endif

namespace core {
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
		* static_cast<volatile int*>(nullptr) = 0;

		// If the above is somehow optimized away, keep the function non-returning.
		for (;;) {
		}
#endif
	}
}