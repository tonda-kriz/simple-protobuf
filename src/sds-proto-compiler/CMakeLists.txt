project(sds-proto-compiler VERSION 1.0.0 LANGUAGES CXX)
add_executable(sds-protoc main.cpp parser/parser.cpp ast/ast.cpp io/file.cpp
  dumper/header.cpp dumper/dumper.cpp dumper/json/dumper.cpp dumper/pb/dumper.cpp )
  
target_include_directories(sds-protoc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sds-protoc PUBLIC sds-proto)
target_compile_options(sds-protoc PUBLIC -Wall -Wpedantic -Werror -Wno-missing-field-initializers)
target_compile_features(sds-protoc PUBLIC cxx_std_20)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
  set_target_properties(sds-protoc PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

