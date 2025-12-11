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

file(GLOB_RECURSE COREAPP_HEADERS
    CONFIGURE_DEPENDS
    core/*.hpp
)

target_sources(coreapp INTERFACE
    ${COREAPP_HEADERS}>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/core
             FILES ${COREAPP_HEADERS})
		
# ---------------------------------------------------------------------------
# GoogleTest
# ---------------------------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

enable_testing()

add_executable(core_config_tests
    tests/core_base/test_core_config.cpp
)

# Test target uses the same includes as the core library
target_link_libraries(core_config_tests PRIVATE coreapp gtest_main)

# Optional: integrate with CTest
add_test(NAME core_config_tests COMMAND core_config_tests)

# core_types tests
add_executable(core_types_tests
	tests/core_base/test_core_types.cpp
)
target_link_libraries(core_types_tests PRIVATE coreapp gtest_main)
add_test(NAME core_types_tests COMMAND core_types_tests)
