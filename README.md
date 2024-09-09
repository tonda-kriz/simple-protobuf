# simple-protobuf

![Linux-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-linux-tests.yml/badge.svg)
![Windows-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-windows-tests.yml/badge.svg)
![Mac-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-macos-tests.yml/badge.svg)

**simple data struct** serialization library for C++. With this library you can serialize and deserialize C++ structs directly to [JSON](https://json.org) or [protobuf](https://github.com/protocolbuffers/protobuf).

```CPP
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

auto john = PhoneBook::Person{
        .name  = "John Doe",
        .id    = 1234,
        .email = "jdoe@example.com",
    };
auto json    = spb::json::serialize( john );
auto person  = spb::json::deserialize< PhoneBook::Person >( json );
auto pb      = spb::pb::serialize( john );
auto person2 = spb::pb::deserialize< PhoneBook::Person >( pb );
//- john == person == person2
```

## goal

goal of this library is to make [JSON](https://json.org) and [protobuf](https://github.com/protocolbuffers/protobuf) *part* of the C++ language itself.

## reason

There are literally a tons of [JSON](https://json.org) C++ libraries but most of them are designed in a way that the user needs to construct the json *Object* via some API and for serialization and deserialization there is a lot of boilerplate code like type/schema checking, `to_json`, `from_json`, macros... All this is needed to be done by the user, and it usually ends up with a conversion to some C++ struct.

spb works the other way around, from C++ struct to [JSON](https://json.org) or [protobuf](https://github.com/protocolbuffers/protobuf). With this approach user can focus only on the data, C++ struct, which is much more natural and spb will handle all the boring stuff like serialization/deserialization and type/schema checking.

## about

spb is an alternative implementation of [protobuf](https://github.com/protocolbuffers/protobuf) for C++. This is not an plugin for `protoc` but an **replacement** for `protoc`, so you don't need `protobuf` or `protoc` installed to use it. Serialization and deserialization to [JSON](https://json.org) or [protobuf](https://github.com/protocolbuffers/protobuf) is compatible with `protoc`, in other words, data serialized with code generated by `spb-protoc` can be deserialized by code generated by `protoc` and vice versa.

## Usage

Cheat sheet

```cmake
# add this repo to your project
add_subdirectory(external/spb-proto)
# compile proto files to C++
spb_protobuf_generate(PROTO_SRCS PROTO_HDRS ${CMAKE_SOURCE_DIR}/proto/person.proto)
# add generated files to your project
add_executable(myapp ${PROTO_SRCS} ${PROTO_HDRS})
# `spb-proto` is an interface library
# the main purpose is to update include path of `myapp` 
target_link_libraries(myapp PUBLIC spb-proto)
```

1. define a schema for you data in a `person.proto` file

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

2. compile `person.proto` with `spb-protoc` into `person.pb.h` and `person.pb.cc`

```cmake
spb_protobuf_generate(PROTO_SRCS PROTO_HDRS ${CMAKE_SOURCE_DIR}/proto/person.proto)
```

*observe the beautifully generated `person.pb.h` and `person.pb.cc`*

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

3. use `Person` struct natively and de/serialize to/from json/pb

```CPP
#include <person.pb.h>

auto john = PhoneBook::Person{
        .name  = "John Doe",
        .id    = 1234,
        .email = "jdoe@example.com",
    };

auto json    = spb::json::serialize( john );
auto person  = spb::json::deserialize< PhoneBook::Person >( json );
auto pb      = spb::pb::serialize( john );
auto person2 = spb::pb::deserialize< PhoneBook::Person >( pb );
//- john == person == person2
```

## API

all API is generated (for both **json** and **protobuf**) in `person.pb.h` for every message and enum type.

```CPP
//- return size in bytes of serialized value
auto serialize_size( const PhoneBook::Person & value ) noexcept -> size_t;
//- serialize value into std::string
auto serialize( const PhoneBook::Person & value ) -> std::string;
//- serialize value into buffer, return serialized size. 
//- Warning: user is responsible for the buffer to be big enough.
auto serialize( const PhoneBook::Person & value, void * buffer ) -> size_t;

//- deserialize variable from data
void deserialize( PhoneBook::Person & result, std::string_view data );
//- return deserialized variable
template<>
auto deserialize< PhoneBook::Person >( std::string_view data ) -> PhoneBook::Person;
```

API is prefixed with `spb::json::` for **json** and `spb::pb::` for **protobuf**.

## Type mapping

| proto type | CPP type    | Notes       |
|------------|-------------|-------------|
| `bool`     | `bool`      |             |
| `float`    | `float`     |             |
| `double`   | `double`    |             |
| `int32`    | `int32_t`   |             |
| `sint32`   | `int32_t`   | zig-zag encoded in pb |
| `uint32`   | `uint32_t`  |             |
| `int64`    | `int64_t`   |             |
| `sint64`   | `int64_t`   | zig-zag encoded in pb |
| `uint64`   | `uint64_t`  |             |
| `int32`    | `int32_t`   |             |
| `fixed32`  | `uint32_t`  | always encoded as 4 bytes in pb |
| `sfixed32` | `int32_t`   | always encoded as 4 bytes in pb |
| `fixed64`  | `uint64_t`  | always encoded as 8 bytes in pb |
| `sfixed64` | `int64_t`   | always encoded as 8 bytes in pb |
| `string`   | `std::string` |             |
| `string`   | `std::string_view` | with `[ ctype = STRING_PIECE ]` |
| `bytes`    | `std::vector< std::byte >` | base64 encoded in json            |
| `bytes`    | `std::span< const std::byte >` | with `[ ctype = STRING_PIECE ]` |

| proto type modifier | CPP type modifier   | Notes       |
|---------------------|-------------|-------------|
| `optional`          | `std::optional` |             |
| `optional`          | `std::unique_ptr` | if there is cyclic dependency between messages ( A -> B, B -> A )|
| `repeated`          | `std::vector`  |             |

## example

navigate to the [example](example/) directory.

## status

- [x] Make it work
- [x] Make it right
- [ ] Make it fast

### roadmap

- [x] parser for proto files (supported syntax: `proto2` and `proto3`)
- [x] compile proto message to C++ data struct
- [x] generate json de/serializer for generated C++ data struct (serialized json has to be compatible with GPB)
- [x] generate protobuf de/serializer for generated C++ data struct (serialized pb has to be compatible with GPB)
