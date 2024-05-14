cmake_minimum_required( VERSION 3.12 )

project(sds-proto VERSION 1.0.0 LANGUAGES CXX)

# the main serialization library is header only
add_library(sds-proto INTERFACE)
target_include_directories(sds-proto INTERFACE include)
target_compile_features(sds-proto INTERFACE cxx_std_20)

add_subdirectory(src)

option(SDS_PROTO_BUILD_TEST "Build tests" OFF)
option(SDS_PROTO_BUILD_EXAMPLE "Build examples" OFF)
option(SDS_PROTO_USE_CLANG_FORMAT "Enable clang format for generated code" ON)
option(SDS_PROTO_USE_COVERAGE "Enable code coverage" OFF)
option(SDS_PROTO_USE_ADDRESS_SANITIZER "Enable address sanitizer" OFF)
option(SDS_PROTO_USE_UB_SANITIZER "Enable undefined behavior sanitizer" OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sds_protobuf.cmake)

if(SDS_PROTO_USE_COVERAGE)
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

if(SDS_PROTO_USE_ADDRESS_SANITIZER)
    message("-- Enable address sanitizer")
    add_compile_options("-fsanitize=address")
    add_link_options("-fsanitize=address")
elseif(SDS_PROTO_USE_UB_SANITIZER)
    message("-- Enable undefined behavior sanitizer")
    add_compile_options("-fsanitize=undefined")
    add_link_options("-fsanitize=undefined")
endif()

if(SDS_PROTO_BUILD_EXAMPLE)
    add_subdirectory(example)    
endif()

if(SDS_PROTO_BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()