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

# Suppress deprecated warnings in this test (intentionally tests deprecated APIs)
if (MSVC)
    target_compile_options(core_attributes_tests PRIVATE /wd4996)
else()
    target_compile_options(core_attributes_tests PRIVATE -Wno-deprecated-declarations)
endif()

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

# system_allocator tests
add_executable(system_allocator_tests
    tests/core_memory/test_system_allocator.cpp
)
target_link_libraries(system_allocator_tests PRIVATE coreapp gtest_main)
add_test(NAME system_allocator_tests COMMAND system_allocator_tests)

# malloc_allocator tests
add_executable(malloc_allocator_tests
    tests/core_memory/test_malloc_allocator.cpp
)
target_link_libraries(malloc_allocator_tests PRIVATE coreapp gtest_main)
add_test(NAME malloc_allocator_tests COMMAND malloc_allocator_tests)

# allocation_hooks tests
add_executable(allocation_hooks_tests
    tests/core_memory/test_allocation_hooks.cpp
)
target_link_libraries(allocation_hooks_tests PRIVATE coreapp gtest_main)
add_test(NAME allocation_hooks_tests COMMAND allocation_hooks_tests)

# allocation_tracker tests
add_executable(allocation_tracker_tests
    tests/core_memory/test_allocation_tracker.cpp
)
target_link_libraries(allocation_tracker_tests PRIVATE coreapp gtest_main)
add_test(NAME allocation_tracker_tests COMMAND allocation_tracker_tests)

# memory_ops tests
add_executable(memory_ops_tests
    tests/core_memory/test_memory_ops.cpp
)
target_link_libraries(memory_ops_tests PRIVATE coreapp gtest_main)
add_test(NAME memory_ops_tests COMMAND memory_ops_tests)

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

# atomic_u32 tests
add_executable(atomic_u32_tests
    tests/core_concurrency/test_atomic_u32.cpp
)
target_link_libraries(atomic_u32_tests PRIVATE coreapp gtest_main)
add_test(NAME atomic_u32_tests COMMAND atomic_u32_tests)

# atomic_u64 tests
add_executable(atomic_u64_tests
    tests/core_concurrency/test_atomic_u64.cpp
)
target_link_libraries(atomic_u64_tests PRIVATE coreapp gtest_main)
add_test(NAME atomic_u64_tests COMMAND atomic_u64_tests)

# atomic_uintptr tests
add_executable(atomic_uintptr_tests
    tests/core_concurrency/test_atomic_uintptr.cpp
)
target_link_libraries(atomic_uintptr_tests PRIVATE coreapp gtest_main)
add_test(NAME atomic_uintptr_tests COMMAND atomic_uintptr_tests)

# atomic_ptr tests
add_executable(atomic_ptr_tests
    tests/core_concurrency/test_atomic_ptr.cpp
)
target_link_libraries(atomic_ptr_tests PRIVATE coreapp gtest_main)
add_test(NAME atomic_ptr_tests COMMAND atomic_ptr_tests)

# atomic_fence tests
add_executable(atomic_fence_tests
    tests/core_concurrency/test_atomic_fence.cpp
)
target_link_libraries(atomic_fence_tests PRIVATE coreapp gtest_main)
add_test(NAME atomic_fence_tests COMMAND atomic_fence_tests)

# tls_ptr tests
add_executable(tls_ptr_tests
    tests/core_concurrency/test_tls_ptr.cpp
)
target_link_libraries(tls_ptr_tests PRIVATE coreapp gtest_main)
add_test(NAME tls_ptr_tests COMMAND tls_ptr_tests)

# cpu_relax tests
add_executable(cpu_relax_tests
    tests/core_concurrency/test_cpu_relax.cpp
)
target_link_libraries(cpu_relax_tests PRIVATE coreapp gtest_main)
add_test(NAME cpu_relax_tests COMMAND cpu_relax_tests)

# spin_mutex tests
add_executable(spin_mutex_tests
    tests/core_concurrency/test_spin_mutex.cpp
)
target_link_libraries(spin_mutex_tests PRIVATE coreapp gtest_main)
add_test(NAME spin_mutex_tests COMMAND spin_mutex_tests)

# scoped_lock tests
add_executable(scoped_lock_tests
    tests/core_concurrency/test_scoped_lock.cpp
)
target_link_libraries(scoped_lock_tests PRIVATE coreapp gtest_main)
add_test(NAME scoped_lock_tests COMMAND scoped_lock_tests)