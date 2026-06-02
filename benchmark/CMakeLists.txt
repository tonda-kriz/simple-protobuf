add_executable(benchmark-pb benchmark-pb.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET benchmark-pb)
spb_set_compile_options(benchmark-pb)

add_executable(pb-serialize pb-serialize.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET pb-serialize)
spb_set_compile_options(pb-serialize)

add_executable(pb-deserialize pb-deserialize.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET pb-deserialize)
spb_set_compile_options(pb-deserialize)
