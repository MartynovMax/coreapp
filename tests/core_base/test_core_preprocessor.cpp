#include <gtest/gtest.h>

#include "core/base/core_preprocessor.hpp"

namespace {

// -----------------------------------------------------------------------------
// C++ standard version helpers
// -----------------------------------------------------------------------------
static_assert(CORE_CPP20_OR_GREATER,
              "Project is expected to compile as C++20+.");
static_assert(CORE_CPP17_OR_GREATER, "C++17+ must be true when C++20+.");
static_assert(CORE_CPP14_OR_GREATER, "C++14+ must be true when C++20+.");
static_assert(CORE_CPP_VERSION >= 202002L,
              "CORE_CPP_VERSION must reflect C++20+.");

// -----------------------------------------------------------------------------
// Attribute detection helpers
// -----------------------------------------------------------------------------
static_assert(CORE_HAS_NODISCARD == 0 || CORE_HAS_NODISCARD == 1,
              "CORE_HAS_NODISCARD must be 0/1.");
static_assert(CORE_HAS_NORETURN == 0 || CORE_HAS_NORETURN == 1,
              "CORE_HAS_NORETURN must be 0/1.");
static_assert(CORE_HAS_FALLTHROUGH == 0 || CORE_HAS_FALLTHROUGH == 1,
              "CORE_HAS_FALLTHROUGH must be 0/1.");
static_assert(CORE_HAS_DEPRECATED == 0 || CORE_HAS_DEPRECATED == 1,
              "CORE_HAS_DEPRECATED must be 0/1.");

// In C++20 mode, these attributes should be available (at least as standard
// attrs).
static_assert(CORE_HAS_NODISCARD, "nodiscard expected in C++20 mode.");
static_assert(CORE_HAS_NORETURN, "noreturn expected in C++20 mode.");
static_assert(CORE_HAS_FALLTHROUGH, "fallthrough expected in C++20 mode.");
static_assert(CORE_HAS_DEPRECATED, "deprecated expected in C++20 mode.");

// -----------------------------------------------------------------------------
// Header availability helpers
// -----------------------------------------------------------------------------
#if CORE_HAS_INCLUDE(<cstddef>)
static_assert(true, "<cstddef> detected via CORE_HAS_INCLUDE.");
#else
static_assert(true, "CORE_HAS_INCLUDE degrades safely (no __has_include or "
                    "header not detectable).");
#endif

// -----------------------------------------------------------------------------
// Builtin detection helpers
// -----------------------------------------------------------------------------
#if CORE_HAS_BUILTIN(__builtin_expect)
static_assert(
    true, "__builtin_expect is available (as detected by CORE_HAS_BUILTIN).");
#else
static_assert(true, "CORE_HAS_BUILTIN degrades safely (no __has_builtin or "
                    "builtin unavailable).");
#endif

// -----------------------------------------------------------------------------
// General feature flags (exceptions/RTTI/thread_local)
// -----------------------------------------------------------------------------
static_assert(CORE_HAS_EXCEPTIONS == 0 || CORE_HAS_EXCEPTIONS == 1,
              "CORE_HAS_EXCEPTIONS must be 0/1.");
static_assert(CORE_HAS_RTTI == 0 || CORE_HAS_RTTI == 1,
              "CORE_HAS_RTTI must be 0/1.");
static_assert(CORE_HAS_THREAD_LOCAL == 0 || CORE_HAS_THREAD_LOCAL == 1,
              "CORE_HAS_THREAD_LOCAL must be 0/1.");

} // namespace

TEST(CorePreprocessor, Smoke) {
  // Runtime smoke only; real validation is compile-time above.
  SUCCEED();
}
