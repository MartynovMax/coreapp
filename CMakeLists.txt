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

# IDE-friendly header-only target (does not affect build)
add_custom_target(coreapp_headers SOURCES ${COREAPP_HEADERS})

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

# core_config tests
add_executable(core_config_tests
    tests/core_base/test_core_config.cpp
)
target_link_libraries(core_config_tests PRIVATE coreapp gtest_main)
add_test(NAME core_config_tests COMMAND core_config_tests)

# core_macros tests
add_executable(core_macros_tests
    tests/core_base/test_core_macros.cpp
)
target_link_libraries(core_macros_tests PRIVATE coreapp gtest_main)
add_test(NAME core_macros_tests COMMAND core_macros_tests)

# core_types tests
add_executable(core_types_tests
    tests/core_base/test_core_types.cpp
)
target_link_libraries(core_types_tests PRIVATE coreapp gtest_main)
add_test(NAME core_types_tests COMMAND core_types_tests)

# core_assert tests
add_executable(core_assert_tests
    tests/core_base/test_core_assert.cpp
)
target_link_libraries(core_assert_tests PRIVATE coreapp gtest_main)
add_test(NAME core_assert_tests COMMAND core_assert_tests)

# core_attributes tests (compile + run)
add_executable(core_attributes_tests
    tests/core_base/test_core_attributes.cpp
)
target_link_libraries(core_attributes_tests PRIVATE coreapp gtest_main)
add_test(NAME core_attributes_tests COMMAND core_attributes_tests)

# core_defs tests
add_executable(core_defs_tests
    tests/core_base/test_core_defs.cpp
)
target_link_libraries(core_defs_tests PRIVATE coreapp gtest_main)
add_test(NAME core_defs_tests COMMAND core_defs_tests)

# core_platform tests
add_executable(core_platform_tests
    tests/core_base/test_core_platform.cpp
)
target_link_libraries(core_platform_tests PRIVATE coreapp gtest_main)
add_test(NAME core_platform_tests COMMAND core_platform_tests)

# core_build tests
add_executable(core_build_tests
    tests/core_base/test_core_build.cpp
)
target_link_libraries(core_build_tests PRIVATE coreapp gtest_main)
add_test(NAME core_build_tests COMMAND core_build_tests)

# core_preprocessor tests
add_executable(core_preprocessor_tests
    tests/core_base/test_core_preprocessor.cpp
)
target_link_libraries(core_preprocessor_tests PRIVATE coreapp gtest_main)
add_test(NAME core_preprocessor_tests COMMAND core_preprocessor_tests)

# core_features tests
add_executable(core_features_tests
    tests/core_base/test_core_features.cpp
)
target_link_libraries(core_features_tests PRIVATE coreapp gtest_main)
add_test(NAME core_features_tests COMMAND core_features_tests)

# core_inline tests
add_executable(core_inline_tests
    tests/core_base/test_core_inline.cpp
)
target_link_libraries(core_inline_tests PRIVATE coreapp gtest_main)
add_test(NAME core_inline_tests COMMAND core_inline_tests)

# core_api tests (compile-only variants via compile definitions)
add_executable(core_api_tests_static
    tests/core_base/test_core_api.cpp
)
target_link_libraries(core_api_tests_static PRIVATE coreapp gtest_main)
target_compile_definitions(core_api_tests_static PRIVATE CORE_BUILD_STATIC=1)
add_test(NAME core_api_tests_static COMMAND core_api_tests_static)

add_executable(core_api_tests_shared_producer
    tests/core_base/test_core_api.cpp
)
target_link_libraries(core_api_tests_shared_producer PRIVATE coreapp gtest_main)
target_compile_definitions(core_api_tests_shared_producer PRIVATE
    CORE_BUILD_SHARED=1
    CORE_BUILD_CORE_LIBRARY=1
)
add_test(NAME core_api_tests_shared_producer COMMAND core_api_tests_shared_producer)

add_library(core_api_tests_shared_consumer OBJECT
    tests/core_base/test_core_api_consumer.cpp
)
target_link_libraries(core_api_tests_shared_consumer PRIVATE coreapp)
target_compile_definitions(core_api_tests_shared_consumer PRIVATE
    CORE_BUILD_SHARED=1
)

# Make sure consumer compile-only target is built in "Build All"/CI builds.
add_custom_target(core_api_consumer_compile ALL
    DEPENDS core_api_tests_shared_consumer
)

# core_static_init tests
add_executable(core_static_init_tests
    tests/core_base/test_core_static_init.cpp
)
target_link_libraries(core_static_init_tests PRIVATE coreapp gtest_main)
add_test(NAME core_static_init_tests COMMAND core_static_init_tests)

# core_version tests
add_executable(core_version_tests
    tests/core_base/test_core_version.cpp
)
target_link_libraries(core_version_tests PRIVATE coreapp gtest_main)
add_test(NAME core_version_tests COMMAND core_version_tests)

# core_memory tests
add_executable(core_memory_tests
    tests/core_memory/test_core_memory.cpp
)
target_link_libraries(core_memory_tests PRIVATE coreapp gtest_main)
add_test(NAME core_memory_tests COMMAND core_memory_tests)

# memory_traits tests
add_executable(memory_traits_tests
    tests/core_memory/test_memory_traits.cpp
)
target_link_libraries(memory_traits_tests PRIVATE coreapp gtest_main)
add_test(NAME memory_traits_tests COMMAND memory_traits_tests)

# core_allocator tests
add_executable(core_allocator_tests
    tests/core_memory/test_core_allocator.cpp
)
target_link_libraries(core_allocator_tests PRIVATE coreapp gtest_main)
add_test(NAME core_allocator_tests COMMAND core_allocator_tests)

# core_attributes warnings (build-only, excluded from ALL)
add_executable(core_attributes_warnings_tests EXCLUDE_FROM_ALL
    tests/core_base/test_core_attributes_warnings.cpp
)
target_link_libraries(core_attributes_warnings_tests PRIVATE coreapp gtest_main)

# Enable useful warnings on this target (do NOT make them errors).
if (MSVC)
    target_compile_options(core_attributes_warnings_tests PRIVATE /W4 /WX-)
else()
    target_compile_options(core_attributes_warnings_tests PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-error
        -Wno-error=deprecated-declarations
        -Wno-error=unused-result
    )
endif()
