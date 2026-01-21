#include "../runner/cli_parser.hpp"
#include "core/base/core_types.hpp"
#include <gtest/gtest.h>

using namespace core;
using namespace core::bench;

// Test --list flag
TEST(CLIParserTest, ListFlag) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--list"};
    
    EXPECT_TRUE(parser.Parse(2, argv, config));
    EXPECT_TRUE(config.showList);
}

// Test --filter flag
TEST(CLIParserTest, FilterFlag) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--filter=arena/*"};
    
    EXPECT_TRUE(parser.Parse(2, argv, config));
    EXPECT_STREQ(config.filter, "arena/*");
}

// Test --seed flag
TEST(CLIParserTest, SeedFlag) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--seed=12345"};
    
    EXPECT_TRUE(parser.Parse(2, argv, config));
    EXPECT_EQ(config.seed, 12345ull);
}

// Test --warmup flag
TEST(CLIParserTest, WarmupFlag) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--warmup=5"};
    
    EXPECT_TRUE(parser.Parse(2, argv, config));
    EXPECT_EQ(config.warmupIterations, 5u);
}

// Test --repetitions flag
TEST(CLIParserTest, RepetitionsFlag) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--repetitions=10"};
    
    EXPECT_TRUE(parser.Parse(2, argv, config));
    EXPECT_EQ(config.measuredRepetitions, 10u);
}

// Test --format flag
TEST(CLIParserTest, FormatFlag) {
    CLIParser parser;
    RunConfig config;
    
    char* argv1[] = {(char*)"prog", (char*)"--format=jsonl"};
    EXPECT_TRUE(parser.Parse(2, argv1, config));
    EXPECT_EQ(config.format, OutputFormat::Jsonl);
    
    char* argv2[] = {(char*)"prog", (char*)"--format=none"};
    EXPECT_TRUE(parser.Parse(2, argv2, config));
    EXPECT_EQ(config.format, OutputFormat::None);
}

// Test --help flag
TEST(CLIParserTest, HelpFlag) {
    CLIParser parser;
    RunConfig config;
    
    char* argv1[] = {(char*)"prog", (char*)"--help"};
    EXPECT_TRUE(parser.Parse(2, argv1, config));
    EXPECT_TRUE(config.showHelp);
    
    char* argv2[] = {(char*)"prog", (char*)"-h"};
    EXPECT_TRUE(parser.Parse(2, argv2, config));
    EXPECT_TRUE(config.showHelp);
}

// Test invalid flag detection
TEST(CLIParserTest, InvalidFlag) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--invalid"};
    
    EXPECT_FALSE(parser.Parse(2, argv, config));
    EXPECT_NE(parser.GetError(), nullptr);
}

// Test invalid flag with prefix similarity (--filterX should be unknown, not --filter)
TEST(CLIParserTest, InvalidFlagSimilarPrefix) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {(char*)"prog", (char*)"--filterX=value"};
    
    EXPECT_FALSE(parser.Parse(2, argv, config));
    EXPECT_STREQ(parser.GetError(), "Unknown flag");
}

// Test missing value detection
TEST(CLIParserTest, MissingValue) {
    CLIParser parser;
    RunConfig config;
    
    char* argv1[] = {(char*)"prog", (char*)"--filter="};
    EXPECT_FALSE(parser.Parse(2, argv1, config));
    
    char* argv2[] = {(char*)"prog", (char*)"--seed="};
    EXPECT_FALSE(parser.Parse(2, argv2, config));
    
    char* argv3[] = {(char*)"prog", (char*)"--warmup="};
    EXPECT_FALSE(parser.Parse(2, argv3, config));
}

// Test multiple flags
TEST(CLIParserTest, MultipleFlags) {
    CLIParser parser;
    RunConfig config;
    char* argv[] = {
        (char*)"prog",
        (char*)"--filter=test*",
        (char*)"--seed=42",
        (char*)"--warmup=3",
        (char*)"--repetitions=5"
    };
    
    EXPECT_TRUE(parser.Parse(5, argv, config));
    EXPECT_STREQ(config.filter, "test*");
    EXPECT_EQ(config.seed, 42ull);
    EXPECT_EQ(config.warmupIterations, 3u);
    EXPECT_EQ(config.measuredRepetitions, 5u);
}
