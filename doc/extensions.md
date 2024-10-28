# extensions

All extensions to the .proto files are specified in the comments, so they are ignored by the GPB protoc and therefore compatible with GPB. Extensions are using GPB [options](https://protobuf.dev/programming-guides/proto2/#options) syntax inside of C++ [attribute](https://en.cppreference.com/w/cpp/language/attributes).

Example:

```proto
//[[ field.type = "int16" ]]
```

## int types

You can use also **8** and **16** bit ints (`int8`, `uint8`, `int16`, `uint16`) for fields and for enums. Warning: due to compatibility with GPB, always use types with the same sign, like `int32` and `int8`, combinations like `int32` and `uint8` are invalid.

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

You can specify your own types for containers (`optionals`, `repeated`, `string`, `bytes`).

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
- for `repeated` and `optional`, `$` will be replaced by a `field type` specified in .proto

You can use those attributes ...

- before `package`, means its set for the **whole .proto file**
- before `message`, means its set for one **message only**
- before `field`, means its set for one **field only**

you can combine them as you want, the more specific ones will be preferred.

#### fixed size bytes, string

You can use **fixed size** containers for `bytes` and `string`. They needs to satisfy concept [proto_field_bytes](../include/spb/concepts.h) or [proto_field_string](../include/spb/concepts.h)

```proto
message Person{
    //[[ string.type = "std::array<$,4>" ]]
    //[[ string.include = "<array>" ]]
    optional string id = 1;
}
```

will be translated into...

```CPP
struct Person{
    std::optional< std::array< char, 4 > > id;
}
```

### integration with [etl library](https://github.com/ETLCPP/etl)

1. define a schema for you data in a `person.proto` file

```proto
//[[ repeated.type = "etl::vector<$,60>" ]]
//[[ repeated.include = "<etl/vector.h>" ]]

//[[ string.type = "etl::string<32>" ]]
//[[ string.include = "<etl/string.h>" ]]

package PhoneBook.Etl;

message Person {
    //[[ string.type = "std::string" ]]
    optional string name = 1;
    //[[ field.type = "int16"]]
    optional int32 id = 2;
    //[[ string.type = "etl::string<64>" ]]
    optional string email = 3;
    //[[ enum.type = "uint8"]]
    enum PhoneType {
        MOBILE = 0;
        HOME = 1;
        WORK = 2;
    }

    //[[ string.type = "etl::string<16>" ]]
    message PhoneNumber {
        required string number = 1;
        optional PhoneType type = 2;
    }
    repeated PhoneNumber phones = 4;
}
```

2. compile `person.proto` with `spb-protoc` into `person.pb.h` and `person.pb.cc`

```cmake
spb_protobuf_generate(PROTO_SRCS PROTO_HDRS ${CMAKE_SOURCE_DIR}/proto/person.proto)
```

You can combine different types for each container in a single .proto file. In the following example the `string` is represented by `etl::string< 16 >`, `std::string` and `etl::string< 64 >`.

```C++
namespace PhoneBook::Etl
{
struct Person
{
    enum class PhoneType : uint8_t
    {
        MOBILE = 0,
        HOME   = 1,
        WORK   = 2,
    };
    struct PhoneNumber
    {
        etl::string< 16 > number;
        std::optional< PhoneType > type;
    };
    std::optional< std::string > name;
    std::optional< int16_t > id;
    std::optional< etl::string< 64 > > email;
    etl::vector< PhoneNumber, 60 > phones;
};
}// namespace PhoneBook::Etl
```

3. use `Person` struct natively and de/serialize to/from json/pb

```CPP
#include <person.pb.h>

auto john = PhoneBook::Etl::Person{
        .name  = "John Doe",
        .id    = 1234,
        .email = "jdoe@example.com",
    };

auto json    = spb::json::serialize( john );
auto person  = spb::json::deserialize< PhoneBook::Etl::Person >( json );
auto pb      = spb::pb::serialize( john );
auto person2 = spb::pb::deserialize< PhoneBook::Etl::Person >( pb );
//- john == person == person2
```
