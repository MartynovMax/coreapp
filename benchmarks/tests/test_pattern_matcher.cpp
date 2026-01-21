#include "../common/pattern_matcher.hpp"
#include <gtest/gtest.h>

using namespace core::bench;

// Test exact match
TEST(PatternMatcherTest, ExactMatch) {
    EXPECT_TRUE(PatternMatch("test", "test"));
    EXPECT_TRUE(PatternMatch("experiment", "experiment"));
    EXPECT_FALSE(PatternMatch("test", "test2"));
    EXPECT_FALSE(PatternMatch("abc", "def"));
}

// Test wildcard * matching
TEST(PatternMatcherTest, WildcardStar) {
    // Star at end
    EXPECT_TRUE(PatternMatch("test*", "test"));
    EXPECT_TRUE(PatternMatch("test*", "test123"));
    EXPECT_TRUE(PatternMatch("test*", "testfoo"));
    EXPECT_FALSE(PatternMatch("test*", "foo"));

    // Star at beginning
    EXPECT_TRUE(PatternMatch("*test", "test"));
    EXPECT_TRUE(PatternMatch("*test", "mytest"));
    EXPECT_TRUE(PatternMatch("*test", "foobartest"));
    EXPECT_FALSE(PatternMatch("*test", "testing"));

    // Star in middle
    EXPECT_TRUE(PatternMatch("te*st", "test"));
    EXPECT_TRUE(PatternMatch("te*st", "te123st"));
    EXPECT_TRUE(PatternMatch("arena/*", "arena/monotonic"));
    EXPECT_TRUE(PatternMatch("arena/*", "arena/bump"));
    EXPECT_FALSE(PatternMatch("arena/*", "allocator/pool"));

    // Match everything
    EXPECT_TRUE(PatternMatch("*", "anything"));
    EXPECT_TRUE(PatternMatch("*", ""));
}

// Test wildcard ? matching
TEST(PatternMatcherTest, WildcardQuestion) {
    EXPECT_TRUE(PatternMatch("test?", "test1"));
    EXPECT_TRUE(PatternMatch("test?", "testa"));
    EXPECT_FALSE(PatternMatch("test?", "test"));
    EXPECT_FALSE(PatternMatch("test?", "test12"));

    EXPECT_TRUE(PatternMatch("?est", "test"));
    EXPECT_TRUE(PatternMatch("?est", "best"));
    EXPECT_FALSE(PatternMatch("?est", "est"));

    EXPECT_TRUE(PatternMatch("t?st", "test"));
    EXPECT_TRUE(PatternMatch("t?st", "t0st"));
    EXPECT_FALSE(PatternMatch("t?st", "tst"));
}

// Test multiple wildcards
TEST(PatternMatcherTest, MultipleWildcards) {
    EXPECT_TRUE(PatternMatch("*test*", "test"));
    EXPECT_TRUE(PatternMatch("*test*", "mytest"));
    EXPECT_TRUE(PatternMatch("*test*", "testing"));
    EXPECT_TRUE(PatternMatch("*test*", "mytestfoo"));
    EXPECT_FALSE(PatternMatch("*test*", "foo"));

    EXPECT_TRUE(PatternMatch("a*b*c", "abc"));
    EXPECT_TRUE(PatternMatch("a*b*c", "a123b456c"));
    EXPECT_TRUE(PatternMatch("a*b*c", "axbxc"));
    EXPECT_FALSE(PatternMatch("a*b*c", "ab"));

    EXPECT_TRUE(PatternMatch("test?*", "test1"));
    EXPECT_TRUE(PatternMatch("test?*", "test123"));
    EXPECT_FALSE(PatternMatch("test?*", "test"));

    EXPECT_TRUE(PatternMatch("*?", "a"));
    EXPECT_TRUE(PatternMatch("*?", "test"));
    EXPECT_FALSE(PatternMatch("*?", ""));
}

// Test edge cases
TEST(PatternMatcherTest, EdgeCases) {
    EXPECT_TRUE(PatternMatch(nullptr, nullptr));
    EXPECT_FALSE(PatternMatch("test", nullptr));
    EXPECT_FALSE(PatternMatch(nullptr, "test"));
    EXPECT_TRUE(PatternMatch("", ""));
    EXPECT_FALSE(PatternMatch("a", ""));
    EXPECT_FALSE(PatternMatch("", "a"));
}
