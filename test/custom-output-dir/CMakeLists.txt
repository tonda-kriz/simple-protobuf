add_executable(addressbook_test)

target_sources(addressbook_test
    PRIVATE
        src/addressbook.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            include/
            proto/
    FILES
        include/openapi.h
    )
    
spb_protobuf_generate(TARGET addressbook_test PROTOS src/proto/addressbook.proto APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
