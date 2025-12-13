#include "core/base/core_assert.hpp"

#include <cstring>  // std::strstr
#include <string>

// NOTE: Tests intentionally use the standard library.
// Core headers remain STL-free.

namespace
{
    struct Record
    {
        int calls = 0;
        const char* expr = nullptr;
        const char* msg  = nullptr;
        const char* file = nullptr;
        int line = 0;
    };

    static Record g_rec{};

    void ResetRecord()
    {
        g_rec = {};
    }

    void RecordingHandler(const char* expr_text,
                          const char* msg,
                          const char* file,
                          int line)
    {
        ++g_rec.calls;
        g_rec.expr = expr_text;
        g_rec.msg  = msg;
        g_rec.file = file;
        g_rec.line = line;
        // Do NOT terminate. This allows testing in-process.
    }

    static int g_side_effect = 0;

    bool SideEffectFalse()
    {
        ++g_side_effect;
        return false;
    }

    bool SideEffectTrue()
    {
        ++g_side_effect;
        return true;
    }

    bool FileContainsTestName(const char* file)
    {
        if (!file) return false;
        return std::strstr(file, "test_core_assert") != nullptr;
    }

    // Tiny test helpers (no framework assumed)
    void ExpectTrue(bool v, const char* msg)
    {
        if (!v)
        {
            FATAL(msg);
        }
    }

    template <class T>
    void ExpectEq(const T& a, const T& b, const char* msg)
    {
        if (!(a == b))
        {
            FATAL(msg);
        }
    }
}

int main()
{
    // Install recording handler for the duration of this test executable.
    core::AssertHandler prev = core::SetAssertHandler(&RecordingHandler);
    (void)prev;

    // 1) Correct assertion triggering in Debug (only when enabled).
#if CORE_ASSERTIONS_ENABLED
    {
        ResetRecord();
        const int expectedLine = __LINE__ + 1;
        ASSERT(false);

        ExpectEq(g_rec.calls, 1, "ASSERT(false) must dispatch exactly once");
        ExpectEq(std::string(g_rec.expr), std::string("false"), "expr_text mismatch for ASSERT(false)");
        ExpectTrue(g_rec.msg == nullptr, "msg must be null for ASSERT(expr)");
        ExpectTrue(FileContainsTestName(g_rec.file), "file must contain test name");
        ExpectEq(g_rec.line, expectedLine, "line mismatch for ASSERT(false)");
    }

    // Forwarding of message
    {
        ResetRecord();
        const int expectedLine = __LINE__ + 1;
        ASSERT_MSG(false, "boom");

        ExpectEq(g_rec.calls, 1, "ASSERT_MSG(false, ...) must dispatch exactly once");
        ExpectEq(std::string(g_rec.expr), std::string("false"), "expr_text mismatch for ASSERT_MSG(false, ...)");
        ExpectTrue(g_rec.msg != nullptr, "msg must be non-null for ASSERT_MSG");
        ExpectEq(std::string(g_rec.msg), std::string("boom"), "msg mismatch for ASSERT_MSG");
        ExpectTrue(FileContainsTestName(g_rec.file), "file must contain test name");
        ExpectEq(g_rec.line, expectedLine, "line mismatch for ASSERT_MSG");
    }

    // Expression stringification (non-trivial)
    {
        ResetRecord();
        const int expectedLine = __LINE__ + 1;
        ASSERT(1 + 2 == 4);

        ExpectEq(g_rec.calls, 1, "ASSERT(1+2==4) must dispatch exactly once");
        ExpectEq(std::string(g_rec.expr), std::string("1 + 2 == 4"), "expr_text mismatch for ASSERT(1+2==4)");
        ExpectEq(g_rec.line, expectedLine, "line mismatch for ASSERT(1+2==4)");
    }
#endif

    // 2) Assertions compiling out in Release: ASSERT must not evaluate expression when disabled.
#if !CORE_ASSERTIONS_ENABLED
    {
        ResetRecord();
        g_side_effect = 0;
        ASSERT(SideEffectFalse());
        ExpectEq(g_side_effect, 0, "ASSERT must not evaluate expression when assertions are disabled");
        ExpectEq(g_rec.calls, 0, "ASSERT must not dispatch when assertions are disabled");
    }

    {
        ResetRecord();
        g_side_effect = 0;
        ASSERT_MSG(SideEffectFalse(), "x");
        ExpectEq(g_side_effect, 0, "ASSERT_MSG must not evaluate expression when assertions are disabled");
        ExpectEq(g_rec.calls, 0, "ASSERT_MSG must not dispatch when assertions are disabled");
    }
#endif

    // 3) VERIFY always evaluates expression (both modes)
    {
        ResetRecord();
        g_side_effect = 0;
        VERIFY(SideEffectTrue());
        ExpectEq(g_side_effect, 1, "VERIFY must evaluate expression (true case)");
        ExpectEq(g_rec.calls, 0, "VERIFY(true) must not dispatch");
    }

#if CORE_ASSERTIONS_ENABLED
    {
        ResetRecord();
        g_side_effect = 0;
        VERIFY(SideEffectFalse());
        ExpectEq(g_side_effect, 1, "VERIFY must evaluate expression (false case)");
        ExpectEq(g_rec.calls, 1, "VERIFY(false) must dispatch when assertions are enabled");
    }
#else
    {
        ResetRecord();
        g_side_effect = 0;
        VERIFY(SideEffectFalse());
        ExpectEq(g_side_effect, 1, "VERIFY must evaluate expression (false case, assertions disabled)");
        ExpectEq(g_rec.calls, 0, "VERIFY(false) must not dispatch when assertions are disabled");
    }
#endif

    // 4) Custom handler installation
    {
        ResetRecord();
        core::AssertHandler old = core::SetAssertHandler(&RecordingHandler);
        ExpectTrue(old != nullptr, "SetAssertHandler must return a previous handler");

#if CORE_ASSERTIONS_ENABLED
        ASSERT(false);
        ExpectEq(g_rec.calls, 1, "custom handler must receive ASSERT dispatch");
#endif

        // Setting null must restore default handler (non-null stored).
        core::SetAssertHandler(nullptr);
        ExpectTrue(core::GetAssertHandler() != nullptr, "GetAssertHandler must be non-null after setting nullptr");

        // Restore recording handler for remaining tests.
        core::SetAssertHandler(&RecordingHandler);
    }

    // Optional diagnostics: must dispatch unconditionally (both modes).
    {
        ResetRecord();
        const int expectedLine = __LINE__ + 1;
        FATAL("fatal");
        ExpectEq(g_rec.calls, 1, "FATAL must dispatch");
        ExpectEq(std::string(g_rec.expr), std::string("FATAL"), "FATAL expr_text mismatch");
        ExpectEq(std::string(g_rec.msg), std::string("fatal"), "FATAL msg mismatch");
        ExpectEq(g_rec.line, expectedLine, "FATAL line mismatch");
    }

    {
        ResetRecord();
        UNREACHABLE();
        ExpectEq(g_rec.calls, 1, "UNREACHABLE must dispatch");
        ExpectEq(std::string(g_rec.expr), std::string("UNREACHABLE"), "UNREACHABLE expr_text mismatch");
        ExpectTrue(g_rec.msg == nullptr, "UNREACHABLE msg must be null");
    }

    {
        ResetRecord();
        NOT_IMPLEMENTED();
        ExpectEq(g_rec.calls, 1, "NOT_IMPLEMENTED must dispatch");
        ExpectEq(std::string(g_rec.expr), std::string("NOT_IMPLEMENTED"), "NOT_IMPLEMENTED expr_text mismatch");
        ExpectTrue(g_rec.msg == nullptr, "NOT_IMPLEMENTED msg must be null");
    }

    // Restore previous handler (best effort)
    core::SetAssertHandler(prev);

    return 0;
}
