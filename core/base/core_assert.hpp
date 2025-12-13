#pragma once

#include "core/base/core_config.hpp"
#include "core/base/core_fail.hpp"
#include "core/base/core_macros.hpp"

namespace core
{
    using AssertHandler = void (*)(const char* expr_text,
        const char* msg,
        const char* file,
        int line);

    inline AssertHandler SetAssertHandler(AssertHandler handler) noexcept;
    inline AssertHandler GetAssertHandler() noexcept;

    inline void DefaultAssertHandler(const char* expr_text,
        const char* msg,
        const char* file,
        int line) noexcept;

    namespace detail
    {
        inline AssertHandler& AssertHandlerRef() noexcept
        {
            static AssertHandler s_handler = &core::DefaultAssertHandler;
            return s_handler;
        }

        inline void AssertDispatch(const char* expr_text,
            const char* msg,
            const char* file,
            int line) noexcept
        {
            AssertHandler handler = AssertHandlerRef();
            if (handler)
            {
                handler(expr_text, msg, file, line);
                return;
            }

            // If user installed a null handler, fall back to default behavior.
            core::DefaultAssertHandler(expr_text, msg, file, line);
        }
    } // namespace detail

    inline AssertHandler SetAssertHandler(AssertHandler handler) noexcept
    {
        AssertHandler& slot = detail::AssertHandlerRef();
        AssertHandler prev = slot;
        slot = handler ? handler : &core::DefaultAssertHandler;
        return prev;
    }

    inline AssertHandler GetAssertHandler() noexcept
    {
        return detail::AssertHandlerRef();
    }

    inline void DefaultAssertHandler(const char* /*expr_text*/,
        const char* /*msg*/,
        const char* /*file*/,
        int /*line*/) noexcept
    {
        core::FailFast();
    }
} // namespace core

// -----------------------------------------------------------------------------
// Assertion enablement
// -----------------------------------------------------------------------------
#if CORE_ASSERTIONS_ENABLED
    #define CORE_ASSERT_ENABLED 1
#else
    #define CORE_ASSERT_ENABLED 0
#endif

// -----------------------------------------------------------------------------
// Public macros (short names, no CORE_ prefix)
// -----------------------------------------------------------------------------

// Runtime assertion (active when CORE_ASSERTIONS_ENABLED).
#if CORE_ASSERT_ENABLED
    #if defined(ASSERT)
        #error "ASSERT macro is already defined. Rename/undef it before including core_assert.hpp."
    #endif
    #define ASSERT(expr)                                                            \
        do {                                                                        \
          if (!(expr))                                                              \
          {                                                                         \
            ::core::detail::AssertDispatch(CORE_STRINGIFY(expr), nullptr,           \
                                          __FILE__, static_cast<int>(__LINE__));    \
          }                                                                         \
        } while (0)
    #else
        #if defined(ASSERT)
            #error "ASSERT macro is already defined. Rename/undef it before including core_assert.hpp."
        #endif
    #define ASSERT(expr) do { (void)0; } while (0)
#endif

// Runtime assertion with a custom message (active when CORE_ASSERTIONS_ENABLED).
#if CORE_ASSERT_ENABLED
    #if defined(ASSERT_MSG)
        #error "ASSERT_MSG macro is already defined. Rename/undef it before including core_assert.hpp."
    #endif
#define ASSERT_MSG(expr, msg)                                                   \
    do {                                                                        \
      if (!(expr))                                                              \
      {                                                                         \
        ::core::detail::AssertDispatch(CORE_STRINGIFY(expr), (msg),             \
                                      __FILE__, static_cast<int>(__LINE__));    \
      }                                                                         \
    } while (0)
#else
    #if defined(ASSERT_MSG)
        #error "ASSERT_MSG macro is already defined. Rename/undef it before including core_assert.hpp."
    #endif
    #define ASSERT_MSG(expr, msg) do { (void)0; } while (0)
#endif

// Always evaluates expr; asserts only when enabled.
#if CORE_ASSERT_ENABLED
    #if defined(VERIFY)
        #error "VERIFY macro is already defined. Rename/undef it before including core_assert.hpp."
    #endif
#define VERIFY(expr)                                                          \
    do {                                                                        \
      const auto CORE_CONCAT(_verify_result_, __LINE__) = (expr);               \
      if (!CORE_CONCAT(_verify_result_, __LINE__))                              \
      {                                                                         \
        ::core::detail::AssertDispatch(CORE_STRINGIFY(expr), nullptr,           \
                                      __FILE__, static_cast<int>(__LINE__));   \
      }                                                                         \
    } while (0)
#else
    #if defined(VERIFY)
        #error "VERIFY macro is already defined. Rename/undef it before including core_assert.hpp."
    #endif
    #define VERIFY(expr) do { (void)(expr); } while (0)
#endif

// -----------------------------------------------------------------------------
// Optional diagnostic macros (always active, always terminate)
// -----------------------------------------------------------------------------

#if defined(FATAL)
    #error "FATAL macro is already defined. Rename/undef it before including core_assert.hpp."
#endif
#define FATAL(msg)                                                              \
  do {                                                                          \
    ::core::detail::AssertDispatch("FATAL", (msg), __FILE__,                    \
                                  static_cast<int>(__LINE__));                  \
    ::core::FailFast();                                                         \
  } while (0)

#if defined(UNREACHABLE)
    #error "UNREACHABLE macro is already defined. Rename/undef it before including core_assert.hpp."
#endif
#define UNREACHABLE()                                                           \
  do {                                                                          \
    ::core::detail::AssertDispatch("UNREACHABLE", nullptr, __FILE__,            \
                                  static_cast<int>(__LINE__));                  \
    ::core::FailFast();                                                         \
  } while (0)

#if defined(NOT_IMPLEMENTED)
    #error "NOT_IMPLEMENTED macro is already defined. Rename/undef it before including core_assert.hpp."
#endif
#define NOT_IMPLEMENTED()                                                       \
  do {                                                                          \
    ::core::detail::AssertDispatch("NOT_IMPLEMENTED", nullptr, __FILE__,        \
                                  static_cast<int>(__LINE__));                  \
    ::core::FailFast();                                                         \
  } while (0)