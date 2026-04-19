cmake_minimum_required(VERSION 3.20)
project(coreapp VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------

# Automatically collect all .cpp files from core/ directory
file(GLOB_RECURSE COREAPP_SOURCES
    CONFIGURE_DEPENDS
    core/**/*.cpp
)

# Collect all headers for IDE display
file(GLOB_RECURSE COREAPP_HEADERS
    CONFIGURE_DEPENDS
    core/**/*.hpp
)

# Core library with both headers and implementation files
add_library(coreapp STATIC
    ${COREAPP_SOURCES}
)

# Expose project root as include directory so that
#   #include "core/base/core_config.hpp"
# works from any target that links to coreapp.
target_include_directories(coreapp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# IDE-friendly header target
add_custom_target(coreapp_headers SOURCES ${COREAPP_HEADERS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/core
             FILES ${COREAPP_HEADERS} ${COREAPP_SOURCES})

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------

add_subdirectory(benchmarks)
		
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
# nlohmann/json  (header-only, used by benchmarks scenario loader)
# ---------------------------------------------------------------------------

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

enable_testing()

# Helper function to automatically add test suite for a module
# Usage: add_module_test_suite(core_base "excluded_pattern1" "excluded_pattern2")
function(add_module_test_suite MODULE_NAME)
    file(GLOB MODULE_TESTS "tests/${MODULE_NAME}/test_*.cpp")
    
    # Exclude special cases passed as additional arguments
    foreach(PATTERN ${ARGN})
        list(FILTER MODULE_TESTS EXCLUDE REGEX ${PATTERN})
    endforeach()
    
    if(MODULE_TESTS)
        add_executable(${MODULE_NAME}_tests ${MODULE_TESTS})
        target_link_libraries(${MODULE_NAME}_tests PRIVATE coreapp gtest_main)
        add_test(NAME ${MODULE_NAME}_tests COMMAND ${MODULE_NAME}_tests)
        
        list(LENGTH MODULE_TESTS TEST_COUNT)
        message(STATUS "Added test suite: ${MODULE_NAME}_tests (${TEST_COUNT} test files)")
    endif()
endfunction()

# Automatically add all test suites (auto-discovers test_*.cpp files)
add_module_test_suite(core_base "test_core_attributes_warnings" "test_core_api" "test_core_attributes")
add_module_test_suite(core_memory)
add_module_test_suite(core_concurrency)

# ---------------------------------------------------------------------------
# Special test executables with custom configurations
# ---------------------------------------------------------------------------

# core_attributes tests - suppress deprecated warnings (intentionally tests deprecated APIs)
file(GLOB CORE_ATTRIBUTES_TEST "tests/core_base/test_core_attributes.cpp")
if(CORE_ATTRIBUTES_TEST)
    add_executable(core_attributes_tests ${CORE_ATTRIBUTES_TEST})
    target_link_libraries(core_attributes_tests PRIVATE coreapp gtest_main)
    
    if (MSVC)
        target_compile_options(core_attributes_tests PRIVATE /wd4996)
    else()
        target_compile_options(core_attributes_tests PRIVATE -Wno-deprecated-declarations)
    endif()
    
    add_test(NAME core_attributes_tests COMMAND core_attributes_tests)
endif()

# core_api tests - multiple variants with different compile definitions
file(GLOB CORE_API_TEST "tests/core_base/test_core_api.cpp")
if(CORE_API_TEST)
    # Static variant
    add_executable(core_api_tests_static ${CORE_API_TEST})
    target_link_libraries(core_api_tests_static PRIVATE coreapp gtest_main)
    target_compile_definitions(core_api_tests_static PRIVATE CORE_BUILD_STATIC=1)
    add_test(NAME core_api_tests_static COMMAND core_api_tests_static)

    # Shared producer variant
    add_executable(core_api_tests_shared_producer ${CORE_API_TEST})
    target_link_libraries(core_api_tests_shared_producer PRIVATE coreapp gtest_main)
    target_compile_definitions(core_api_tests_shared_producer PRIVATE
        CORE_BUILD_SHARED=1
        CORE_BUILD_CORE_LIBRARY=1
    )
    add_test(NAME core_api_tests_shared_producer COMMAND core_api_tests_shared_producer)

    # Shared consumer variant (compile-only)
    file(GLOB CORE_API_CONSUMER "tests/core_base/test_core_api_consumer.cpp")
    if(CORE_API_CONSUMER)
        add_library(core_api_tests_shared_consumer OBJECT ${CORE_API_CONSUMER})
        target_link_libraries(core_api_tests_shared_consumer PRIVATE coreapp)
        target_compile_definitions(core_api_tests_shared_consumer PRIVATE CORE_BUILD_SHARED=1)
        
        # Make sure consumer compile-only target is built in "Build All"/CI builds
        add_custom_target(core_api_consumer_compile ALL
            DEPENDS core_api_tests_shared_consumer
        )
    endif()
endif()

# core_attributes warnings test - build-only, excluded from ALL
file(GLOB CORE_ATTRIBUTES_WARNINGS "tests/core_base/test_core_attributes_warnings.cpp")
if(CORE_ATTRIBUTES_WARNINGS)
    add_executable(core_attributes_warnings_tests EXCLUDE_FROM_ALL ${CORE_ATTRIBUTES_WARNINGS})
    target_link_libraries(core_attributes_warnings_tests PRIVATE coreapp gtest_main)

    # Enable useful warnings on this target (do NOT make them errors)
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
endif()