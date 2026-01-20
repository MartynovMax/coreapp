cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# Benchmark executable
# ---------------------------------------------------------------------------

# Automatically collect all sources
file(GLOB_RECURSE BENCHMARK_SOURCES
    CONFIGURE_DEPENDS
    *.cpp
)

# Collect headers for IDE
file(GLOB_RECURSE BENCHMARK_HEADERS
    CONFIGURE_DEPENDS
    *.hpp
)

add_executable(coreapp_benchmarks ${BENCHMARK_SOURCES})

target_link_libraries(coreapp_benchmarks PRIVATE coreapp)

target_include_directories(coreapp_benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# IDE source grouping
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             FILES ${BENCHMARK_HEADERS} ${BENCHMARK_SOURCES})

message(STATUS "Added benchmark target: coreapp_benchmarks")
