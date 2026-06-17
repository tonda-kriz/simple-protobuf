add_library(spb-common STATIC spb/common.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET spb-common)
spb_set_compile_options(spb-common)

add_executable(spb-benchmark spb/benchmark.cpp)
target_link_libraries(spb-benchmark PUBLIC spb-common)
spb_set_compile_options(spb-benchmark)

add_executable(spb-pb-serialize spb/pb-serialize.cpp)
target_link_libraries(spb-pb-serialize PUBLIC spb-common)
spb_set_compile_options(spb-pb-serialize)

add_executable(spb-pb-deserialize spb/pb-deserialize.cpp)
target_link_libraries(spb-pb-deserialize PUBLIC spb-common)
spb_set_compile_options(spb-pb-deserialize)

add_executable(spb-json-serialize spb/json-serialize.cpp)
target_link_libraries(spb-json-serialize PUBLIC spb-common)
spb_set_compile_options(spb-json-serialize)

add_executable(spb-json-deserialize spb/json-deserialize.cpp)
target_link_libraries(spb-json-deserialize PUBLIC spb-common)
spb_set_compile_options(spb-json-deserialize)

add_executable(spb-utf8 spb/utf8.cpp)
target_link_libraries(spb-utf8 PUBLIC spb::proto)
spb_set_compile_options(spb-utf8)

PROTOBUF_GENERATE_CPP(PROTO_BOOK_SRC PROTO_BOOK_HDR ${CMAKE_CURRENT_LIST_DIR}/proto/addressbook.proto)
add_library(gpb-common STATIC gpb/common.cpp ${PROTO_BOOK_SRC})
target_link_libraries(gpb-common PUBLIC protobuf::libprotobuf)
spb_set_compile_options(gpb-common)

PROTOBUF_GENERATE_CPP(PROTO_BOOK_LITE_SRC PROTO_BOOK_LITE_HDR ${CMAKE_CURRENT_LIST_DIR}/proto/addressbook-lite.proto)
add_library(gpb-lite-common STATIC gpb/common.cpp ${PROTO_BOOK_LITE_SRC})
target_compile_definitions(gpb-lite-common PUBLIC GPB_LITE=1)
target_link_libraries(gpb-lite-common PUBLIC protobuf::libprotobuf-lite)
spb_set_compile_options(gpb-lite-common)

add_executable(gpb-benchmark gpb/benchmark.cpp)
spb_set_compile_options(gpb-benchmark)
target_link_libraries(gpb-benchmark PUBLIC gpb-common)

add_executable(gpb-lite-benchmark gpb/benchmark.cpp)
spb_set_compile_options(gpb-lite-benchmark)
target_link_libraries(gpb-lite-benchmark PUBLIC gpb-lite-common)

add_executable(gpb-pb-serialize gpb/pb-serialize.cpp)
spb_set_compile_options(gpb-pb-serialize)
target_link_libraries(gpb-pb-serialize PUBLIC gpb-common)

add_executable(gpb-lite-pb-serialize gpb/pb-serialize.cpp)
spb_set_compile_options(gpb-lite-pb-serialize)
target_link_libraries(gpb-lite-pb-serialize PUBLIC gpb-lite-common)

add_executable(gpb-pb-deserialize gpb/pb-deserialize.cpp)
spb_set_compile_options(gpb-pb-deserialize)
target_link_libraries(gpb-pb-deserialize PUBLIC gpb-common)

add_executable(gpb-lite-pb-deserialize gpb/pb-deserialize.cpp)
spb_set_compile_options(gpb-lite-pb-deserialize)
target_link_libraries(gpb-lite-pb-deserialize PUBLIC gpb-lite-common)

add_executable(gpb-json-serialize gpb/json-serialize.cpp)
target_link_libraries(gpb-json-serialize PUBLIC gpb-common)
spb_set_compile_options(gpb-json-serialize)

add_executable(gpb-json-deserialize gpb/json-deserialize.cpp)
target_link_libraries(gpb-json-deserialize PUBLIC gpb-common)
spb_set_compile_options(gpb-json-deserialize)
