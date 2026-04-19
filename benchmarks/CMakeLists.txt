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

target_link_libraries(coreapp_benchmarks PRIVATE coreapp nlohmann_json::nlohmann_json)

target_include_directories(coreapp_benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# Environment metadata compile-time definitions
# ---------------------------------------------------------------------------

# Compiler version (human-readable string)
if(CMAKE_CXX_COMPILER_VERSION)
    target_compile_definitions(coreapp_benchmarks PRIVATE
        CORE_COMPILER_VERSION_STRING="${CMAKE_CXX_COMPILER_VERSION}"
    )
endif()

# Build flags (captured at configure time)
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
set(BUILD_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}}")
string(STRIP "${BUILD_FLAGS}" BUILD_FLAGS)
if(BUILD_FLAGS)
    # Escape quotes for proper inclusion in C++ string
    string(REPLACE "\"" "\\\"" BUILD_FLAGS_ESCAPED "${BUILD_FLAGS}")
    target_compile_definitions(coreapp_benchmarks PRIVATE
        CORE_BUILD_FLAGS_CMAKE="${BUILD_FLAGS_ESCAPED}"
    )
endif()

# OS version at build time (optional, may differ from runtime)
cmake_host_system_information(RESULT OS_RELEASE QUERY OS_RELEASE)
if(OS_RELEASE)
    target_compile_definitions(coreapp_benchmarks PRIVATE
        CORE_OS_VERSION_BUILD="${OS_RELEASE}"
    )
endif()

# CPU architecture details
if(CMAKE_SYSTEM_PROCESSOR)
    target_compile_definitions(coreapp_benchmarks PRIVATE
        CORE_CPU_ARCH_CMAKE="${CMAKE_SYSTEM_PROCESSOR}"
    )
endif()

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
    
    target_link_libraries(benchmark_tests PRIVATE coreapp gtest_main nlohmann_json::nlohmann_json)

    target_include_directories(benchmark_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Provide absolute path to the config directory so tests can load JSON fixtures.
    target_compile_definitions(benchmark_tests PRIVATE
        BENCHMARK_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/config"
    )

    # Apply same metadata definitions to tests
    if(CMAKE_CXX_COMPILER_VERSION)
        target_compile_definitions(benchmark_tests PRIVATE
            CORE_COMPILER_VERSION_STRING="${CMAKE_CXX_COMPILER_VERSION}"
        )
    endif()
    
    add_test(NAME benchmark_tests COMMAND benchmark_tests)
    
    list(LENGTH BENCHMARK_TEST_SOURCES TEST_COUNT)
    message(STATUS "Added benchmark tests: ${TEST_COUNT} test files")
endif()
