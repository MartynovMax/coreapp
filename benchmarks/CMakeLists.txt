cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# Build option
# ---------------------------------------------------------------------------

option(BUILD_BENCHMARKS "Build benchmark executables" OFF)

if(NOT BUILD_BENCHMARKS)
    return()
endif()

# ---------------------------------------------------------------------------
# Benchmark executable
# ---------------------------------------------------------------------------

# Automatically collect all sources (exclude tests)
file(GLOB_RECURSE BENCHMARK_SOURCES
    CONFIGURE_DEPENDS
    *.cpp
)
list(FILTER BENCHMARK_SOURCES EXCLUDE REGEX ".*/tests/.*")

# Collect headers for IDE (exclude tests)
file(GLOB_RECURSE BENCHMARK_HEADERS
    CONFIGURE_DEPENDS
    *.hpp
)
list(FILTER BENCHMARK_HEADERS EXCLUDE REGEX ".*/tests/.*")

add_executable(coreapp_benchmarks ${BENCHMARK_SOURCES})

target_link_libraries(coreapp_benchmarks PRIVATE coreapp)

target_include_directories(coreapp_benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# IDE source grouping
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             FILES ${BENCHMARK_HEADERS} ${BENCHMARK_SOURCES})

message(STATUS "Added benchmark target: coreapp_benchmarks")

# ---------------------------------------------------------------------------
# Benchmark tests
# ---------------------------------------------------------------------------

file(GLOB BENCHMARK_TEST_SOURCES tests/*.cpp)

if(BENCHMARK_TEST_SOURCES)
    # Get benchmark sources without main
    set(BENCHMARK_LIB_SOURCES ${BENCHMARK_SOURCES})
    list(REMOVE_ITEM BENCHMARK_LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main_benchmarks.cpp)
    
    add_executable(benchmark_tests 
        ${BENCHMARK_TEST_SOURCES}
        ${BENCHMARK_LIB_SOURCES}
    )
    
    target_link_libraries(benchmark_tests PRIVATE coreapp gtest_main)
    
    target_include_directories(benchmark_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    
    add_test(NAME benchmark_tests COMMAND benchmark_tests)
    
    list(LENGTH BENCHMARK_TEST_SOURCES TEST_COUNT)
    message(STATUS "Added benchmark tests: ${TEST_COUNT} test files")
endif()
