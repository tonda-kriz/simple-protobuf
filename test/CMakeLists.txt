include(cmake/doctest.cmake)

FILE(GLOB protos ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

sds_protobuf_generate(PROTO_SRCS PROTO_HDRS ${protos})
sds_protobuf_generate(PROTO_PERSON_SRC PROTO_PERSON_HDR proto/person/person.proto)
sds_protobuf_generate(PROTO_NAME_SRC PROTO_NAME_HDR proto/name/name.proto)

add_custom_target(unit_tests)

add_executable(base64-test base64.cpp)
target_compile_options(base64-test PUBLIC -Wall -Wpedantic -Werror -Wno-missing-field-initializers)
target_link_libraries(base64-test PUBLIC sds-proto)
add_dependencies(unit_tests base64-test)
doctest_discover_tests(base64-test)

add_executable(json-detail-test json-detail.cpp ${PROTO_PERSON_SRC} ${PROTO_NAME_SRC})
target_compile_options(json-detail-test PUBLIC -Wall -Wpedantic -Werror -Wno-missing-field-initializers)
target_link_libraries(json-detail-test PUBLIC sds-proto)
add_dependencies(unit_tests json-detail-test)
doctest_discover_tests(json-detail-test)

add_executable(compiled-protos main.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_compile_options(compiled-protos PUBLIC -Wno-deprecated-declarations -Wno-deprecated -Wall -Wpedantic -Werror)
target_link_libraries(compiled-protos PUBLIC sds-proto)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
  set_target_properties(compiled-protos PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)  
endif()

