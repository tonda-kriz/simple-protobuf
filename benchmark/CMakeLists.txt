add_executable(benchmark-spb benchmark-spb.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET benchmark-spb)
spb_set_compile_options(benchmark-spb)

add_executable(pb-serialize pb-serialize.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET pb-serialize)
spb_set_compile_options(pb-serialize)

add_executable(pb-deserialize pb-deserialize.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET pb-deserialize)
spb_set_compile_options(pb-deserialize)

add_executable(json-serialize json-serialize.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET json-serialize)
spb_set_compile_options(json-serialize)

add_executable(json-deserialize json-deserialize.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET json-deserialize)
spb_set_compile_options(json-deserialize)
