#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <spb/json.hpp>
#include <spb/pb.hpp>
#include <vector>

namespace SPB::Options
{
//- enum type: int8, uint8, int16, uint16, int32
enum class Enum8 : uint8_t
{
    MOBILE = 0,
    HOME   = 1,
    WORK   = 2,
};
enum class Enum16 : int16_t
{
    MOBILE = 0,
    HOME   = 1,
    WORK   = 2,
};
// int can also be: int8, uint8, int16, uint16
struct Integers
{
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    // alternative syntax, to avoid import "spb.proto";
    std::optional< int8_t > i8;
    std::optional< int16_t > i16;
    std::optional< int32_t > i32;
    std::optional< int64_t > i64;
    std::vector< int8_t > r8;
    std::vector< int16_t > r16;
    std::vector< int32_t > r32;
    std::vector< int64_t > r64;
};
// int bit-fields: `int8:1`, `uint8:2`, ...
struct BitFields
{
    uint8_t id_major : 4;
    uint8_t id_minor : 4;
};
// repeated fields can have maximal number of elements
// this is not directly visible in the generated pb.h
struct MaximumCount
{
    struct Person
    {
        uint32_t age;
    };
    // can be set for a single field
    std::vector< int32_t > up_to_4_int;
    std::vector< Person > up_to_5_persons;
};
// bytes and string can have maximum size
// this is not directly visible in the generated pb.h
struct MaximumSize
{
    // can be set for a single field
    std::optional< std::string > up_to_4_chars;
    std::optional< std::vector< std::byte > > up_to_5_bytes;
    std::string up_to_6_chars;
    std::vector< std::byte > up_to_6_bytes;
    // can be also combined with the `.max_count`
    std::vector< std::string > up_to_7_short_strings;
    std::vector< std::vector< std::byte > > up_to_7_short_bytes;
};
// container types for (`optional`, `repeated`, `string`, `bytes`)
// if you prefer for example boost, you can make all `optional`s in the whole
// file boost like, just uncomment the following line
// option (spb_fileopt).optional = "boost::optional<$>";
struct Containers
{
    struct Repeated
    {
        // default repeated is an std::vector<$>
        std::vector< int32_t > default_i;
        // packed repeated fields can have fixed size
        std::array< int32_t, 10 > fixed_size;
    };
    struct String
    {
        struct SubStrings
        {
            std::vector< char > vec_of_chars1;
            std::optional< std::vector< char > > vec_of_chars2;
            //- an array of fixed size strings
            std::vector< std::array< char, 8 > > fixed_string;
        };
        // default string is an std::string
        std::string default_string;
    };
    struct Bytes
    {
        struct SubBytes
        {
            std::basic_string< std::byte > bytes_as_string1;
            std::optional< std::basic_string< std::byte > > bytes_as_string2;
            // an array of fixed size bytes
            std::vector< std::array< std::byte, 8 > > fixed_bytes;
        };
        // default bytes is an std::vector<std::byte>
        std::vector< std::byte > default_bytes;
    };
    struct Optional
    {
        // if messages depends on each other, one of them
        // will became std::unique_ptr<$>
        struct CyclicDependency
        {
            struct MessageB;

            struct MessageA
            {
                std::optional< int32_t > i;
                std::unique_ptr< MessageB > b;
            };
            struct MessageC
            {
                std::optional< int32_t > i;
                std::optional< MessageA > a;
            };
            struct MessageB
            {
                std::optional< int32_t > i;
                std::optional< MessageC > c;
            };
        };
        // default optional is an std::optional<$>
        std::optional< int32_t > default_i;
    };
};
}// namespace SPB::Options

namespace spb::json::detail
{
struct ostream;
struct istream;

void serialize_value( ostream & stream, const ::SPB::Options::Integers & value );
void deserialize_value( istream & stream, ::SPB::Options::Integers & value );

void serialize_value( ostream & stream, const ::SPB::Options::BitFields & value );
void deserialize_value( istream & stream, ::SPB::Options::BitFields & value );

void serialize_value( ostream & stream, const ::SPB::Options::MaximumCount & value );
void deserialize_value( istream & stream, ::SPB::Options::MaximumCount & value );

void serialize_value( ostream & stream, const ::SPB::Options::MaximumSize & value );
void deserialize_value( istream & stream, ::SPB::Options::MaximumSize & value );

void serialize_value( ostream & stream, const ::SPB::Options::Containers & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers & value );

void serialize_value( ostream & stream, const ::SPB::Options::MaximumCount::Person & value );
void deserialize_value( istream & stream, ::SPB::Options::MaximumCount::Person & value );

void serialize_value( ostream & stream, const ::SPB::Options::Containers::Repeated & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Repeated & value );

void serialize_value( ostream & stream, const ::SPB::Options::Containers::String & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::String & value );

void serialize_value( ostream & stream, const ::SPB::Options::Containers::Bytes & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Bytes & value );

void serialize_value( ostream & stream, const ::SPB::Options::Containers::Optional & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Optional & value );

void serialize_value( ostream & stream,
                      const ::SPB::Options::Containers::String::SubStrings & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::String::SubStrings & value );

void serialize_value( ostream & stream, const ::SPB::Options::Containers::Bytes::SubBytes & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Bytes::SubBytes & value );

void serialize_value( ostream & stream,
                      const ::SPB::Options::Containers::Optional::CyclicDependency & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency & value );

void serialize_value(
    ostream & stream,
    const ::SPB::Options::Containers::Optional::CyclicDependency::MessageA & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency::MessageA & value );

void serialize_value(
    ostream & stream,
    const ::SPB::Options::Containers::Optional::CyclicDependency::MessageC & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency::MessageC & value );

void serialize_value(
    ostream & stream,
    const ::SPB::Options::Containers::Optional::CyclicDependency::MessageB & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency::MessageB & value );

void serialize_value( ostream & stream, const ::SPB::Options::Enum8 & value );
void deserialize_value( istream & stream, ::SPB::Options::Enum8 & value );

void serialize_value( ostream & stream, const ::SPB::Options::Enum16 & value );
void deserialize_value( istream & stream, ::SPB::Options::Enum16 & value );

}// namespace spb::json::detail
namespace spb::pb::detail
{
struct ostream;
struct istream;

void serialize( ostream & stream, const ::SPB::Options::Integers & value );
void deserialize_value( istream & stream, ::SPB::Options::Integers & value, tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::BitFields & value );
void deserialize_value( istream & stream, ::SPB::Options::BitFields & value, tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::MaximumCount & value );
void deserialize_value( istream & stream, ::SPB::Options::MaximumCount & value, tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::MaximumSize & value );
void deserialize_value( istream & stream, ::SPB::Options::MaximumSize & value, tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::Containers & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers & value, tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::MaximumCount::Person & value );
void deserialize_value( istream & stream, ::SPB::Options::MaximumCount::Person & value,
                        tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::Containers::Repeated & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Repeated & value,
                        tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::Containers::String & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::String & value,
                        tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::Containers::Bytes & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Bytes & value, tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::Containers::Optional & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Optional & value,
                        tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::Containers::String::SubStrings & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::String::SubStrings & value,
                        tag_type tag );

void serialize( ostream & stream, const ::SPB::Options::Containers::Bytes::SubBytes & value );
void deserialize_value( istream & stream, ::SPB::Options::Containers::Bytes::SubBytes & value,
                        tag_type tag );

void serialize( ostream & stream,
                const ::SPB::Options::Containers::Optional::CyclicDependency & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency & value,
                        tag_type tag );

void serialize( ostream & stream,
                const ::SPB::Options::Containers::Optional::CyclicDependency::MessageA & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency::MessageA & value,
                        tag_type tag );

void serialize( ostream & stream,
                const ::SPB::Options::Containers::Optional::CyclicDependency::MessageC & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency::MessageC & value,
                        tag_type tag );

void serialize( ostream & stream,
                const ::SPB::Options::Containers::Optional::CyclicDependency::MessageB & value );
void deserialize_value( istream & stream,
                        ::SPB::Options::Containers::Optional::CyclicDependency::MessageB & value,
                        tag_type tag );

}// namespace spb::pb::detail
