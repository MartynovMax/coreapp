cmake_minimum_required(VERSION 3.20)
project(coreapp VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Core interface library
# ---------------------------------------------------------------------------

# Header-only core library for now
add_library(coreapp INTERFACE)

# Expose project root as include directory so that
#   #include "core/base/core_config.hpp"
# works from any target that links to coreapp.
target_include_directories(coreapp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

add_executable(core_config_tests
    tests/core_base/test_core_config.cpp
)

# Test target uses the same includes as the core library
target_link_libraries(core_config_tests PRIVATE coreapp)

# Optional: integrate with CTest
enable_testing()
add_test(NAME core_config_tests COMMAND core_config_tests)
