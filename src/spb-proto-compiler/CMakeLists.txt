project(spb-proto-compiler VERSION 1.0.0 LANGUAGES CXX)
add_executable(spb-protoc main.cpp parser/parser.cpp ast/ast.cpp io/file.cpp
  dumper/header.cpp dumper/dumper.cpp dumper/json/dumper.cpp dumper/pb/dumper.cpp )
  
target_include_directories(spb-protoc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(spb-protoc PUBLIC spb-proto)
target_compile_options(spb-protoc PUBLIC -Wall -Wpedantic -Werror -Wno-missing-field-initializers)
target_compile_features(spb-protoc PUBLIC cxx_std_20)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
  set_target_properties(spb-protoc PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

