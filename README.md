# simple-protobuf

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C++-20-blue)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C)](https://cmake.org/)
[![Linux-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-linux-tests.yml/badge.svg)](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-linux-tests.yml)
[![Windows-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-windows-tests.yml/badge.svg)](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-windows-tests.yml)
[![Mac-build](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-macos-tests.yml/badge.svg)](https://github.com/tonda-kriz/simple-protobuf/actions/workflows/ci-macos-tests.yml)
[![Library-coverage](https://tonda-kriz.github.io/simple-protobuf/library/coverage.svg)](https://tonda-kriz.github.io/simple-protobuf/library)

**Lightweight C++ protobuf & JSON serialization library** 
 - _Where **simplicity** meets **usability**_
 - Without **protoc** and without **Google toolchain** dependency

### Usage

1. Define your data in standard `.proto` files.
2. Generate clean, native C++ structs.
3. Serialize/deserialize to `protobuf` (GPB-wire-compatible) and `JSON` (GPB-compatible) with minimal effort.

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

```bash
# CMakeLists.txt, hand written
add_subdirectory(external/simple-protobuf) # or FetchContent
add_executable(myapp main.cpp proto/person.proto)
spb_protobuf_generate(TARGET myapp)
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

## Features

* No `protoc` or `Google libs` dependency - instead it uses its own proto-compiler called `sprotoc`.
* Supports `.proto` files with `proto2` or `proto3` syntax (no edition syntax).
* Generates clean, modern C++ with `std::optional`, `std::vector`, and `enum class`.
* Bundles protobuf and JSON support in a single library.
    * Serialized protobuf and JSON are compatible with official protoc, Python, Go, Java.
* Embedded-friendly, zero heap allocations when paired with [ETL](https://github.com/ETLCPP/etl) or fixed-size strings/vectors.
    * See [options](doc/options.md) for user-specified types and advanced usage.

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

## Doc

* [API](doc/API.md)
* [Options](doc/options.md)

## Performance benchmark

_Tiny and Fast_

See the [benchmark](benchmark/) directory.

### Speed

Measured on `Linux/i7-8650U CPU @ 1.90GHz` with GCC 16.1.1 `-flto -O2` using [nanobench](https://github.com/martinus/nanobench).

SPB protobuf serializer/deserializer has the **same speed** as google GPB.
SPB JSON serializer/deserializer is about **4x faster** than google GPB.

|        ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% | benchmark
|-------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|:----------
|       321.88 |        3,106,725.78 |    0.7% |        1,958.00 |          670.07 |  2.922 |         441.00 |    0.0% | [gpb-pb-serialize](benchmark/gpb/benchmark.cpp)
|       314.33 |        3,181,404.47 |    0.6% |        1,946.00 |          650.52 |  2.991 |         435.00 |    0.0% | [gpb-lite-pb-serialize](benchmark/gpb/benchmark.cpp)
|       330.46 |        3,026,072.90 |    1.0% |        2,107.00 |          674.66 |  3.123 |         480.00 |    0.0% | [spb-pb-serialize](benchmark/spb/benchmark.cpp)
|       920.77 |        1,086,048.03 |    0.7% |        4,887.00 |        1,922.73 |  2.542 |       1,159.00 |    0.0% | [gpb-pb-init-serialize](benchmark/gpb/benchmark.cpp)
|       949.20 |        1,053,516.29 |    0.7% |        4,875.00 |        1,966.39 |  2.479 |       1,153.00 |    0.0% | [gpb-lite-pb-init-serialize](benchmark/gpb/benchmark.cpp)
|       733.35 |        1,363,600.01 |    0.8% |        4,156.00 |        1,509.86 |  2.753 |       1,006.00 |    0.0% | [spb-pb-init-serialize](benchmark/spb/benchmark.cpp)
|     1,002.70 |          997,308.51 |    0.6% |        5,208.00 |        2,098.15 |  2.482 |       1,204.00 |    0.1% | [gpb-pb-deserialize](benchmark/gpb/benchmark.cpp)
|     1,017.12 |          983,171.45 |    0.7% |        5,209.00 |        2,115.23 |  2.463 |       1,204.00 |    0.0% | [gpb-lite-pb-deserialize](benchmark/gpb/benchmark.cpp)
|       862.48 |        1,159,452.34 |    0.7% |        4,523.00 |        1,772.13 |  2.552 |       1,051.00 |    0.0% | [spb-pb-deserialize](benchmark/spb/benchmark.cpp)
|    10,478.95 |           95,429.37 |    0.7% |       51,398.01 |       21,751.67 |  2.363 |      13,117.01 |    0.4% | [gpb-json-serialize](benchmark/gpb/benchmark.cpp)
|     2,316.65 |          431,657.25 |    0.7% |       10,848.00 |        4,750.24 |  2.284 |       3,005.00 |    0.2% | [spb-json-serialize](benchmark/spb/benchmark.cpp)
|    11,229.19 |           89,053.66 |    0.5% |       54,660.36 |       23,453.17 |  2.331 |      13,913.09 |    0.4% | [gpb-json-init-serialize](benchmark/gpb/benchmark.cpp)
|     2,799.49 |          357,208.27 |    0.9% |       12,898.00 |        5,742.87 |  2.246 |       3,531.00 |    0.2% | [spb-json-init-serialize](benchmark/spb/benchmark.cpp)
|    20,054.93 |           49,863.05 |    0.6% |       93,949.63 |       41,858.71 |  2.244 |      22,892.55 |    0.3% | [gpb-json-deserialize](benchmark/gpb/benchmark.cpp)
|     3,132.97 |          319,186.41 |    0.7% |       16,691.00 |        6,439.46 |  2.592 |       3,194.00 |    0.3% | [spb-json-deserialize](benchmark/spb/benchmark.cpp)

### Binary size

Measured with 
```bash
$ ls -alh ./build/benchmark
```

SPB is tiny compared to google GPB, which makes it ideal for Embedded systems.

| Binary size (bytes) |             Binary name | Description                                          |
|--------------------:|:------------------------|------------------------------------------------------|
|                3,0M |        [gpb-pb-serialize](benchmark/gpb/pb-serialize.cpp) | Serialize single PB message with libprotobuf         |
|                822K |   [gpb-lite-pb-serialize](benchmark/gpb/pb-serialize.cpp) | Serialize single PB message with libprotobuf-lite    |
|             **22K** |        [spb-pb-serialize](benchmark/spb/pb-serialize.cpp) | Serialize single PB message with simple-protobuf     |
|                3,0M |      [gpb-pb-deserialize](benchmark/gpb/pb-deserialize.cpp) | Deserialize single PB message with libprotobuf       |
|                822K | [gpb-lite-pb-deserialize](benchmark/gpb/pb-deserialize.cpp) | Deserialize single PB message with libprotobuf-lite  |
|             **31K** |      [spb-pb-deserialize](benchmark/spb/pb-deserialize.cpp) | Deserialize single PB message with simple-protobuf   |
|                3,5M |      [gpb-json-serialize](benchmark/gpb/json-serialize.cpp) | Serialize single JSON message with libprotobuf       |
|             **27K** |      [spb-json-serialize](benchmark/spb/json-serialize.cpp) | Serialize single JSON message with simple-protobuf   |
|                3,5M |    [gpb-json-deserialize](benchmark/gpb/json-deserialize.cpp) | Deserialize single JSON message with libprotobuf     |
|             **81K** |    [spb-json-deserialize](benchmark/spb/json-deserialize.cpp) | Deserialize single JSON message with simple-protobuf |


## Status

* [x] Make it work
* [x] Make it right
* [x] Make it fast

## Roadmap

* [x] Parser for proto files (supported syntax: `proto2` and `proto3`)
* [x] Compile proto messages to C++ data structs
* [x] JSON de/serializer for generated C++ data structs (serialized JSON is GPB-compatible)
* [x] Protobuf de/serializer for generated C++ data structs (serialized protobuf is GPB-compatible)
* [x] Implement options and user-specified types/containers
* [x] Benchmarks for size and speed, direct comparison with other libraries (official protobuf, nanopb)

## Missing features

* gRPC is not implemented
