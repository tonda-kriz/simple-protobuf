include(cmake/doctest.cmake)

FILE(GLOB protos ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

SET(PB_PACKAGE "")
configure_file(proto/scalar/scalar.proto.in ${CMAKE_CURRENT_BINARY_DIR}/scalar.proto @ONLY)
configure_file(proto/person/person.proto.in ${CMAKE_CURRENT_BINARY_DIR}/person.proto @ONLY)
configure_file(proto/name/name.proto.in ${CMAKE_CURRENT_BINARY_DIR}/name.proto @ONLY)

SET(PB_PACKAGE ".gpb")
configure_file(proto/scalar/scalar.proto.in ${CMAKE_CURRENT_BINARY_DIR}/gpb-scalar.proto @ONLY)
configure_file(proto/person/person.proto.in ${CMAKE_CURRENT_BINARY_DIR}/gpb-person.proto @ONLY)
configure_file(proto/name/name.proto.in ${CMAKE_CURRENT_BINARY_DIR}/gpb-name.proto @ONLY)

spb_protobuf_generate(SPB_PROTO_SRCS SPB_PROTO_HDRS ${protos})
spb_protobuf_generate(SPB_PROTO_PERSON_SRC SPB_PROTO_PERSON_HDR ${CMAKE_CURRENT_BINARY_DIR}/person.proto)
spb_protobuf_generate(SPB_PROTO_NAME_SRC SPB_PROTO_NAME_HDR ${CMAKE_CURRENT_BINARY_DIR}/name.proto)
spb_protobuf_generate(SPB_PROTO_SCALAR_SRC SPB_PROTO_SCALAR_HDR ${CMAKE_CURRENT_BINARY_DIR}/scalar.proto)

add_custom_target(unit_tests)

add_executable(base64-test base64.cpp)
spb_enable_warnings(base64-test)
target_link_libraries(base64-test PUBLIC spb-proto)
add_dependencies(unit_tests base64-test)
doctest_discover_tests(base64-test)

add_executable(json-detail-test json-detail.cpp ${SPB_PROTO_PERSON_SRC} ${SPB_PROTO_NAME_SRC})
spb_enable_warnings(json-detail-test)
target_link_libraries(json-detail-test PUBLIC spb-proto)
add_dependencies(unit_tests json-detail-test)
doctest_discover_tests(json-detail-test)

add_executable(pb-detail-test pb-detail.cpp ${SPB_PROTO_PERSON_SRC} ${SPB_PROTO_NAME_SRC} ${SPB_PROTO_SCALAR_SRC})
spb_enable_warnings(pb-detail-test)
target_link_libraries(pb-detail-test PUBLIC spb-proto)
add_dependencies(unit_tests pb-detail-test)
doctest_discover_tests(pb-detail-test)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_ENABLED)

add_executable(compiled-protos main.cpp ${SPB_PROTO_SRCS} ${SPB_PROTO_HDRS})
spb_enable_warnings(compiled-protos)
if(MSVC)
else()
  target_compile_options(compiled-protos PRIVATE -Wno-deprecated-declarations -Wno-deprecated)
endif()
target_link_libraries(compiled-protos PUBLIC spb-proto)
set_target_properties(compiled-protos PROPERTIES INTERPROCEDURAL_OPTIMIZATION IPO_ENABLED)

if(SPB_PROTO_BUILD_COMPATIBILITY_TESTS)
  find_package(Protobuf REQUIRED)

  PROTOBUF_GENERATE_CPP(PROTO_PERSON_SRC PROTO_PERSON_HDR ${CMAKE_CURRENT_BINARY_DIR}/gpb-person.proto)
  PROTOBUF_GENERATE_CPP(PROTO_NAME_SRC PROTO_NAME_HDR ${CMAKE_CURRENT_BINARY_DIR}/gpb-name.proto)
  PROTOBUF_GENERATE_CPP(PROTO_SCALAR_SRC PROTO_SCALAR_HDR ${CMAKE_CURRENT_BINARY_DIR}/gpb-scalar.proto)

  add_library(gpb-proto STATIC ${PROTO_SCALAR_SRC} ${PROTO_PERSON_SRC} ${PROTO_NAME_SRC})

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/etc/arch-release")
    # On Arch Linux protobuf::libprotobuf needs to be linked with absl
    target_link_libraries(gpb-proto PUBLIC protobuf::libprotobuf absl_log_internal_check_op absl_log_internal_message absl_status)
  else()
    target_link_libraries(gpb-proto PUBLIC protobuf::libprotobuf)
  endif()
  target_compile_features(gpb-proto INTERFACE cxx_std_20)

  add_executable(gpb-compatibility-test gpb-compatibility.cpp ${SPB_PROTO_PERSON_SRC} ${SPB_PROTO_NAME_SRC} ${SPB_PROTO_SCALAR_SRC})
  spb_enable_warnings(gpb-compatibility-test)
  target_link_libraries(gpb-compatibility-test PUBLIC spb-proto gpb-proto)
  add_dependencies(unit_tests gpb-compatibility-test)
  doctest_discover_tests(gpb-compatibility-test)
endif()

add_executable(spb-protoc-test parser.cpp 
    ../../src/spb-proto-compiler/parser/parser.cpp 
    ../../src/spb-proto-compiler/io/file.cpp
    ../../src/spb-proto-compiler/ast/ast.cpp
    )
target_include_directories(spb-protoc-test PUBLIC ${CMAKE_SOURCE_DIR}/src/spb-proto-compiler)
target_link_libraries(spb-protoc-test PUBLIC spb-proto)
add_dependencies(spb-protoc-test pb-detail-test)
doctest_discover_tests(spb-protoc-test)


if(SPB_PROTO_BUILD_FUZZER_TESTS)
  if(SPB_PROTO_USE_ADDRESS_SANITIZER)
    add_compile_options("-fsanitize=fuzzer,address" -O1 -g)
    add_link_options("-fsanitize=fuzzer,address")
elseif(SPB_PROTO_USE_UB_SANITIZER)
    message("-- Enable undefined behavior sanitizer" -O1 -g)
    add_compile_options("-fsanitize=fuzzer,undefined")
    add_link_options("-fsanitize=fuzzer,undefined")
endif()

  add_executable(spb-protoc-fuzzer fuzzer/spb-protoc-fuzzer.cpp 
    ../../src/spb-proto-compiler/parser/parser.cpp 
    ../../src/spb-proto-compiler/io/file.cpp
    ../../src/spb-proto-compiler/ast/ast.cpp
    )
  target_include_directories(spb-protoc-fuzzer PUBLIC ${CMAKE_SOURCE_DIR}/src/spb-proto-compiler)
  target_link_libraries(spb-protoc-fuzzer PUBLIC spb-proto)
  target_compile_options(spb-protoc-fuzzer PRIVATE -Wno-deprecated-declarations -Wno-deprecated)

  add_executable(spb-pb-fuzzer fuzzer/spb-pb-fuzzer.cpp 
    ${SPB_PROTO_PERSON_SRC} ${SPB_PROTO_NAME_SRC} ${SPB_PROTO_SCALAR_SRC} ${SPB_PROTO_SRCS}
  )
  target_link_libraries(spb-pb-fuzzer PUBLIC spb-proto)
  target_compile_options(spb-pb-fuzzer PRIVATE -Wno-deprecated-declarations -Wno-deprecated)

  add_executable(spb-json-fuzzer fuzzer/spb-json-fuzzer.cpp 
    ${SPB_PROTO_PERSON_SRC} ${SPB_PROTO_NAME_SRC} ${SPB_PROTO_SCALAR_SRC} ${SPB_PROTO_SRCS}
  )
  target_link_libraries(spb-json-fuzzer PUBLIC spb-proto)
  target_compile_options(spb-json-fuzzer PRIVATE -Wno-deprecated-declarations -Wno-deprecated)
endif()


