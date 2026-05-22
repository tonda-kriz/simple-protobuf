# simple-protobuf

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C++-20-blue)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C)](https://cmake.org/)
[![Linux-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-linux-tests.yml/badge.svg)](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-linux-tests.yml)
[![Windows-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-windows-tests.yml/badge.svg)](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-windows-tests.yml)
[![Mac-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-macos-tests.yml/badge.svg)](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-macos-tests.yml)
[![Library-coverage](https://tonda-kriz.github.io/simple-protobuf/library/coverage.svg)](https://tonda-kriz.github.io/simple-protobuf/library)

**Lightweight C++ protobuf & JSON serialization library** - no **protoc**, no **Google toolchain** dependency.

1. Define your data in standard `.proto` files.
2. Generate clean, native C++ structs.
3. Serialize/deserialize to `protobuf` (GPB-wire-compatible) and `JSON` (GPB-compatible) with minimal effort.

Ideal for embedded systems (zero heap allocations are possible with [ETL](https://github.com/ETLCPP/etl)), IoT, microservices, or anywhere you want protobuf without installing heavy tooling.

### Example

```proto
// proto/person.proto, hand written
package PhoneBook;

message Person {
  optional string name = 1;
  optional int32 id = 2;  // Unique ID number for this person.
  optional string email = 3;

  enum PhoneType {
    MOBILE = 0;
    HOME = 1;
    WORK = 2;
  }

  message PhoneNumber {
    required string number = 1; // phone number is always required
    optional PhoneType type = 2;
  }

  // all registered phones
  repeated PhoneNumber phones = 4;
}
```

```CPP
// proto/person.pb.h, generated
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
        std::optional<PhoneType> type;
    };
    std::optional<std::string> name;
    // Unique ID number for this person.
    std::optional<int32_t> id;
    std::optional<std::string> email;
    // all registered phones
    std::vector<PhoneNumber> phones;
};
} // namespace PhoneBook
```

```CPP
// main.cpp, hand written
#include <iostream>
#include <proto/person.pb.h>   // <- generated

int main() {
    const auto john = phonebook::Person{
        .name   = "John Doe",
        .id     = 1234,
        .email  = "john@example.com",
        .phones = {{.number = "123456789", .type = phonebook::Person::PhoneType::MOBILE}}
    };

    // JSON round-trip
    const auto json = spb::json::serialize<std::string>(john);
    std::cout << "JSON:\n" << json << "\n\n";

    // Protobuf binary round-trip
    const auto pb_bytes = spb::pb::serialize<std::vector<std::byte>>(john);

    const auto decoded_json = spb::json::deserialize<phonebook::Person>(json);
    const auto decoded_pb = spb::pb::deserialize<phonebook::Person>(pb_bytes);

    // All equal: john == decoded_json == decoded_pb
}
```

```bash
# CMakeLists.txt
add_subdirectory(external/simple-protobuf) # or FetchContent
add_executable(myapp main.cpp proto/person.proto)
spb_protobuf_generate(TARGET myapp)
```

## Why simple-protobuf?

`Simplicity` and `usability`.

* No `protoc` or `Google libs` dependency - uses only its own tiny parser/compiler (`sprotoc`) and the C++ standard library.
* Supports `.proto` files with `proto2` or `proto3` syntax (no edition syntax).
* Generates clean, modern C++ with `std::optional`, `std::vector`, and `enum class`.
* Bundles protobuf and JSON support in a single library.
    * Serialized protobuf and JSON are compatible with official protoc, Python, Go, Java.
* Embedded-friendly, with zero heap allocations when paired with ETL or fixed-size strings/vectors.
    * See [options](doc/options.md) for user-specified types and advanced usage.

## Comparison to other libraries

| Library | Language | Toolchain needed | JSON built-in | Generated style | Best for |
|---------|----------|------------------|--------------|----------------|----------|
| simple-protobuf | C++20 | None (own compiler) | Yes | C++ structs | Lightweight, simple, easy |
| Official protobuf | C++ | protoc | Yes | Heavy classes | Full protobuf but not for embedded |
| nanopb | C | protoc + python plugin | No | C structs + streams | Tiny MCUs, pure C |

## Dependencies

* C++ compiler with C++20 support
* CMake
* Standard C++ library
* *(optional) clang-format for code formatting*

## Type mapping

| proto type | CPP type | GPB encoding |
|------------|----------|--------------|
| `bool`     | `bool` | varint |
| `float`    | `float` | 4 bytes |
| `double`   | `double` | 8 bytes |
| `int32`    | `int32_t` | varint |
| `sint32`   | `int32_t` | zig-zag varint |
| `uint32`   | `uint32_t` | varint |
| `int64`    | `int64_t` | varint |
| `sint64`   | `int64_t` | zig-zag varint |
| `uint64`   | `uint64_t` | varint |
| `fixed32`  | `uint32_t` | 4 bytes |
| `sfixed32` | `int32_t` | 4 bytes |
| `fixed64`  | `uint64_t` | 8 bytes |
| `sfixed64` | `int64_t` | 8 bytes |
| `string`   | `std::string` | UTF-8 string |
| `bytes`    | `std::vector<std::byte>` | base64 encoded in JSON |
| `message`  | `struct` | length delimited |
| `enum`     | `enum class` | varint |
| `map`      | `std::map<,>` | |
| `oneof`    | `std::variant<>` | |

| proto type modifier | CPP type modifier | Notes |
|---------------------|-------------------|-------|
| `optional`          | `std::optional<Message>` | |
| `optional`          | `std::unique_ptr<Message>` | for cyclic message dependencies (A -> B, B -> A) |
| `repeated`          | `std::vector<Message>` | |

See also [options](doc/options.md) for user-specified types and advanced usage.

## Examples

See the [example](example/) directory.

## Status

* [x] Make it work
* [x] Make it right
* [ ] Make it fast

## Roadmap

* [x] Parser for proto files (supported syntax: `proto2` and `proto3`)
* [x] Compile proto messages to C++ data structs
* [x] JSON de/serializer for generated C++ data structs (serialized JSON is GPB-compatible)
* [x] Protobuf de/serializer for generated C++ data structs (serialized protobuf is GPB-compatible)
* [x] Implement options and user-specified types/containers
* [ ] Benchmarks for size and speed, direct comparison with other libraries (official protobuf, nanopb)

## Missing features

* gRPC is not implemented
