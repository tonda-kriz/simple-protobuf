#include <cstdint>
#include <google/protobuf/util/json_util.h>
#include <gpb-name.pb.h>
#include <gpb-person.pb.h>
#include <gpb-scalar.pb.h>
#include <name.pb.h>
#include <optional>
#include <person.pb.h>
#include <scalar.pb.h>
#include <span>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace std
{
auto operator==( const string & lhs, const vector< byte > & rhs ) noexcept -> bool
{
    return lhs.size( ) == rhs.size( ) && ( memcmp( lhs.data( ), rhs.data( ), lhs.size( ) ) == 0 );
}
}// namespace std

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
concept is_gpb_repeated = requires( T t ) {
    {
        t.value( 0 )
    };
};

template < typename T >
auto opt_size( const std::optional< T > & opt ) -> std::size_t
{
    if( opt.has_value( ) )
    {
        return opt.value( ).size( );
    }
    return 0;
}

template < typename T >
auto opt_size( const std::vector< T > & opt ) -> std::size_t
{
    return opt.size( );
}

template < typename GPB, typename SPB >
void gpb_test( const SPB & spb )
{
    auto gpb            = GPB( );
    auto spb_serialized = spb::pb::serialize( spb );

    REQUIRE( gpb.ParseFromString( spb_serialized ) );
    if constexpr( is_gpb_repeated< GPB > )
    {
        REQUIRE( gpb.value( ).size( ) == opt_size( spb.value ) );
        for( size_t i = 0; i < opt_size( spb.value ); ++i )
        {
            REQUIRE( gpb.value( i ) == spb.value[ i ] );
        }
    }
    else
    {
        REQUIRE( spb.value == gpb.value( ) );
    }
    auto gpb_serialized = std::string( );
    REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
    REQUIRE( spb::pb::deserialize< SPB >( gpb_serialized ) == spb );
}

template < typename GPB, typename SPB >
void gpb_json( const SPB & spb )
{
    auto gpb            = GPB( );
    auto spb_serialized = spb::json::serialize( spb );

    auto parse_options = google::protobuf::util::JsonParseOptions{ };
    ( void ) JsonStringToMessage( spb_serialized, &gpb, parse_options );

    if constexpr( is_gpb_repeated< GPB > )
    {
        REQUIRE( gpb.value( ).size( ) == opt_size( spb.value ) );
        for( size_t i = 0; i < opt_size( spb.value ); ++i )
        {
            REQUIRE( gpb.value( i ) == spb.value[ i ] );
        }
    }
    else
    {
        REQUIRE( spb.value == gpb.value( ) );
    }
    auto json_string                         = std::string( );
    auto print_options                       = google::protobuf::util::JsonPrintOptions( );
    print_options.preserve_proto_field_names = true;

    ( void ) MessageToJsonString( gpb, &json_string, print_options );
    REQUIRE( spb::json::deserialize< SPB >( json_string ) == spb );

    print_options.preserve_proto_field_names = false;
    json_string.clear( );
    ( void ) MessageToJsonString( gpb, &json_string, print_options );
    REQUIRE( spb::json::deserialize< SPB >( json_string ) == spb );
}

template < typename GPB, typename SPB >
void gpb_compatibility( const SPB & spb )
{
    SUBCASE( "gpb serialize/deserialize" )
    {
        gpb_test< GPB, SPB >( spb );
    }
    SUBCASE( "json serialize/deserialize" )
    {
        gpb_json< GPB, SPB >( spb );
    }
}

}// namespace

using namespace std::literals;

TEST_CASE( "string" )
{
    SUBCASE( "required" )
    {
        gpb_compatibility< Test::Scalar::gpb::ReqString >( Test::Scalar::ReqString{ .value = "hello" } );
    }
    SUBCASE( "optional" )
    {
        gpb_compatibility< Test::Scalar::gpb::OptString >( Test::Scalar::OptString{ .value = "hello" } );
    }
    SUBCASE( "repeated" )
    {
        gpb_compatibility< Test::Scalar::gpb::RepString >( Test::Scalar::RepString{ .value = { "hello" } } );
        gpb_compatibility< Test::Scalar::gpb::RepString >( Test::Scalar::RepString{ .value = { "hello", "world" } } );
    }
}
TEST_CASE( "bool" )
{
    SUBCASE( "required" )
    {
        gpb_compatibility< Test::Scalar::gpb::ReqBool >( Test::Scalar::ReqBool{ .value = true } );
        gpb_compatibility< Test::Scalar::gpb::ReqBool >( Test::Scalar::ReqBool{ .value = false } );
    }
    SUBCASE( "optional" )
    {
        gpb_compatibility< Test::Scalar::gpb::OptBool >( Test::Scalar::OptBool{ .value = true } );
        gpb_compatibility< Test::Scalar::gpb::OptBool >( Test::Scalar::OptBool{ .value = false } );
    }
    SUBCASE( "repeated" )
    {
        gpb_compatibility< Test::Scalar::gpb::RepBool >( Test::Scalar::RepBool{ .value = { true } } );
        gpb_compatibility< Test::Scalar::gpb::RepBool >( Test::Scalar::RepBool{ .value = { true, false } } );

        SUBCASE( "packed" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepPackBool >( Test::Scalar::RepPackBool{ .value = { true } } );
            gpb_compatibility< Test::Scalar::gpb::RepPackBool >( Test::Scalar::RepPackBool{ .value = { true, false } } );
        }
    }
}
TEST_CASE( "int" )
{
    SUBCASE( "varint32" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqInt32 >( Test::Scalar::ReqInt32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqInt32 >( Test::Scalar::ReqInt32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqInt32 >( Test::Scalar::ReqInt32{ .value = -2 } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptInt32 >( Test::Scalar::OptInt32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptInt32 >( Test::Scalar::OptInt32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptInt32 >( Test::Scalar::OptInt32{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepInt32 >( Test::Scalar::RepInt32{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepInt32 >( Test::Scalar::RepInt32{ .value = { 0x42, 0x3 } } );

            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackInt32 >( Test::Scalar::RepPackInt32{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackInt32 >( Test::Scalar::RepPackInt32{ .value = { 0x42, 0x3 } } );
            }
        }
    }
    SUBCASE( "varint64" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqInt64 >( Test::Scalar::ReqInt64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqInt64 >( Test::Scalar::ReqInt64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqInt64 >( Test::Scalar::ReqInt64{ .value = -2 } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptInt64 >( Test::Scalar::OptInt64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptInt64 >( Test::Scalar::OptInt64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptInt64 >( Test::Scalar::OptInt64{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepInt64 >( Test::Scalar::RepInt64{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepInt64 >( Test::Scalar::RepInt64{ .value = { 0x42, 0x3 } } );

            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackInt64 >( Test::Scalar::RepPackInt64{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackInt64 >( Test::Scalar::RepPackInt64{ .value = { 0x42, 0x3 } } );
            }
        }
    }
    SUBCASE( "svarint32" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqSint32 >( Test::Scalar::ReqSint32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqSint32 >( Test::Scalar::ReqSint32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqSint32 >( Test::Scalar::ReqSint32{ .value = -2 } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptSint32 >( Test::Scalar::OptSint32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptSint32 >( Test::Scalar::OptSint32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptSint32 >( Test::Scalar::OptSint32{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepSint32 >( Test::Scalar::RepSint32{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepSint32 >( Test::Scalar::RepSint32{ .value = { 0x42, -2 } } );

            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackSint32 >( Test::Scalar::RepPackSint32{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackSint32 >( Test::Scalar::RepPackSint32{ .value = { 0x42, -2 } } );
            }
        }
    }
    SUBCASE( "svarint64" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqSint64 >( Test::Scalar::ReqSint64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqSint64 >( Test::Scalar::ReqSint64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqSint64 >( Test::Scalar::ReqSint64{ .value = -2 } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptSint64 >( Test::Scalar::OptSint64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptSint64 >( Test::Scalar::OptSint64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptSint64 >( Test::Scalar::OptSint64{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepSint64 >( Test::Scalar::RepSint64{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepSint64 >( Test::Scalar::RepSint64{ .value = { 0x42, -2 } } );

            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackSint64 >( Test::Scalar::RepPackSint64{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackSint64 >( Test::Scalar::RepPackSint64{ .value = { 0x42, -2 } } );
            }
        }
    }
    SUBCASE( "fixed32" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqFixed32 >( Test::Scalar::ReqFixed32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqFixed32 >( Test::Scalar::ReqFixed32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqFixed32 >( Test::Scalar::ReqFixed32{ .value = uint32_t( -2 ) } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptFixed32 >( Test::Scalar::OptFixed32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptFixed32 >( Test::Scalar::OptFixed32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptFixed32 >( Test::Scalar::OptFixed32{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepFixed32 >( Test::Scalar::RepFixed32{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepFixed32 >( Test::Scalar::RepFixed32{ .value = { 0x42, 0x3 } } );

            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackFixed32 >( Test::Scalar::RepPackFixed32{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackFixed32 >( Test::Scalar::RepPackFixed32{ .value = { 0x42, 0x3 } } );
            }
        }
    }
    SUBCASE( "fixed64" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqFixed64 >( Test::Scalar::ReqFixed64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqFixed64 >( Test::Scalar::ReqFixed64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqFixed64 >( Test::Scalar::ReqFixed64{ .value = uint64_t( -2 ) } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptFixed64 >( Test::Scalar::OptFixed64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptFixed64 >( Test::Scalar::OptFixed64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptFixed64 >( Test::Scalar::OptFixed64{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepFixed64 >( Test::Scalar::RepFixed64{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepFixed64 >( Test::Scalar::RepFixed64{ .value = { 0x42, 0x3 } } );
            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackFixed64 >( Test::Scalar::RepPackFixed64{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackFixed64 >( Test::Scalar::RepPackFixed64{ .value = { 0x42, 0x3 } } );
            }
        }
    }
    SUBCASE( "sfixed32" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqSfixed32 >( Test::Scalar::ReqSfixed32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqSfixed32 >( Test::Scalar::ReqSfixed32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqSfixed32 >( Test::Scalar::ReqSfixed32{ .value = -2 } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptSfixed32 >( Test::Scalar::OptSfixed32{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptSfixed32 >( Test::Scalar::OptSfixed32{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptSfixed32 >( Test::Scalar::OptSfixed32{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepSfixed32 >( Test::Scalar::RepSfixed32{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepSfixed32 >( Test::Scalar::RepSfixed32{ .value = { 0x42, 0x3 } } );

            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackSfixed32 >( Test::Scalar::RepPackSfixed32{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackSfixed32 >( Test::Scalar::RepPackSfixed32{ .value = { 0x42, 0x3 } } );
            }
        }
    }
    SUBCASE( "sfixed64" )
    {
        SUBCASE( "required" )
        {
            gpb_compatibility< Test::Scalar::gpb::ReqSfixed64 >( Test::Scalar::ReqSfixed64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::ReqSfixed64 >( Test::Scalar::ReqSfixed64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::ReqSfixed64 >( Test::Scalar::ReqSfixed64{ .value = -2 } );
        }
        SUBCASE( "optional" )
        {
            gpb_compatibility< Test::Scalar::gpb::OptSfixed64 >( Test::Scalar::OptSfixed64{ .value = 0x42 } );
            gpb_compatibility< Test::Scalar::gpb::OptSfixed64 >( Test::Scalar::OptSfixed64{ .value = 0xff } );
            gpb_compatibility< Test::Scalar::gpb::OptSfixed64 >( Test::Scalar::OptSfixed64{ .value = -2 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_compatibility< Test::Scalar::gpb::RepSfixed64 >( Test::Scalar::RepSfixed64{ .value = { 0x42 } } );
            gpb_compatibility< Test::Scalar::gpb::RepSfixed64 >( Test::Scalar::RepSfixed64{ .value = { 0x42, 0x3 } } );

            SUBCASE( "packed" )
            {
                gpb_compatibility< Test::Scalar::gpb::RepPackSfixed64 >( Test::Scalar::RepPackSfixed64{ .value = { 0x42 } } );
                gpb_compatibility< Test::Scalar::gpb::RepPackSfixed64 >( Test::Scalar::RepPackSfixed64{ .value = { 0x42, 0x3 } } );
            }
        }
    }
}
TEST_CASE( "float" )
{
    SUBCASE( "required" )
    {
        gpb_compatibility< Test::Scalar::gpb::ReqFloat >( Test::Scalar::ReqFloat{ .value = 42.0 } );
    }
    SUBCASE( "optional" )
    {
        gpb_compatibility< Test::Scalar::gpb::OptFloat >( Test::Scalar::OptFloat{ .value = 42.3 } );
    }
    SUBCASE( "repeated" )
    {
        gpb_compatibility< Test::Scalar::gpb::RepFloat >( Test::Scalar::RepFloat{ .value = { 42.3 } } );
        gpb_compatibility< Test::Scalar::gpb::RepFloat >( Test::Scalar::RepFloat{ .value = { 42.0, 42.3 } } );
    }
}
TEST_CASE( "double" )
{
    SUBCASE( "required" )
    {
        gpb_compatibility< Test::Scalar::gpb::ReqDouble >( Test::Scalar::ReqDouble{ .value = 42.0 } );
    }
    SUBCASE( "optional" )
    {
        gpb_compatibility< Test::Scalar::gpb::OptDouble >( Test::Scalar::OptDouble{ .value = 42.3 } );
    }
    SUBCASE( "repeated" )
    {
        gpb_compatibility< Test::Scalar::gpb::RepDouble >( Test::Scalar::RepDouble{ .value = { 42.3 } } );
        gpb_compatibility< Test::Scalar::gpb::RepDouble >( Test::Scalar::RepDouble{ .value = { 42.3, 3.0 } } );
    }
}
TEST_CASE( "bytes" )
{
    SUBCASE( "required" )
    {
        gpb_compatibility< Test::Scalar::gpb::ReqBytes >( Test::Scalar::ReqBytes{ .value = to_bytes( "hello" ) } );
        gpb_compatibility< Test::Scalar::gpb::ReqBytes >( Test::Scalar::ReqBytes{ .value = to_bytes( "\x00\x01\x02"sv ) } );
        gpb_compatibility< Test::Scalar::gpb::ReqBytes >( Test::Scalar::ReqBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) } );
    }
    SUBCASE( "optional" )
    {
        gpb_compatibility< Test::Scalar::gpb::OptBytes >( Test::Scalar::OptBytes{ .value = to_bytes( "hello" ) } );
        gpb_compatibility< Test::Scalar::gpb::OptBytes >( Test::Scalar::OptBytes{ .value = to_bytes( "\x00\x01\x02"sv ) } );
        gpb_compatibility< Test::Scalar::gpb::OptBytes >( Test::Scalar::OptBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) } );
    }
    SUBCASE( "repeated" )
    {
        gpb_compatibility< Test::Scalar::gpb::RepBytes >( Test::Scalar::RepBytes{ .value = { to_bytes( "hello" ) } } );
        gpb_compatibility< Test::Scalar::gpb::RepBytes >( Test::Scalar::RepBytes{ .value = { to_bytes( "\x00\x01\x02"sv ) } } );
        gpb_compatibility< Test::Scalar::gpb::RepBytes >( Test::Scalar::RepBytes{ .value = { to_bytes( "\x00\x01\x02\x03\x04"sv ) } } );
    }
}
TEST_CASE( "variant" )
{
    SUBCASE( "int" )
    {
        auto gpb            = Test::gpb::Variant( );
        auto spb            = Test::Variant{ .oneof_field = 0x42U };
        auto spb_serialized = spb::pb::serialize( spb );

        REQUIRE( gpb.ParseFromString( spb_serialized ) );
        REQUIRE( gpb.var_int( ) == std::get< 0 >( spb.oneof_field ) );

        SUBCASE( "deserialize" )
        {
            auto gpb_serialized = std::string( );
            REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
            REQUIRE( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
        }
    }
    SUBCASE( "string" )
    {
        auto gpb            = Test::gpb::Variant( );
        auto spb            = Test::Variant{ .oneof_field = "hello" };
        auto spb_serialized = spb::pb::serialize( spb );

        REQUIRE( gpb.ParseFromString( spb_serialized ) );
        REQUIRE( gpb.var_string( ) == std::get< 1 >( spb.oneof_field ) );

        SUBCASE( "deserialize" )
        {
            auto gpb_serialized = std::string( );
            REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
            REQUIRE( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
        }
    }
    SUBCASE( "bytes" )
    {
        auto gpb            = Test::gpb::Variant( );
        auto spb            = Test::Variant{ .oneof_field = to_bytes( "hello" ) };
        auto spb_serialized = spb::pb::serialize( spb );

        REQUIRE( gpb.ParseFromString( spb_serialized ) );
        REQUIRE( gpb.var_bytes( ) == std::get< 2 >( spb.oneof_field ) );

        SUBCASE( "deserialize" )
        {
            auto gpb_serialized = std::string( );
            REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
            REQUIRE( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
        }
    }
    SUBCASE( "name" )
    {
        auto gpb            = Test::gpb::Variant( );
        auto spb            = Test::Variant{ .oneof_field = Test::Name{ .name = "John" } };
        auto spb_serialized = spb::pb::serialize( spb );

        REQUIRE( gpb.ParseFromString( spb_serialized ) );
        REQUIRE( gpb.has_name( ) );

        REQUIRE( gpb.name( ).name( ) == std::get< 3 >( spb.oneof_field ).name );

        SUBCASE( "deserialize" )
        {
            auto gpb_serialized = std::string( );
            REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
            REQUIRE( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
        }
    }
}
TEST_CASE( "person" )
{
    auto gpb = PhoneBook::gpb::Person( );
    auto spb = PhoneBook::Person{
        .name   = "John Doe",
        .id     = 123,
        .email  = "QXUeh@example.com",
        .phones = {
            PhoneBook::Person::PhoneNumber{
                .number = "555-4321",
                .type   = PhoneBook::Person::PhoneType::HOME,
            },
            PhoneBook::Person::PhoneNumber{
                .number = "999-1234",
                .type   = PhoneBook::Person::PhoneType::MOBILE,
            },
        }
    };
    SUBCASE( "gpb serialize/deserialize" )
    {
        auto spb_serialized = spb::pb::serialize( spb );

        REQUIRE( gpb.ParseFromString( spb_serialized ) );
        REQUIRE( gpb.name( ) == spb.name );
        REQUIRE( gpb.id( ) == spb.id );
        REQUIRE( gpb.email( ) == spb.email );
        REQUIRE( gpb.phones_size( ) == 2 );
        for( auto i = 0; i < gpb.phones_size( ); i++ )
        {
            REQUIRE( gpb.phones( i ).number( ) == spb.phones[ i ].number );
            REQUIRE( int( gpb.phones( i ).type( ) ) == int( spb.phones[ i ].type.value( ) ) );
        }

        auto gpb_serialized = std::string( );
        REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
        REQUIRE( spb::pb::deserialize< PhoneBook::Person >( gpb_serialized ) == spb );
    }
    SUBCASE( "json serialize/deserialize" )
    {
        auto spb_json = spb::json::serialize( spb );

        auto parse_options = google::protobuf::util::JsonParseOptions{ };
        ( void ) JsonStringToMessage( spb_json, &gpb, parse_options );

        REQUIRE( gpb.name( ) == spb.name );
        REQUIRE( gpb.id( ) == spb.id );
        REQUIRE( gpb.email( ) == spb.email );
        REQUIRE( gpb.phones_size( ) == 2 );
        for( auto i = 0; i < gpb.phones_size( ); i++ )
        {
            REQUIRE( gpb.phones( i ).number( ) == spb.phones[ i ].number );
            REQUIRE( int( gpb.phones( i ).type( ) ) == int( spb.phones[ i ].type.value( ) ) );
        }

        auto json_string                         = std::string( );
        auto print_options                       = google::protobuf::util::JsonPrintOptions( );
        print_options.preserve_proto_field_names = true;

        ( void ) MessageToJsonString( gpb, &json_string, print_options );
        REQUIRE( spb::json::deserialize< PhoneBook::Person >( json_string ) == spb );

        print_options.preserve_proto_field_names = false;
        json_string.clear( );
        ( void ) MessageToJsonString( gpb, &json_string, print_options );
        REQUIRE( spb::json::deserialize< PhoneBook::Person >( json_string ) == spb );
    }
}
