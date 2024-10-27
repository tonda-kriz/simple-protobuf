# extensions

All extensions to the .proto files are compatible with gpb.

## small int types

The library lets you specify also 8 and 16 bit ints (`int8`, `uint8`, `int16`, `uint16`) for scalars and for enums. Warning: due to compatibility with gpb, always use scalar types with the same sign, like `int32` and `int8`, combinations like `int32` and `uint8` are invalid.

### how to use small ints for scalars

Small int types are specified in the comments, so they are ignored by the gpb protoc.
Each field has an attribute `.type`. Example:

```proto
//[[ field.type = "int16" ]]
optional int32 id = 2;
```

### how to use small ints for enums

Small int enum types are specified in the comments, so they are ignored by the gpb protoc.
Each enum has an attribute `.type`. Valid values are (`int8`, `uint8`, `int16`, `uint16`, `int32`)
Example:

```proto
//[[ enum.type = "int16" ]]
enum PhoneType {
    MOBILE = 0;
    HOME = 1;
    WORK = 2;
}
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

The library lets you specify bit fields (`int8:1`, `uint8:2` ...) for [small ints](#small-int-types)

### how to use bit-fields

Bit fields are used similar as in C with [small ints](#small-int-types)

```proto
//[[ field.type = "int16:4" ]]
optional int32 id = 2;
```

## container types

The library lets you specify your own types for containers (`repeated`, `string`, `bytes`, `optionals`).

### how to use user container types

Containers types are specified in the comments, so they are ignored by the gpb protoc.
Each container has 2 attributes `.type` (user type) and `.include` (include header for the type).
Container needs to satisfy a [concept](../include/spb/concepts.h).

| container  | [concept](../include/spb/concepts.h)     | Notes       |
|------------|------------------------------|-------------|
| `optional` | `proto_label_optional`       | `optional` field label |
| `repeated` | `proto_label_repeated`       | `repeated` field label |
| `string` | `proto_field_string`           | fixed size `string` field type |
| `string` | `proto_field_string_resizable` | resizable `string` field type |
| `bytes`  | `proto_field_bytes`            | fixed size `bytes` field type |
| `string` | `proto_field_bytes_resizable`  | resizable `bytes` field type |

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
