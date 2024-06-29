#include "spb/pb/wire-types.h"
#include <cstdint>
#include <memory>
#include <name.pb.h>
#include <optional>
#include <person.pb.h>
#include <scalar.pb.h>
#include <span>
#include <spb/pb/deserialize.hpp>
#include <spb/pb/serialize.hpp>
#include <string>
#include <string_view>
#include <type_traits>

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

namespace Scalar
{
template < typename T >
concept HasValueMember = requires( T t ) {
    {
        t.value
    };
};

template < typename T >
    requires HasValueMember< T >
auto operator==( const T & lhs, const T & rhs ) noexcept -> bool
{
    return lhs.value == rhs.value;
}
}// namespace Scalar
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
    auto size_stream = spb::pb::detail::ostream( nullptr );
    spb::pb::detail::serialize( size_stream, 1, value );
    const auto size = size_stream.size( );
    auto result     = std::string( size, '\0' );
    auto stream     = spb::pb::detail::ostream( result.data( ) );
    spb::pb::detail::serialize( stream, 1, value );
    return result;
}

template < spb::pb::detail::scalar_encoder encoder, typename T >
auto pb_serialize_as( const T & value ) -> std::string
{
    auto size_stream = spb::pb::detail::ostream( nullptr );
    spb::pb::detail::serialize_as< encoder >( size_stream, 1, value );
    const auto size = size_stream.size( );
    auto result     = std::string( size, '\0' );
    auto stream     = spb::pb::detail::ostream( result.data( ) );
    spb::pb::detail::serialize_as< encoder >( stream, 1, value );
    return result;
}

template < typename T >
void pb_test( const T & value, std::string_view protobuf )
{
    {
        auto serialized = spb::pb::serialize( value );
        CHECK( serialized == protobuf );
    }

    {
        auto size   = spb::pb::serialize_size( value );
        auto buffer = std::string( size, '\0' );
        spb::pb::serialize( value, buffer.data( ) );
        CHECK( buffer == protobuf );
    }

    {
        auto deserialized = spb::pb::deserialize< T >( protobuf );
        CHECK( deserialized == value );
    }
    {
        auto deserialized = T( );
        spb::pb::deserialize( deserialized, protobuf );
        CHECK( deserialized == value );
    }
}

using spb::pb::detail::scalar_encoder;
using spb::pb::detail::wire_type;

}// namespace
using namespace std::literals;

TEST_CASE( "protobuf" )
{
    SUBCASE( "string" )
    {
        SUBCASE( "required" )
        {
            pb_test( Test::Scalar::ReqString{ }, "" );
            pb_test( Test::Scalar::ReqString{ .value = "hello" }, "\x0a\x05hello" );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqString >( "\x0a\x05hell"sv ) );
        }
        SUBCASE( "optional" )
        {
            pb_test( Test::Scalar::OptString{ }, "" );
            pb_test( Test::Scalar::OptString{ .value = "hello" }, "\x0a\x05hello" );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::OptString >( "\x08\x05hello"sv ) );
        }
        SUBCASE( "repeated" )
        {
            pb_test( Test::Scalar::RepString{ }, "" );
            pb_test( Test::Scalar::RepString{ .value = { "hello" } }, "\x0a\x05hello" );
            pb_test( Test::Scalar::RepString{ .value = { "hello", "world" } }, "\x0a\x05hello\x0a\x05world" );
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
            pb_test( Test::Scalar::ReqBool{ }, "\x08\x00"sv );
            pb_test( Test::Scalar::ReqBool{ .value = true }, "\x08\x01" );
            pb_test( Test::Scalar::ReqBool{ .value = false }, "\x08\x00"sv );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqBool >( "\x08\x02"sv ) );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqBool >( "\x08\xff\x01"sv ) );
        }
        SUBCASE( "optional" )
        {
            pb_test( Test::Scalar::OptBool{ }, "" );
            pb_test( Test::Scalar::OptBool{ .value = true }, "\x08\x01" );
            pb_test( Test::Scalar::OptBool{ .value = false }, "\x08\x00"sv );
        }
        SUBCASE( "repeated" )
        {
            pb_test( Test::Scalar::RepBool{ }, "" );
            pb_test( Test::Scalar::RepBool{ .value = { true } }, "\x08\x01" );
            pb_test( Test::Scalar::RepBool{ .value = { true, false } }, "\x08\x01\x08\x00"sv );
            pb_test( Test::Scalar::RepBool{ .value = {} }, "" );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepBool >( "\x08\x01\x08"sv ) );
        }
    }
    SUBCASE( "int" )
    {
        SUBCASE( "varint32" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqInt32{ .value = 0x42 }, "\x08\x42" );
                pb_test( Test::Scalar::ReqInt32{ .value = 0xff }, "\x08\xff\x01" );
                pb_test( Test::Scalar::ReqInt32{ .value = -2 }, "\x08\xfe\xff\xff\xff\x0f"sv );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::OptInt32{ }, "" );
                pb_test( Test::Scalar::OptInt32{ .value = 0x42 }, "\x08\x42" );
                pb_test( Test::Scalar::OptInt32{ .value = 0xff }, "\x08\xff\x01" );
                pb_test( Test::Scalar::OptInt32{ .value = -2 }, "\x08\xfe\xff\xff\xff\x0f"sv );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepInt32{ }, "" );
                pb_test( Test::Scalar::RepInt32{ .value = { 0x42 } }, "\x08\x42" );
                pb_test( Test::Scalar::RepInt32{ .value = { 0x42, 0x3 } }, "\x08\x42\x08\x03" );
                pb_test( Test::Scalar::RepInt32{ .value = {} }, "" );

                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackInt32{ }, "" );
                    pb_test( Test::Scalar::RepPackInt32{ .value = { 0x42 } }, "\x0a\x01\x42" );
                    pb_test( Test::Scalar::RepPackInt32{ .value = { 0x42, 0x3 } }, "\x0a\x02\x42\x03" );
                    pb_test( Test::Scalar::RepPackInt32{ .value = {} }, "" );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackInt32 >( "\x0a\x02\x42" ) );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackInt32 >( "\x0a\x02\x42\xff" ) );
                }
            }
        }
        SUBCASE( "varint64" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqInt64{ .value = 0x42 }, "\x08\x42" );
                pb_test( Test::Scalar::ReqInt64{ .value = 0xff }, "\x08\xff\x01" );
                pb_test( Test::Scalar::ReqInt64{ .value = -2 }, "\x08\xfe\xff\xff\xff\xff\xff\xff\xff\xff\x01" );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::OptInt64{ }, "" );
                pb_test( Test::Scalar::OptInt64{ .value = 0x42 }, "\x08\x42" );
                pb_test( Test::Scalar::OptInt64{ .value = 0xff }, "\x08\xff\x01" );
                pb_test( Test::Scalar::OptInt64{ .value = -2 }, "\x08\xfe\xff\xff\xff\xff\xff\xff\xff\xff\x01" );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepInt64{ }, "" );
                pb_test( Test::Scalar::RepInt64{ .value = { 0x42 } }, "\x08\x42" );
                pb_test( Test::Scalar::RepInt64{ .value = { 0x42, 0x3 } }, "\x08\x42\x08\x03" );
                pb_test( Test::Scalar::RepInt64{ .value = {} }, "" );

                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackInt64{ }, "" );
                    pb_test( Test::Scalar::RepPackInt64{ .value = { 0x42 } }, "\x0a\x01\x42" );
                    pb_test( Test::Scalar::RepPackInt64{ .value = { 0x42, 0x3 } }, "\x0a\x02\x42\x03" );
                    pb_test( Test::Scalar::RepPackInt64{ .value = {} }, "" );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackInt64 >( "\x0a\x02\x42" ) );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackInt64 >( "\x0a\x02\x42\xff" ) );
                }
            }
        }
        SUBCASE( "svarint32" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqSint32{ .value = 0x42 }, "\x08\x84\x01"sv );
                pb_test( Test::Scalar::ReqSint32{ .value = 0xff }, "\x08\xfe\x03"sv );
                pb_test( Test::Scalar::ReqSint32{ .value = -2 }, "\x08\x03"sv );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSint32 >( "\x08" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSint32 >( "\x08\xff" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSint32 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::OptSint32{ }, "" );
                pb_test( Test::Scalar::OptSint32{ .value = 0x42 }, "\x08\x84\x01"sv );
                pb_test( Test::Scalar::OptSint32{ .value = 0xff }, "\x08\xfe\x03"sv );
                pb_test( Test::Scalar::OptSint32{ .value = -2 }, "\x08\x03"sv );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepSint32{ }, "" );
                pb_test( Test::Scalar::RepSint32{ .value = { 0x42 } }, "\x08\x84\x01"sv );
                pb_test( Test::Scalar::RepSint32{ .value = { 0x42, -2 } }, "\x08\x84\x01\x08\x03"sv );
                pb_test( Test::Scalar::RepSint32{ .value = {} }, "" );

                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackSint32{ .value = { 0x42 } }, "\x0a\x02\x84\x01"sv );
                    pb_test( Test::Scalar::RepPackSint32{ .value = { 0x42, -2 } }, "\x0a\x03\x84\x01\x03"sv );
                    pb_test( Test::Scalar::RepPackSint32{ .value = {} }, "" );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackSint32 >( "\x0a\x02\x42" ) );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackSint32 >( "\x0a\x02\x42\xff" ) );
                }
            }
        }
        SUBCASE( "svarint64" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqSint64{ .value = 0x42 }, "\x08\x84\x01"sv );
                pb_test( Test::Scalar::ReqSint64{ .value = 0xff }, "\x08\xfe\x03"sv );
                pb_test( Test::Scalar::ReqSint64{ .value = -2 }, "\x08\x03"sv );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSint64 >( "\x08" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSint64 >( "\x08\xff" ) );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSint64 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::OptSint64{ }, "" );
                pb_test( Test::Scalar::OptSint64{ .value = 0x42 }, "\x08\x84\x01"sv );
                pb_test( Test::Scalar::OptSint64{ .value = 0xff }, "\x08\xfe\x03"sv );
                pb_test( Test::Scalar::OptSint64{ .value = -2 }, "\x08\x03"sv );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepSint64{ }, "" );
                pb_test( Test::Scalar::RepSint64{ .value = { 0x42 } }, "\x08\x84\x01"sv );
                pb_test( Test::Scalar::RepSint64{ .value = { 0x42, -2 } }, "\x08\x84\x01\x08\x03"sv );
                pb_test( Test::Scalar::RepSint64{ .value = {} }, "" );

                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackSint64{ .value = { 0x42 } }, "\x0a\x02\x84\x01"sv );
                    pb_test( Test::Scalar::RepPackSint64{ .value = { 0x42, -2 } }, "\x0a\x03\x84\x01\x03"sv );
                    pb_test( Test::Scalar::RepPackSint64{ .value = {} }, "" );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackSint64 >( "\x0a\x02\x84"sv ) );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackSint64 >( "\x0a\x03\x84\x01"sv ) );
                }
            }
        }
        SUBCASE( "fixed32" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqFixed32{ .value = 0x42 }, "\x0d\x42\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqFixed32{ .value = 0xff }, "\x0d\xff\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqFixed32{ .value = uint32_t( -2 ) }, "\x0d\xfe\xff\xff\xff"sv );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqFixed32 >( "\x0d\x42\x00\x00"sv ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::OptFixed32{ .value = 0x42 }, "\x0d\x42\x00\x00\x00"sv );
                pb_test( Test::Scalar::OptFixed32{ .value = 0xff }, "\x0d\xff\x00\x00\x00"sv );
                pb_test( Test::Scalar::OptFixed32{ .value = -2 }, "\x0d\xfe\xff\xff\xff"sv );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepFixed32{ .value = { 0x42 } }, "\x0d\x42\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepFixed32{ .value = { 0x42, 0x3 } }, "\x0d\x42\x00\x00\x00\x0d\x03\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepFixed32{ .value = {} }, ""sv );

                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackFixed32{ .value = { 0x42 } }, "\x0a\x04\x42\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackFixed32{ .value = { 0x42, 0x3 } }, "\x0a\x08\x42\x00\x00\x00\x03\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackFixed32{ .value = {} }, ""sv );
                    CHECK_THROWS( spb::pb::deserialize< Test::Scalar::RepPackFixed32 >( "\x0a\x08\x42\x00\x00\x00\x03\x00\x00"sv ) );
                }
            }
        }
        SUBCASE( "fixed64" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqFixed64{ .value = 0x42 }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqFixed64{ .value = 0xff }, "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqFixed64{ .value = uint64_t( -2 ) }, "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqFixed32 >( "\x09\x42\x00\x00\x00\x00\x00\x00"sv ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::OptFixed64{ .value = 0x42 }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::OptFixed64{ .value = 0xff }, "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::OptFixed64{ .value = -2 }, "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepFixed64{ .value = { 0x42 } }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepFixed64{ .value = { 0x42, 0x3 } }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00\x09\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepFixed64{ .value = {} }, ""sv );
                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackFixed64{ .value = { 0x42 } }, "\x0a\x08\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackFixed64{ .value = { 0x42, 0x3 } }, "\x0a\x10\x42\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackFixed64{ .value = {} }, ""sv );
                }
            }
        }
        SUBCASE( "sfixed32" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqSfixed32{ .value = 0x42 }, "\x0d\x42\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqSfixed32{ .value = 0xff }, "\x0d\xff\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqSfixed32{ .value = -2 }, "\x0d\xfe\xff\xff\xff"sv );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSfixed32 >( "\x0d\x42\x00\x00"sv ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::ReqSfixed32{ .value = 0x42 }, "\x0d\x42\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqSfixed32{ .value = 0xff }, "\x0d\xff\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqSfixed32{ .value = -2 }, "\x0d\xfe\xff\xff\xff"sv );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepSfixed32{ .value = { 0x42 } }, "\x0d\x42\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepSfixed32{ .value = { 0x42, 0x3 } }, "\x0d\x42\x00\x00\x00\x0d\x03\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepSfixed32{ .value = {} }, ""sv );

                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackSfixed32{ .value = { 0x42 } }, "\x0a\x04\x42\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackSfixed32{ .value = { 0x42, 0x3 } }, "\x0a\x08\x42\x00\x00\x00\x03\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackSfixed32{ .value = {} }, ""sv );
                }
            }
        }
        SUBCASE( "sfixed64" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqSfixed64{ .value = 0x42 }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqSfixed64{ .value = 0xff }, "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::ReqSfixed64{ .value = -2 }, "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
                CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqSfixed64 >( "\x09\x42\x00\x00\x00\x00\x00\x00"sv ) );
            }
            SUBCASE( "optional" )
            {
                pb_test( Test::Scalar::OptSfixed64{ .value = 0x42 }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::OptSfixed64{ .value = 0xff }, "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::OptSfixed64{ .value = -2 }, "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
            }
            SUBCASE( "repeated" )
            {
                pb_test( Test::Scalar::RepSfixed64{ .value = { 0x42 } }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepSfixed64{ .value = { 0x42, 0x3 } }, "\x09\x42\x00\x00\x00\x00\x00\x00\x00\x09\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                pb_test( Test::Scalar::RepSfixed64{ .value = {} }, ""sv );
                SUBCASE( "packed" )
                {
                    pb_test( Test::Scalar::RepPackSfixed64{ .value = { 0x42 } }, "\x0a\x08\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackSfixed64{ .value = { 0x42, 0x3 } }, "\x0a\x10\x42\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                    pb_test( Test::Scalar::RepPackSfixed64{ .value = {} }, ""sv );
                }
            }
        }
    }
    SUBCASE( "float" )
    {
        SUBCASE( "required" )
        {
            pb_test( Test::Scalar::ReqFloat{ .value = 42.0f }, "\x0d\x00\x00\x28\x42"sv );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqFloat >( "\x0d\x00\x00\x28"sv ) );
        }
        SUBCASE( "optional" )
        {
            pb_test( Test::Scalar::OptFloat{ .value = 42.3f }, "\x0d\x33\x33\x29\x42"sv );
        }
        SUBCASE( "repeated" )
        {
            pb_test( Test::Scalar::RepFloat{ .value = { 42.3f } }, "\x0d\x33\x33\x29\x42"sv );
            pb_test( Test::Scalar::RepFloat{ .value = { 42.0f, 42.3f } }, "\x0d\x00\x00\x28\x42\x0d\x33\x33\x29\x42"sv );
            pb_test( Test::Scalar::RepFloat{ .value = {} }, ""sv );
        }
    }
    SUBCASE( "double" )
    {
        SUBCASE( "required" )
        {
            pb_test( Test::Scalar::ReqDouble{ .value = 42.0 }, "\x09\x00\x00\x00\x00\x00\x00\x45\x40"sv );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqDouble >( "\x0d\x00\x00\x28"sv ) );
        }
        SUBCASE( "optional" )
        {
            pb_test( Test::Scalar::OptDouble{ .value = 42.3 }, "\x09\x66\x66\x66\x66\x66\x26\x45\x40"sv );
        }
        SUBCASE( "repeated" )
        {
            pb_test( Test::Scalar::RepDouble{ .value = { 42.3 } }, "\x09\x66\x66\x66\x66\x66\x26\x45\x40"sv );
            pb_test( Test::Scalar::RepDouble{ .value = { 42.3, 3.0 } }, "\x09\x66\x66\x66\x66\x66\x26\x45\x40\x09\x00\x00\x00\x00\x00\x00\x08\x40"sv );
            pb_test( Test::Scalar::RepDouble{ .value = {} }, ""sv );
        }
    }
    SUBCASE( "bytes" )
    {
        SUBCASE( "required" )
        {
            pb_test( Test::Scalar::ReqBytes{ }, "" );
            pb_test( Test::Scalar::ReqBytes{ .value = to_bytes( "hello" ) }, "\x0a\x05hello"sv );
            pb_test( Test::Scalar::ReqBytes{ .value = to_bytes( "\x00\x01\x02"sv ) }, "\x0a\x03\x00\x01\x02"sv );
            pb_test( Test::Scalar::ReqBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) }, "\x0a\x05\x00\x01\x02\x03\x04"sv );
            CHECK_THROWS( spb::pb::deserialize< Test::Scalar::ReqBytes >( "\x0a\x05hell"sv ) );
        }
        SUBCASE( "optional" )
        {
            pb_test( Test::Scalar::OptBytes{ }, "" );
            pb_test( Test::Scalar::OptBytes{ .value = to_bytes( "hello" ) }, "\x0a\x05hello"sv );
            pb_test( Test::Scalar::OptBytes{ .value = to_bytes( "\x00\x01\x02"sv ) }, "\x0a\x03\x00\x01\x02"sv );
            pb_test( Test::Scalar::OptBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) }, "\x0a\x05\x00\x01\x02\x03\x04"sv );
        }
        SUBCASE( "repeated" )
        {
            pb_test( Test::Scalar::RepBytes{ }, "" );
            pb_test( Test::Scalar::RepBytes{ .value = { to_bytes( "hello" ) } }, "\x0a\x05hello"sv );
            pb_test( Test::Scalar::RepBytes{ .value = { to_bytes( "\x00\x01\x02"sv ) } }, "\x0a\x03\x00\x01\x02"sv );
            pb_test( Test::Scalar::RepBytes{ .value = { to_bytes( "\x00\x01\x02\x03\x04"sv ) } }, "\x0a\x05\x00\x01\x02\x03\x04"sv );
        }
    }
    SUBCASE( "variant" )
    {
        SUBCASE( "int" )
        {
            pb_test( Test::Variant{ .oneof_field = 0x42U }, "\x08\x42" );
        }
        SUBCASE( "string" )
        {
            pb_test( Test::Variant{ .oneof_field = "hello" }, "\x12\x05hello" );
        }
        SUBCASE( "bytes" )
        {
            pb_test( Test::Variant{ .oneof_field = to_bytes( "hello" ) }, "\x1A\x05hello" );
        }
        SUBCASE( "name" )
        {
            pb_test( Test::Variant{ .oneof_field = Test::Name{ .name = "John" } }, "\x22\x06\x0A\x04John" );
        }
    }
    SUBCASE( "map" )
    {
        SUBCASE( "int32/int32" )
        {
            CHECK( pb_serialize_as< combine( spb::pb::detail::scalar_encoder::varint, spb::pb::detail::scalar_encoder::varint ) >( std::map< int32_t, int32_t >{ { 1, 2 } } ) == "\x0a\x04\x08\x01\x10\x02" );
            CHECK( pb_serialize_as< combine( spb::pb::detail::scalar_encoder::varint, spb::pb::detail::scalar_encoder::varint ) >( std::map< int32_t, int32_t >{ { 1, 2 }, { 2, 3 } } ) == "\x0a\x08\x08\x01\x10\x02\x08\x02\x10\x03" );
        }
        SUBCASE( "string/string" )
        {
            CHECK( pb_serialize_as< combine( spb::pb::detail::scalar_encoder::varint, spb::pb::detail::scalar_encoder::varint ) >( std::map< std::string, std::string >{ { "hello", "world" } } ) == "\x0a\x0e\x0a\x05hello\x12\x05world" );
        }
        SUBCASE( "int32/string" )
        {
            CHECK( pb_serialize_as< combine( spb::pb::detail::scalar_encoder::varint, spb::pb::detail::scalar_encoder::varint ) >( std::map< int32_t, std::string >{ { 1, "hello" } } ) == "\x0a\x09\x08\x01\x12\x05hello" );
        }
        SUBCASE( "string/int32" )
        {
            CHECK( pb_serialize_as< combine( spb::pb::detail::scalar_encoder::varint, spb::pb::detail::scalar_encoder::varint ) >( std::map< std::string, int32_t >{ { "hello", 2 } } ) == "\x0a\x09\x0a\x05hello\x10\x02" );
        }
        SUBCASE( "string/name" )
        {
            CHECK( pb_serialize_as< combine( spb::pb::detail::scalar_encoder::varint, spb::pb::detail::scalar_encoder::varint ) >( std::map< std::string, Test::Name >{ { "hello", { .name = "john" } } } ) == "\x0a\x0f\x0a\x05hello\x12\x06\x0A\x04john" );
        }
    }
    SUBCASE( "person" )
    {
        pb_test( PhoneBook::Person{
                     .name   = "John Doe",
                     .id     = 123,
                     .email  = "QXUeh@example.com",
                     .phones = {
                         PhoneBook::Person::PhoneNumber{
                             .number = "555-4321",
                             .type   = PhoneBook::Person::PhoneType::HOME,
                         },
                     },
                 },
                 "\x0a\x08John Doe\x10\x7b\x1a\x11QXUeh@example.com\x22\x0c\x0A\x08"
                 "555-4321\x10\x01" );
    }
    SUBCASE( "name" )
    {
        pb_test( Test::Name{ }, "" );
    }
}
