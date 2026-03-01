# extensions

All extensions to the .proto files are specified in the comments, so they are ignored by the GPB protoc and therefore compatible with GPB. Extensions are using GPB [options](https://protobuf.dev/programming-guides/proto2/#options) syntax inside of C++ [attribute](https://en.cppreference.com/w/cpp/language/attributes).

Example:

```proto
//[[ field.type = "int16" ]]
```

## int types

You can use also **8** and **16** bit ints (`int8`, `uint8`, `int16`, `uint16`) for fields and for enums. **Warning:** due to compatibility with GPB, always use types with the same sign, like `int32` and `int8`, combinations like `int32` and `uint8` are invalid.

### how to use int types

Each field has an attribute `.type`.

```proto
message Person{
    //[[ field.type = "int16" ]]
    required int32 id = 2;
}
```

will be translated into...

```CPP
struct Person{
    int16_t id;
}
```

## enum types

You can specify type of an enum (`int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`, `int64` and `uint64`)

### how to use enum types

Each enum has an attribute `.type`.

```proto
//[[ enum.type = "uint8" ]]
enum PhoneType {
    MOBILE = 0;
    HOME = 1;
    WORK = 2;
}
```

will be translated into...

```CPP
enum class PhoneType : uint8_t {
    MOBILE = 0,
    HOME   = 1,
    WORK   = 2,
};
```

Default is set to:

```proto
//[[ enum.type = "int32" ]]
```

You can use this attribute ...

- before `package`, means its set for all enums in the **whole .proto file**
- before `message`, means its set for all enums inside one **message only**
- before `enum`, means its set for a specific **enum only**

you can combine them as you want, the more specific ones will be preferred.

## bit-fields

You can use bit fields (`int8:1`, `uint8:2` ...) for [ints](#int-types)

### how to use bit-fields

Bit fields are used similar as in C with [ints](#int-types). *Remember: always use types with the same sign*

```proto
message Device{
    //[[ field.type = "uint8:4" ]]
    required uint8 id_major = 2;
    //[[ field.type = "uint8:4" ]]
    required uint8 id_minor = 2;
}
```

will be translated into...

```CPP
struct Device{
    uint8_t id_major : 4;
    uint8_t id_minor : 4;
}
```

## container types

You can use your own types for containers (`optionals`, `repeated`, `string`, `bytes`).

### how to use user container types

Each container has 2 attributes `.type` (user type) and `.include` (include header for the type).
Container needs to satisfy a [concept](../include/spb/concepts.h).

| container  | [concept](../include/spb/concepts.h)     | notes       |
|------------|------------------------------|-------------|
| `optional` | `proto_label_optional`       | [`optional`](https://protobuf.dev/programming-guides/proto2/#field-labels) field label |
| `repeated` | `proto_label_repeated`       | [`repeated`](https://protobuf.dev/programming-guides/proto2/#field-labels) field label |
| `string` | `proto_field_string`           | fixed size [`string`](https://protobuf.dev/programming-guides/proto2/#scalar) field type |
| `string` | `proto_field_string_resizable` | resizable [`string`](https://protobuf.dev/programming-guides/proto2/#scalar) field type |
| `bytes`  | `proto_field_bytes`            | fixed size [`bytes`](https://protobuf.dev/programming-guides/proto2/#scalar) field type |
| `bytes`  | `proto_field_bytes_resizable`  | resizable [`bytes`](https://protobuf.dev/programming-guides/proto2/#scalar) field type |

Defaults are set to:

```proto
//[[ optional.type = "std::optional<$>" ]]
//[[ optional.include = "<optional>" ]]

//[[ repeated.type = "std::vector<$>" ]]
//[[ repeated.include = "<vector>" ]]

//[[ string.type = "std::string" ]]
//[[ string.include = "<string>" ]]

//[[ bytes.type = "std::vector<$>" ]]
//[[ bytes.include = "<vector>" ]]
```

`$` will be replaced with the `value_type` of a container.

- for `string`, `$` (if specified) will be replaced by `char`.
- for `bytes`, `$` (if specified) will be replaced by `std::byte`.
- for `repeated` and `optional`, `$` will be replaced by a `field.type` specified in .proto

You can use those attributes ...

- before `package`, for the **whole .proto file**
- before `message`, for one **message only**
- before `field`, for one **field only**

you can combine them as you want, the more specific ones will be preferred.

#### fixed size repeated fields

You can use **fixed size** container for repeated fields with simple type (integers, floating points).
Field has to be `repeated` with `packed` attribute (which is a default in proto version 3, for proto version 2 use `[packed = true]`). The container needs to satisfy concept [proto_label_repeated_fixed_size](../include/spb/concepts.h). 
**Warning:** Deserialization will return `false` if the repeated field has a different length than expected.

```proto
message Data{
    //[[ repeated.type = "std::fixed_size_simd<$,4>" ]]
    //[[ repeated.include = "<simd>" ]]
    repeated float coordinates = 1;

    //[[ repeated.type = "std::array<$,32>" ]]
    //[[ repeated.include = "<array>" ]]
    repeated int32 fields = 2;
}
```

will be translated into...

```CPP
struct Data{
    std::fixed_size_simd<float, 4> coordinates;
    std::array<int32_t, 32> fields;
}
```

#### fixed size bytes, string

You can use **fixed size** containers for `bytes` and `string`. They needs to satisfy concept [proto_field_bytes](../include/spb/concepts.h) or [proto_field_string](../include/spb/concepts.h)

```proto
message Device{
    //[[ string.type = "std::array<$,4>" ]]
    //[[ string.include = "<array>" ]]
    optional string id = 1;

    //[[ bytes.type = "std::array<$,32>" ]]
    //[[ bytes.include = "<array>" ]]
    optional bytes sha256 = 2;

    //[[ bytes.type = "std::array<uint8_t,16>" ]]
    //[[ bytes.include = "<array>" ]]
    optional bytes md5 = 3;
}
```

will be translated into...

```CPP
struct Device{
    std::optional< std::array< char, 4 > > id;
    std::optional< std::array< std::byte, 32 > > sha256;
    std::optional< std::array< uint8_t, 16 > > md5;
}
```

### integration with [etl library](https://github.com/ETLCPP/etl)

*the whole code is in [examples](../example/)*

1. define a schema for you data in a `etl.proto` file

```proto

syntax = "proto2";

package ETL.Example;

message DeviceStatus {
    //[[ field.type = "uint32:31"]]
    required uint32 device_id = 1;
    //[[ field.type = "uint32:1"]]
    required uint32 is_online = 2;
    required uint64 last_heartbeat = 3;
    //[[ string.type = "std::array<$,8>" ]]
    //[[ string.include = "<array>" ]]
    required string firmware_version = 4;
    //[[ string.type = "etl::string<16>" ]]
    //[[ string.include = "<etl/string.h>" ]]
    required string name = 5;
}

message Command {
    //[[ enum.type = "uint8"]]
    enum CMD {
        CMD_RST = 1;
        CMD_READ = 2;
        CMD_WRITE = 3;
        CMD_TST = 4;
    }
    //[[ field.type = "uint8:4"]]
    required uint32 command_id = 1; // CMD
    //[[ field.type = "uint8:2"]]
    required uint32 arg = 2; // argument for the command
    //[[ field.type = "uint8:1"]]
    required uint32 in_flag = 3; // input flag
    //[[ field.type = "uint8:1"]]
    required uint32 out_flag = 4; // output flag
}

//[[ repeated.type = "etl::vector<$,16>" ]]
//[[ repeated.include = "<etl/vector.h>" ]]
message CommandQueue {
    repeated Command commands = 1;
    repeated DeviceStatus statuses = 2;
}
```

2. compile `etl.proto` with `spb-protoc` into `etl.pb.h` and `etl.pb.cc`

```cmake
spb_protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_SOURCE_DIR}/proto/etl.proto)
```

You can combine different types for each container in a single .proto file. *In this example the `string` is represented as `etl::string< 16 >` and `std::array< char, 8 >`.*

```C++
namespace ETL::Example
{
struct DeviceStatus
{
    uint32_t device_id : 31;
    uint32_t is_online : 1;
    uint64_t last_heartbeat;
    std::array< char, 8 > firmware_version;
    etl::string< 16 > name;
};
struct Command
{
    enum class CMD : uint8_t
    {
        CMD_RST   = 1,
        CMD_READ  = 2,
        CMD_WRITE = 3,
        CMD_TST   = 4,
    };
    // CMD
    uint8_t command_id : 4;
    // argument for the command
    uint8_t arg : 2;
    // input flag
    uint8_t in_flag : 1;
    // output flag
    uint8_t out_flag : 1;
};
struct CommandQueue
{
    etl::vector< Command, 16 > commands;
    etl::vector< DeviceStatus, 16 > statuses;
};
}// namespace ETL::Example
```

3. use generated structs natively and de/serialize to/from json/pb

```CPP
#include <etl.pb.h>

auto command = ETL::Example::Command{
        .command_id = uint8_t( ETL::Example::Command::CMD::CMD_READ ),
        .arg        = 2,
        .in_flag    = 1,
        .out_flag   = 0,
    };

auto json = spb::json::serialize( command );
auto pb   = spb::pb::serialize( command );

auto cmd  = spb::json::deserialize< ETL::Example::Command >( json );
auto cmd2 = spb::pb::deserialize< ETL::Example::Command >( pb );
//- command == cmd == cmd2
```
