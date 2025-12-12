add_executable(addressbook addressbook.cpp proto/addressbook.proto)
spb_protobuf_generate(TARGET addressbook)

Include(FetchContent)

FetchContent_Declare(
  etl
  GIT_REPOSITORY https://github.com/ETLCPP/etl
  GIT_TAG        "20.39.4"
)

FetchContent_MakeAvailable(etl)

add_executable(etl-example etl.cpp)
spb_protobuf_generate(TARGET etl-example PROTOS proto/etl.proto)
target_link_libraries(etl-example PUBLIC etl::etl)
spb_set_compile_options(etl-example)
