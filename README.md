# sds-proto

An alternative implementation of [protobuf](https://github.com/protocolbuffers/protobuf) for C++. This is not an plugin for `protoc` but an **replacement** for `protoc`, so you don't need `protobuf` or `protoc` installed to use it.

**SDS** means **simple data struct** so

```proto
package PhoneBook;

message Person {
  string name = 1;
  int32 id = 2;  // Unique ID number for this person.
  string email = 3;

  enum PhoneType {
    MOBILE = 0;
    HOME = 1;
    WORK = 2;
  }

  message PhoneNumber {
    required string number = 1; // phone number is always required
    PhoneType type = 2;
  }
  // all registered phones
  repeated PhoneNumber phones = 4;
}
```

gets compiled with `sds-protoc` into

```C++
namespace PhoneBook
{
struct Person {
    enum class PhoneType : int32_t {
        MOBILE = 0,
        HOME   = 1,
        WORK   = 2,
    };
    struct PhoneNumber {
        // phone number is always required
        std::string number;
        std::optional< PhoneType > type;
    };
    std::optional< std::string > name;
    // Unique ID number for this person.
    std::optional< int32_t > id;
    std::optional< std::string > email;
    // all registered phones
    std::vector< PhoneNumber > phones;
};
}// namespace PhoneBook
```

which can be de/serialized using

```CPP
#include <person.pb.h>

auto john = PhoneBook::Person{
        .name  = "John Doe",
        .id    = 1234,
        .email = "jdoe@example.com",
    };
auto json   = sds::json::serialize( john );
auto person = sds::json::deserialize< PhoneBook::Person >( json );
//- person is now john
```

## Usage

Use this repo as an git submodule

```cmake
# add this repo to your project
add_subdirectory(external/sds-proto)
# compile proto files to C++
sds_protobuf_generate(PROTO_SRCS PROTO_HDRS ${CMAKE_SOURCE_DIR}/proto/person.proto)
# add generated files to your project
add_executable(myapp ${PROTO_SRCS} ${PROTO_HDRS})
# `sds-proto` is an interface library
# the main purpose is to update include path of `myapp` 
target_link_libraries(myapp PUBLIC sds-proto)
```

```C++
//- use generated files
#include <person.pb.h>
...
```

## status

project is in an early stage of development, expect bugs, API changes and definitely do **NOT** use it in production, *yet*.

- [x] Make it work
- [ ] Make it right
- [ ] Make it fast

### roadmap

- [x] parser for proto files (supported syntax: `proto2` and `proto3`)
- [x] compile proto message to C++ data struct
- [x] generate json de/serializer for generated C++ data struct (serialized json has to be compatible with GPB)
- [ ] generate protobuf de/serializer for generated C++ data struct (serialized pb has to be compatible with GPB)

### Missing features

- [ ] rpc service support
- [ ] extends
- [ ] groups
