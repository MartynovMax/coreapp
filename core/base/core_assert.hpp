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
#define ASSERT(expr)                                                          \
    do {                                                                        \
      if (!(expr))                                                              \
      {                                                                         \
        ::core::detail::AssertDispatch(CORE_STRINGIFY(expr), nullptr,           \
                                      __FILE__, static_cast<int>(__LINE__));   \
      }                                                                         \
    } while (0)
#else
#if defined(ASSERT)
#error "ASSERT macro is already defined. Rename/undef it before including core_assert.hpp."
#endif
#define ASSERT(expr) do { (void)0; } while (0)
#endif