cmake_minimum_required( VERSION 3.20 )

project(spb-proto VERSION 1.0.0 LANGUAGES CXX)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# the main serialization library is header only
add_library(spb-proto INTERFACE)
target_include_directories(spb-proto INTERFACE include)
target_compile_features(spb-proto INTERFACE cxx_std_20)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/spb_protobuf.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/spb_warnings.cmake)

add_subdirectory(src)

option(SPB_PROTO_BUILD_TESTS "Build tests" OFF)
option(SPB_PROTO_BUILD_COMPATIBILITY_TESTS "Build gpb compatibility tests" OFF)
option(SPB_PROTO_BUILD_ETL_TESTS "Build etl compatibility tests" OFF)
option(SPB_PROTO_BUILD_FUZZER_TESTS "Build fuzzers for spb-protoc, json-parser, gpb-parser" OFF)
option(SPB_PROTO_BUILD_EXAMPLES "Build examples" OFF)
option(SPB_PROTO_USE_CLANG_FORMAT "Enable clang format for generated code" ON)
option(SPB_PROTO_USE_COVERAGE "Enable code coverage" OFF)
option(SPB_PROTO_USE_ADDRESS_SANITIZER "Enable address sanitizer" OFF)
option(SPB_PROTO_USE_UB_SANITIZER "Enable undefined behavior sanitizer" OFF)

#[[ 
  You may specify the output directory for auto-generated files either
  as an absolute path or relative to CMAKE_SOURCE_DIR.
  If nothing is set, files will be generated in CMAKE_CURRENT_BINARY_DIR.
]]
set(SPB_PROTO_OUTPUT_DIR "" CACHE STRING "Output directory for auto-generated files")

if(SPB_PROTO_USE_COVERAGE)
    find_program(GCOV gcov)
    if (NOT GCOV)
        message(FATAL "program gcov not found")
    endif()

    find_program(LCOV lcov)
    if (NOT LCOV)
        message(FATAL "program lcov not found")
    endif()

    find_program(GENHTML genhtml)
    if (NOT GENHTML)
        message(FATAL "program genhtml not found")
    endif()

    add_compile_options(-fprofile-arcs -ftest-coverage)
    add_link_options(-fprofile-arcs -ftest-coverage)
    
    message("-- Enable code coverage")
endif()

if(SPB_PROTO_USE_ADDRESS_SANITIZER)
    message("-- Enable address sanitizer")
    add_compile_options("-fsanitize=address")
    add_link_options("-fsanitize=address")
endif()

if(SPB_PROTO_USE_UB_SANITIZER)
    message("-- Enable undefined behavior sanitizer")
    add_compile_options("-fsanitize=undefined")
    add_link_options("-fsanitize=undefined")
endif()

if(SPB_PROTO_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(SPB_PROTO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()