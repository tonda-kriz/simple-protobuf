spb_protobuf_generate(PROTO_SRCS PROTO_HDRS proto/addressbook.proto)

add_executable(addressbook addressbook.cpp ${PROTO_SRCS})
target_link_libraries(addressbook PUBLIC spb-proto)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
  set_target_properties(addressbook PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
