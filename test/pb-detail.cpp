#include "sds/pb/wire-types.h"
#include <cstdint>
#include <memory>
#include <name.pb.h>
#include <optional>
#include <person.pb.h>
#include <scalar.pb.h>
#include <sds/pb/deserialize.hpp>
#include <sds/pb/serialize.hpp>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace Test
{
auto operator==( const Test::Name & lhs, const Test::Name & rhs ) noexcept -> bool
{
    return lhs.name == rhs.name;
}
auto operator==( const Test::Variant & lhs, const Test::Variant & rhs ) noexcept -> bool
{
    return lhs.oneof_field == rhs.oneof_field;
}
}// namespace Test

namespace PhoneBook
{
auto operator==( const Person::PhoneNumber & lhs, const Person::PhoneNumber & rhs ) noexcept -> bool
{
    return lhs.number == rhs.number && lhs.type == rhs.type;
}

auto operator==( const Person & lhs, const Person & rhs ) noexcept -> bool
{
    return lhs.name == rhs.name && lhs.id == rhs.id && lhs.email == rhs.email && lhs.phones == rhs.phones;
}
}// namespace PhoneBook

namespace
{
auto to_bytes( std::string_view str ) -> std::vector< std::byte >
{
    auto span = std::span< std::byte >( ( std::byte * ) str.data( ), str.size( ) );
    return { span.data( ), span.data( ) + span.size( ) };
}

template < typename T >
auto pb_serialize( const T & value ) -> std::string
{
    auto size_stream = sds::pb::detail::ostream( nullptr );
    sds::pb::detail::serialize( size_stream, 1, value );
    const auto size = size_stream.size( );
    auto result     = std::string( size, '\0' );
    auto stream     = sds::pb::detail::ostream( result.data( ) );
    sds::pb::detail::serialize( stream, 1, value );
    return result;
}

template < sds::pb::detail::scalar_encoder encoder, typename T >
auto pb_serialize_as( const T & value ) -> std::string
{
    auto size_stream = sds::pb::detail::ostream( nullptr );
    sds::pb::detail::serialize_as< encoder >( size_stream, 1, value );
    const auto size = size_stream.size( );
    auto result     = std::string( size, '\0' );
    auto stream     = sds::pb::detail::ostream( result.data( ) );
    sds::pb::detail::serialize_as< encoder >( stream, 1, value );
    return result;
}

template < sds::pb::detail::wire_type type, typename T >
auto pb_deserialize( std::string_view protobuf ) -> T
{
    auto stream = sds::pb::detail::istream( protobuf.data( ), protobuf.size( ) );
    auto value  = T( );
    sds::pb::detail::deserialize( stream, value, type );
    CHECK( stream.empty( ) );

    return value;
}

template < sds::pb::detail::scalar_encoder encoder, typename T >
auto pb_deserialize_as( std::string_view protobuf ) -> T
{
    auto stream = sds::pb::detail::istream( protobuf.data( ), protobuf.size( ) );
    auto value  = T( );
    while( !stream.empty( ) )
    {
        sds::pb::detail::deserialize_as< encoder >( stream, value, sds::pb::detail::wire_type_from_scalar_encoder( encoder ) );
    }
    return value;
}

template < sds::pb::detail::scalar_encoder encoder, typename T >
auto pb_deserialize_map_as( std::string_view protobuf ) -> T
{
    auto stream = sds::pb::detail::istream( protobuf.data( ), protobuf.size( ) );
    auto value  = T( );
    while( !stream.empty( ) )
    {
        sds::pb::detail::deserialize_as< encoder >( stream, value, sds::pb::detail::wire_type::length_delimited );
    }
    return value;
}

using sds::pb::detail::scalar_encoder;
using sds::pb::detail::wire_type;

}// namespace
using namespace std::literals;

TEST_CASE( "protobuf" )
{
    SUBCASE( "serialize" )
    {
        SUBCASE( "string" )
        {
            SUBCASE( "required" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Strings::ReqString{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Strings::ReqString{ .value = "hello" } ) == "\x0a\x05hello" );
            }
            SUBCASE( "optional" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Strings::OptString{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Strings::OptString{ .value = "hello" } ) == "\x0a\x05hello" );
            }
            SUBCASE( "repeated" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Strings::RepString{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Strings::RepString{ .value = { "hello" } } ) == "\x0a\x05hello" );
                CHECK( sds::pb::serialize( Test::Scalar::Strings::RepString{ .value = { "hello", "world" } } ) == "\x0a\x05hello\x0a\x05world" );
            }
        }
        SUBCASE( "string_view" )
        {
            CHECK( pb_serialize( "john"sv ) == "\x0a\x04john" );
            CHECK( pb_serialize( ""sv ) == "" );
            SUBCASE( "optional" )
            {
                CHECK( pb_serialize< std::optional< std::string_view > >( std::nullopt ) == "" );
                CHECK( pb_serialize< std::optional< std::string_view > >( "hello" ) == "\x0a\x05hello" );
            }
            SUBCASE( "array" )
            {
                CHECK( pb_serialize< std::vector< std::string_view > >( { "hello", "world" } ) == "\x0a\x05hello\x0a\x05world" );
                CHECK( pb_serialize< std::vector< std::string_view > >( { } ) == "" );
            }
        }
        SUBCASE( "bool" )
        {
            SUBCASE( "required" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqBool{ } ) == "\x08\x00"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqBool{ .value = true } ) == "\x08\x01" );
                CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqBool{ .value = false } ) == "\x08\x00"sv );
            }
            SUBCASE( "optional" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Integers::OptBool{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Integers::OptBool{ .value = true } ) == "\x08\x01" );
                CHECK( sds::pb::serialize( Test::Scalar::Integers::OptBool{ .value = false } ) == "\x08\x00"sv );
            }
            SUBCASE( "repeated" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Integers::RepBool{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Integers::RepBool{ .value = { true } } ) == "\x08\x01" );
                CHECK( sds::pb::serialize( Test::Scalar::Integers::RepBool{ .value = { true, false } } ) == "\x08\x01\x08\x00"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Integers::RepBool{ .value = {} } ) == "" );
            }
        }
        SUBCASE( "int" )
        {
            SUBCASE( "varint32" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqInt32{ .value = 0x42 } ) == "\x08\x42" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqInt32{ .value = 0xff } ) == "\x08\xff\x01" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqInt32{ .value = -2 } ) == "\x08\xfe\xff\xff\xff\x0f"sv );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt32{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt32{ .value = 0x42 } ) == "\x08\x42" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt32{ .value = 0xff } ) == "\x08\xff\x01" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt32{ .value = -2 } ) == "\x08\xfe\xff\xff\xff\x0f"sv );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt32{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt32{ .value = { 0x42 } } ) == "\x08\x42" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt32{ .value = { 0x42, 0x3 } } ) == "\x08\x42\x08\x03" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt32{ .value = {} } ) == "" );

                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt32{ } ) == "" );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt32{ .value = { 0x42 } } ) == "\x0a\x01\x42" );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt32{ .value = { 0x42, 0x3 } } ) == "\x0a\x02\x42\x03" );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt32{ .value = {} } ) == "" );
                    }
                }
            }
            SUBCASE( "varint64" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqInt64{ .value = 0x42 } ) == "\x08\x42" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqInt64{ .value = 0xff } ) == "\x08\xff\x01" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqInt64{ .value = -2 } ) == "\x08\xfe\xff\xff\xff\xff\xff\xff\xff\xff\x01" );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt64{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt64{ .value = 0x42 } ) == "\x08\x42" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt64{ .value = 0xff } ) == "\x08\xff\x01" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptInt64{ .value = -2 } ) == "\x08\xfe\xff\xff\xff\xff\xff\xff\xff\xff\x01" );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt64{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt64{ .value = { 0x42 } } ) == "\x08\x42" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt64{ .value = { 0x42, 0x3 } } ) == "\x08\x42\x08\x03" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepInt64{ .value = {} } ) == "" );

                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt64{ } ) == "" );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt64{ .value = { 0x42 } } ) == "\x0a\x01\x42" );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt64{ .value = { 0x42, 0x3 } } ) == "\x0a\x02\x42\x03" );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackInt64{ .value = {} } ) == "" );
                    }
                }
            }
            SUBCASE( "svarint32" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSint32{ .value = 0x42 } ) == "\x08\x84\x01"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSint32{ .value = 0xff } ) == "\x08\xfe\x03"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSint32{ .value = -2 } ) == "\x08\x03"sv );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint32{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint32{ .value = 0x42 } ) == "\x08\x84\x01"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint32{ .value = 0xff } ) == "\x08\xfe\x03"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint32{ .value = -2 } ) == "\x08\x03"sv );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint32{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint32{ .value = { 0x42 } } ) == "\x08\x84\x01"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint32{ .value = { 0x42, -2 } } ) == "\x08\x84\x01\x08\x03"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint32{ .value = {} } ) == "" );

                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSint32{ .value = { 0x42 } } ) == "\x0a\x02\x84\x01"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSint32{ .value = { 0x42, -2 } } ) == "\x0a\x03\x84\x01\x03"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSint32{ .value = {} } ) == "" );
                    }
                }
            }
            SUBCASE( "svarint64" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSint64{ .value = 0x42 } ) == "\x08\x84\x01"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSint64{ .value = 0xff } ) == "\x08\xfe\x03"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSint64{ .value = -2 } ) == "\x08\x03"sv );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint64{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint64{ .value = 0x42 } ) == "\x08\x84\x01"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint64{ .value = 0xff } ) == "\x08\xfe\x03"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSint64{ .value = -2 } ) == "\x08\x03"sv );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint64{ } ) == "" );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint64{ .value = { 0x42 } } ) == "\x08\x84\x01"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint64{ .value = { 0x42, -2 } } ) == "\x08\x84\x01\x08\x03"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSint64{ .value = {} } ) == "" );

                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSint64{ .value = { 0x42 } } ) == "\x0a\x02\x84\x01"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSint64{ .value = { 0x42, -2 } } ) == "\x0a\x03\x84\x01\x03"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSint64{ .value = {} } ) == "" );
                    }
                }
            }
            SUBCASE( "fixed32" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqFixed32{ .value = 0x42 } ) == "\x0d\x42\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqFixed32{ .value = 0xff } ) == "\x0d\xff\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqFixed32{ .value = uint32_t( -2 ) } ) == "\x0d\xfe\xff\xff\xff"sv );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptFixed32{ .value = 0x42 } ) == "\x0d\x42\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptFixed32{ .value = 0xff } ) == "\x0d\xff\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptFixed32{ .value = -2 } ) == "\x0d\xfe\xff\xff\xff"sv );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepFixed32{ .value = { 0x42 } } ) == "\x0d\x42\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepFixed32{ .value = { 0x42, 0x3 } } ) == "\x0d\x42\x00\x00\x00\x0d\x03\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepFixed32{ .value = {} } ) == ""sv );

                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackFixed32{ .value = { 0x42 } } ) == "\x0a\x04\x42\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackFixed32{ .value = { 0x42, 0x3 } } ) == "\x0a\x08\x42\x00\x00\x00\x03\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackFixed32{ .value = {} } ) == ""sv );
                    }
                }
            }
            SUBCASE( "fixed64" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqFixed64{ .value = 0x42 } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqFixed64{ .value = 0xff } ) == "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqFixed64{ .value = uint64_t( -2 ) } ) == "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptFixed64{ .value = 0x42 } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptFixed64{ .value = 0xff } ) == "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptFixed64{ .value = -2 } ) == "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepFixed64{ .value = { 0x42 } } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepFixed64{ .value = { 0x42, 0x3 } } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00\x09\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepFixed64{ .value = {} } ) == ""sv );
                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackFixed64{ .value = { 0x42 } } ) == "\x0a\x08\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackFixed64{ .value = { 0x42, 0x3 } } ) == "\x0a\x10\x42\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackFixed64{ .value = {} } ) == ""sv );
                    }
                }
            }
            SUBCASE( "sfixed32" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed32{ .value = 0x42 } ) == "\x0d\x42\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed32{ .value = 0xff } ) == "\x0d\xff\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed32{ .value = -2 } ) == "\x0d\xfe\xff\xff\xff"sv );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed32{ .value = 0x42 } ) == "\x0d\x42\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed32{ .value = 0xff } ) == "\x0d\xff\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed32{ .value = -2 } ) == "\x0d\xfe\xff\xff\xff"sv );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSfixed32{ .value = { 0x42 } } ) == "\x0d\x42\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSfixed32{ .value = { 0x42, 0x3 } } ) == "\x0d\x42\x00\x00\x00\x0d\x03\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSfixed32{ .value = {} } ) == ""sv );

                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSfixed32{ .value = { 0x42 } } ) == "\x0a\x04\x42\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSfixed32{ .value = { 0x42, 0x3 } } ) == "\x0a\x08\x42\x00\x00\x00\x03\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSfixed32{ .value = {} } ) == ""sv );
                    }
                }
            }
            SUBCASE( "sfixed64" )
            {
                SUBCASE( "required" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed64{ .value = 0x42 } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed64{ .value = 0xff } ) == "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::ReqSfixed64{ .value = -2 } ) == "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
                }
                SUBCASE( "optional" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSfixed64{ .value = 0x42 } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSfixed64{ .value = 0xff } ) == "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::OptSfixed64{ .value = -2 } ) == "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
                }
                SUBCASE( "repeated" )
                {
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSfixed64{ .value = { 0x42 } } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSfixed64{ .value = { 0x42, 0x3 } } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00\x09\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( sds::pb::serialize( Test::Scalar::Integers::RepSfixed64{ .value = {} } ) == ""sv );
                    SUBCASE( "packed" )
                    {
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSfixed64{ .value = { 0x42 } } ) == "\x0a\x08\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSfixed64{ .value = { 0x42, 0x3 } } ) == "\x0a\x10\x42\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                        CHECK( sds::pb::serialize( Test::Scalar::Integers::RepPackSfixed64{ .value = {} } ) == ""sv );
                    }
                }
            }
        }
        SUBCASE( "float" )
        {
            SUBCASE( "required" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Floats::ReqFloat{ .value = 42.0 } ) == "\x0d\x00\x00\x28\x42"sv );
            }
            SUBCASE( "optional" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Floats::OptFloat{ .value = 42.3 } ) == "\x0d\x33\x33\x29\x42"sv );
            }
            SUBCASE( "repeated" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Floats::RepFloat{ .value = { 42.3 } } ) == "\x0d\x33\x33\x29\x42"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Floats::RepFloat{ .value = { 42.0, 42.3 } } ) == "\x0d\x00\x00\x28\x42\x0d\x33\x33\x29\x42"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Floats::RepFloat{ .value = {} } ) == ""sv );
            }
        }
        SUBCASE( "double" )
        {
            SUBCASE( "required" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Floats::ReqDouble{ .value = 42.0 } ) == "\x09\x00\x00\x00\x00\x00\x00\x45\x40"sv );
            }
            SUBCASE( "optional" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Floats::OptDouble{ .value = 42.3 } ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40"sv );
            }
            SUBCASE( "repeated" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Floats::RepDouble{ .value = { 42.3 } } ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Floats::RepDouble{ .value = { 42.3, 3.0 } } ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40\x09\x00\x00\x00\x00\x00\x00\x08\x40"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Floats::RepDouble{ .value = {} } ) == ""sv );
            }
        }
        SUBCASE( "bytes" )
        {
            SUBCASE( "required" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::ReqBytes{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::ReqBytes{ .value = to_bytes( "hello" ) } ) == "\x0a\x05hello"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::ReqBytes{ .value = to_bytes( "\x00\x01\x02"sv ) } ) == "\x0a\x03\x00\x01\x02"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::ReqBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) } ) == "\x0a\x05\x00\x01\x02\x03\x04"sv );
            }
            SUBCASE( "optional" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::OptBytes{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::OptBytes{ .value = to_bytes( "hello" ) } ) == "\x0a\x05hello"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::OptBytes{ .value = to_bytes( "\x00\x01\x02"sv ) } ) == "\x0a\x03\x00\x01\x02"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::OptBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) } ) == "\x0a\x05\x00\x01\x02\x03\x04"sv );
            }
            SUBCASE( "repeated" )
            {
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::RepBytes{ } ) == "" );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::RepBytes{ .value = { to_bytes( "hello" ) } } ) == "\x0a\x05hello"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::RepBytes{ .value = { to_bytes( "\x00\x01\x02"sv ) } } ) == "\x0a\x03\x00\x01\x02"sv );
                CHECK( sds::pb::serialize( Test::Scalar::Bytes::RepBytes{ .value = { to_bytes( "\x00\x01\x02\x03\x04"sv ) } } ) == "\x0a\x05\x00\x01\x02\x03\x04"sv );
            }
        }
        SUBCASE( "variant" )
        {
            SUBCASE( "int" )
            {
                CHECK( sds::pb::serialize( Test::Variant{ .oneof_field = 0x42U } ) == "\x08\x42" );
            }
            SUBCASE( "string" )
            {
                CHECK( sds::pb::serialize( Test::Variant{ .oneof_field = "hello" } ) == "\x12\x05hello" );
            }
            SUBCASE( "bytes" )
            {
                CHECK( sds::pb::serialize( Test::Variant{ .oneof_field = std::vector< std::byte >{ std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } } ) == "\x1A\x05hello" );
            }
            SUBCASE( "name" )
            {
                CHECK( sds::pb::serialize( Test::Variant{ .oneof_field = Test::Name{ .name = "John" } } ) == "\x22\x06\x0A\x04John" );
            }
        }
        SUBCASE( "map" )
        {
            SUBCASE( "int32/int32" )
            {
                CHECK( pb_serialize_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ) >( std::map< int32_t, int32_t >{ { 1, 2 } } ) == "\x0a\x04\x08\x01\x10\x02" );
                CHECK( pb_serialize_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ) >( std::map< int32_t, int32_t >{ { 1, 2 }, { 2, 3 } } ) == "\x0a\x08\x08\x01\x10\x02\x08\x02\x10\x03" );
            }
            SUBCASE( "string/string" )
            {
                CHECK( pb_serialize_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ) >( std::map< std::string, std::string >{ { "hello", "world" } } ) == "\x0a\x0e\x0a\x05hello\x12\x05world" );
            }
            SUBCASE( "int32/string" )
            {
                CHECK( pb_serialize_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ) >( std::map< int32_t, std::string >{ { 1, "hello" } } ) == "\x0a\x09\x08\x01\x12\x05hello" );
            }
            SUBCASE( "string/int32" )
            {
                CHECK( pb_serialize_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ) >( std::map< std::string, int32_t >{ { "hello", 2 } } ) == "\x0a\x09\x0a\x05hello\x10\x02" );
            }
            SUBCASE( "string/name" )
            {
                CHECK( pb_serialize_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ) >( std::map< std::string, Test::Name >{ { "hello", { .name = "john" } } } ) == "\x0a\x0f\x0a\x05hello\x12\x06\x0A\x04john" );
            }
        }
        SUBCASE( "person" )
        {
            CHECK( sds::pb::serialize( PhoneBook::Person{
                       .name   = "John Doe",
                       .id     = 123,
                       .email  = "QXUeh@example.com",
                       .phones = {
                           PhoneBook::Person::PhoneNumber{
                               .number = "555-4321",
                               .type   = PhoneBook::Person::PhoneType::HOME,
                           },
                       },
                   } ) == "\x0a\x08John Doe\x10\x7b\x1a\x11QXUeh@example.com\x22\x0c\x0A\x08"
                          "555-4321\x10\x01" );
        }
        SUBCASE( "name" )
        {
            CHECK( sds::pb::serialize( Test::Name{ } ) == "" );
            CHECK( sds::pb::serialize( Test::Name{ }, nullptr ) == 0 );
        }
    }
    SUBCASE( "deserialize" )
    {
        SUBCASE( "string" )
        {
            CHECK( pb_deserialize< wire_type::length_delimited, std::string >( "" ) == "" );
            CHECK( pb_deserialize< wire_type::length_delimited, std::string >( "hello" ) == "hello" );
            CHECK_THROWS( pb_deserialize< wire_type::varint, std::string >( "hello" ) );
            CHECK_THROWS( pb_deserialize< wire_type::fixed32, std::string >( "hello" ) );
            SUBCASE( "optional" )
            {
                CHECK( pb_deserialize< wire_type::length_delimited, std::optional< std::string > >( "hello" ) == "hello" );
            }
        }
        SUBCASE( "string_view" )
        {
            CHECK( pb_deserialize< wire_type::length_delimited, std::string_view >( "" ) == "" );
            CHECK( pb_deserialize< wire_type::length_delimited, std::string_view >( "john" ) == "john"sv );
            SUBCASE( "optional" )
            {
                CHECK( pb_deserialize< wire_type::length_delimited, std::optional< std::string_view > >( "hello" ) == "hello" );
            }
        }
        SUBCASE( "bool" )
        {
            CHECK( pb_deserialize_as< scalar_encoder::varint, bool >( "\x01" ) == true );
            CHECK( pb_deserialize_as< scalar_encoder::varint, bool >( "\x00"sv ) == false );
            CHECK_THROWS( pb_deserialize_as< scalar_encoder::varint, bool >( "\x02"sv ) );
            CHECK_THROWS( pb_deserialize_as< scalar_encoder::varint, bool >( "\xff\x01"sv ) );

            SUBCASE( "optional" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::varint, std::optional< bool > >( "\x01" ) == true );
                CHECK( pb_deserialize_as< scalar_encoder::varint, std::optional< bool > >( "\x00"sv ) == false );
            }
            SUBCASE( "array" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::varint, std::vector< bool > >( "\x01" ) == std::vector< bool >{ true } );
                CHECK( pb_deserialize_as< scalar_encoder::varint, std::vector< bool > >( "\x00"sv ) == std::vector< bool >{ false } );
                SUBCASE( "packed" )
                {
                    CHECK( pb_deserialize_as< combine( scalar_encoder::varint, scalar_encoder::packed ), std::vector< bool > >( "\x01" ) == std::vector< bool >{ true } );
                    CHECK( pb_deserialize_as< combine( scalar_encoder::varint, scalar_encoder::packed ), std::vector< bool > >( "\x01\x00"sv ) == std::vector< bool >{ true, false } );
                }
            }
        }
        SUBCASE( "int" )
        {
            SUBCASE( "varint" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::varint, int >( "\x42" ) == 0x42 );
                CHECK( pb_deserialize_as< scalar_encoder::varint, int >( "\xff\x01" ) == 0xff );
                CHECK_THROWS( pb_deserialize_as< scalar_encoder::varint, int >( "\xff" ) );
                CHECK_THROWS( pb_deserialize_as< scalar_encoder::varint, int >( "\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
                CHECK( pb_deserialize_as< scalar_encoder::varint, int32_t >( "\xfe\xff\xff\xff\x0f" ) == -2 );
                CHECK( pb_deserialize_as< scalar_encoder::varint, int64_t >( "\xfe\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) == -2 );

                SUBCASE( "optional" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::varint, std::optional< int > >( "\x42" ) == 0x42 );
                }
                SUBCASE( "array" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::varint, std::vector< int > >( "\x42" ) == std::vector< int >{ 0x42 } );

                    SUBCASE( "packed" )
                    {
                        CHECK( pb_deserialize_as< combine( scalar_encoder::varint, scalar_encoder::packed ), std::vector< int > >( "\x42" ) == std::vector< int >{ 0x42 } );
                        CHECK( pb_deserialize_as< combine( scalar_encoder::varint, scalar_encoder::packed ), std::vector< int > >( "\x42\x03" ) == std::vector< int >{ 0x42, 0x3 } );
                    }
                }
            }
            SUBCASE( "svarint" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::svarint, int >( "\x84\x01" ) == 0x42 );
                CHECK( pb_deserialize_as< scalar_encoder::svarint, int >( "\xfe\x03" ) == 0xff );
                CHECK( pb_deserialize_as< scalar_encoder::svarint, int32_t >( "\x03" ) == -2 );
                CHECK( pb_deserialize_as< scalar_encoder::svarint, int64_t >( "\x03" ) == -2 );

                SUBCASE( "optional" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::svarint, std::optional< int > >( "\x84\x01" ) == 0x42 );
                }
                SUBCASE( "array" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::svarint, std::vector< int > >( "\x84\x01" ) == std::vector< int >{ 0x42 } );

                    SUBCASE( "packed" )
                    {
                        CHECK( pb_deserialize_as< combine( scalar_encoder::svarint, scalar_encoder::packed ), std::vector< int > >( "\x84\x01" ) == std::vector< int >{ 0x42 } );
                        CHECK( pb_deserialize_as< combine( scalar_encoder::svarint, scalar_encoder::packed ), std::vector< int > >( "\x84\x01\x03" ) == std::vector< int >{ 0x42, -2 } );
                    }
                }
            }
            SUBCASE( "i32" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::i32, int >( "\x42\x00\x00\x00"sv ) == 0x42 );
                CHECK_THROWS( pb_deserialize_as< scalar_encoder::i32, int >( "\x42\x00\x00"sv ) );
                CHECK( pb_deserialize_as< scalar_encoder::i32, int >( "\xff\x00\x00\x00"sv ) == 0xff );
                CHECK( pb_deserialize_as< scalar_encoder::i32, int32_t >( "\xfe\xff\xff\xff" ) == -2 );
                SUBCASE( "optional" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::i32, std::optional< int > >( "\x42\x00\x00\x00"sv ) == 0x42 );
                }
                SUBCASE( "array" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::i32, std::vector< int > >( "\x42\x00\x00\x00"sv ) == std::vector< int >{ 0x42 } );
                    SUBCASE( "packed" )
                    {
                        CHECK( pb_deserialize_as< combine( scalar_encoder::i32, scalar_encoder::packed ), std::vector< int > >( "\x42\x00\x00\x00"sv ) == std::vector< int >{ 0x42 } );
                        CHECK( pb_deserialize_as< combine( scalar_encoder::i32, scalar_encoder::packed ), std::vector< int > >( "\x42\x00\x00\x00\x03\x00\x00\x00"sv ) == std::vector< int >{ 0x42, 0x3 } );
                    }
                }
            }
            SUBCASE( "i64" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::i64, int64_t >( "\x42\x00\x00\x00\x00\x00\x00\x00"sv ) == 0x42 );
                CHECK( pb_deserialize_as< scalar_encoder::i64, uint64_t >( "\xff\x00\x00\x00\x00\x00\x00\x00"sv ) == 0xff );
                CHECK( pb_deserialize_as< scalar_encoder::i64, int64_t >( "\xfe\xff\xff\xff\xff\xff\xff\xff"sv ) == int64_t( -2 ) );
                SUBCASE( "optional" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::i64, std::optional< int64_t > >( "\x42\x00\x00\x00\x00\x00\x00\x00"sv ) == 0x42 );
                }
                SUBCASE( "array" )
                {
                    CHECK( pb_deserialize_as< scalar_encoder::i64, std::vector< int64_t > >( "\x42\x00\x00\x00\x00\x00\x00\x00"sv ) == std::vector< int64_t >{ 0x42 } );
                    SUBCASE( "packed" )
                    {
                        CHECK( pb_deserialize_as< combine( scalar_encoder::i64, scalar_encoder::packed ), std::vector< int64_t > >( "\x42\x00\x00\x00\x00\x00\x00\x00"sv ) == std::vector< int64_t >{ 0x42 } );
                        CHECK( pb_deserialize_as< combine( scalar_encoder::i64, scalar_encoder::packed ), std::vector< int64_t > >( "\x42\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00"sv ) == std::vector< int64_t >{ 0x42, 0x3 } );
                    }
                }
            }
        }
        SUBCASE( "double" )
        {
            CHECK( pb_deserialize_as< scalar_encoder::i64, double >( "\x00\x00\x00\x00\x00\x00\x45\x40"sv ) == 42.0 );
            CHECK_THROWS( pb_deserialize_as< scalar_encoder::i64, double >( "\x00\x00\x00\x00\x00\x00\x45"sv ) );
            SUBCASE( "optional" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::i64, std::optional< double > >( "\x66\x66\x66\x66\x66\x26\x45\x40" ) == 42.3 );
            }
            SUBCASE( "array" )
            {
                CHECK( pb_deserialize_as< scalar_encoder::i64, std::vector< double > >( "\x66\x66\x66\x66\x66\x26\x45\x40" ) == std::vector< double >{ 42.3 } );
            }
        }
        SUBCASE( "bytes" )
        {
            CHECK( pb_deserialize< wire_type::length_delimited, std::vector< std::byte > >( "\x00\x01\x02"sv ) == std::vector< std::byte >{ std::byte{ 0 }, std::byte{ 1 }, std::byte{ 2 } } );

            CHECK( pb_deserialize< wire_type::length_delimited, std::vector< std::byte > >( "\x00\x01\x02\x03\x04"sv ) == std::vector< std::byte >( { std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) } ) );
            CHECK( pb_deserialize< wire_type::length_delimited, std::vector< std::byte > >( "hello"sv ) == std::vector< std::byte >( { std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } ) );

            SUBCASE( "array" )
            {
                CHECK( pb_deserialize< wire_type::length_delimited, std::vector< std::vector< std::byte > > >( "\x00\x01\x02\x03\x04"sv ) == std::vector< std::vector< std::byte > >{ { std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) } } );
            }
            SUBCASE( "optional" )
            {
                CHECK( pb_deserialize< wire_type::length_delimited, std::optional< std::vector< std::byte > > >( "\x00\x01\x02\x03\x04"sv ) == std::vector< std::byte >{ std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) } );
                CHECK( pb_deserialize< wire_type::length_delimited, std::optional< std::vector< std::byte > > >( "hello"sv ) == std::vector< std::byte >{ std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } );
            }
        }
        SUBCASE( "variant" )
        {
            SUBCASE( "int" )
            {
                CHECK( sds::pb::deserialize< Test::Variant >( "\x08\x42" ) == Test::Variant{ .oneof_field = 0x42U } );
            }
            SUBCASE( "string" )
            {
                CHECK( sds::pb::deserialize< Test::Variant >( "\x12\x05hello" ) == Test::Variant{ .oneof_field = "hello" } );
                CHECK_THROWS( sds::pb::deserialize< Test::Variant >( "\x12\x05hell" ) );
            }
            SUBCASE( "bytes" )
            {
                CHECK( sds::pb::deserialize< Test::Variant >( "\x1A\x05hello" ) == Test::Variant{ .oneof_field = std::vector< std::byte >{ std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } } );
                CHECK_THROWS( sds::pb::deserialize< Test::Variant >( "\x1A\x05hell" ) );
            }
            SUBCASE( "name" )
            {
                CHECK( sds::pb::deserialize< Test::Variant >( "\x22\x06\x0A\x04John" ) == Test::Variant{ .oneof_field = Test::Name{ .name = "John" } } );
                CHECK_THROWS( sds::pb::deserialize< Test::Variant >( "\x22\x06\x0A\x04Joh" ) );
            }
        }
        SUBCASE( "map" )
        {
            SUBCASE( "int32/int32" )
            {
                CHECK( pb_deserialize_map_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ), std::map< int32_t, int32_t > >( "\x08\x01\x10\x02" ) == std::map< int32_t, int32_t >{ { 1, 2 } } );
            }
            SUBCASE( "string/string" )
            {
                CHECK( pb_deserialize_map_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ), std::map< std::string, std::string > >( "\x0a\x05hello\x12\x05world" ) == std::map< std::string, std::string >{ { "hello", "world" } } );
            }
            SUBCASE( "int32/string" )
            {
                CHECK( pb_deserialize_map_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ), std::map< int32_t, std::string > >( "\x08\x01\x12\x05hello" ) == std::map< int32_t, std::string >{ { 1, "hello" } } );
            }
            SUBCASE( "string/int32" )
            {
                CHECK( pb_deserialize_map_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ), std::map< std::string, int32_t > >( "\x0a\x05hello\x10\x02" ) == std::map< std::string, int32_t >{ { "hello", 2 } } );
            }
            SUBCASE( "string/name" )
            {
                CHECK( pb_deserialize_map_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ), std::map< std::string, Test::Name > >( "\x0a\x05hello\x12\x06\x0A\x04john" ) == std::map< std::string, Test::Name >{ { "hello", { .name = "john" } } } );
            }
        }
        SUBCASE( "person" )
        {
            const auto person = sds::pb::deserialize< PhoneBook::Person >( "\x0a\x08John Doe\x10\x7b\x1a\x11QXUeh@example.com\x22\x0c\x0A\x08"
                                                                           "555-4321\x10\x01" );
            CHECK( person == PhoneBook::Person{
                                 .name   = "John Doe",
                                 .id     = 123,
                                 .email  = "QXUeh@example.com",
                                 .phones = {
                                     PhoneBook::Person::PhoneNumber{
                                         .number = "555-4321",
                                         .type   = PhoneBook::Person::PhoneType::HOME,
                                     },
                                 },
                             } );

            auto person2 = PhoneBook::Person{ };
            CHECK_NOTHROW( sds::pb::deserialize( person2, "\x0a\x08John Doe\x10\x7b\x1a\x11QXUeh@example.com\x22\x0c\x0A\x08"
                                                          "555-4321\x10\x01"sv ) );
            CHECK( person2 == PhoneBook::Person{
                                  .name   = "John Doe",
                                  .id     = 123,
                                  .email  = "QXUeh@example.com",
                                  .phones = {
                                      PhoneBook::Person::PhoneNumber{
                                          .number = "555-4321",
                                          .type   = PhoneBook::Person::PhoneType::HOME,
                                      },
                                  },
                              } );
            CHECK_THROWS( sds::pb::deserialize( person2, "\x0a\x08John Doe\x10\x7b\x1a\x11QXUeh@example.com\x22\x0c\x0A\x08"sv ) );
        }
    }
}
