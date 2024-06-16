#include <cstdint>
#include <gpb-name.pb.h>
#include <gpb-person.pb.h>
#include <gpb-scalar.pb.h>
#include <name.pb.h>
#include <optional>
#include <person.pb.h>
#include <scalar.pb.h>
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

template < typename GPB, typename SDS >
void gpb_test( const SDS & spb )
{
    SUBCASE( "serialize" )
    {
        auto gpb            = GPB( );
        auto sds_serialized = spb::pb::serialize( spb );

        REQUIRE( gpb.ParseFromString( sds_serialized ) );
        if constexpr( is_gpb_repeated< GPB > )
        {
            REQUIRE( gpb.value( ).size( ) == opt_size( spb.value ) );
            for( size_t i = 0; i < opt_size( spb.value ); ++i )
            {
                CHECK( gpb.value( i ) == spb.value[ i ] );
            }
        }
        else
        {
            CHECK( spb.value == gpb.value( ) );
        }
        SUBCASE( "deserialize" )
        {
            auto gpb_serialized = std::string( );
            REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
            CHECK( spb::pb::deserialize< SDS >( gpb_serialized ) == spb );
        }
    }
}

}// namespace

using namespace std::literals;

TEST_CASE( "gpb-compatibility" )
{
    SUBCASE( "string" )
    {
        SUBCASE( "required" )
        {
            gpb_test< Test::Scalar::gpb::ReqString >( Test::Scalar::ReqString{ .value = "hello" } );
        }
        SUBCASE( "optional" )
        {
            gpb_test< Test::Scalar::gpb::OptString >( Test::Scalar::OptString{ .value = "hello" } );
        }
        SUBCASE( "repeated" )
        {
            gpb_test< Test::Scalar::gpb::RepString >( Test::Scalar::RepString{ .value = { "hello" } } );
            gpb_test< Test::Scalar::gpb::RepString >( Test::Scalar::RepString{ .value = { "hello", "world" } } );
        }
    }
    SUBCASE( "bool" )
    {
        SUBCASE( "required" )
        {
            gpb_test< Test::Scalar::gpb::ReqBool >( Test::Scalar::ReqBool{ .value = true } );
            gpb_test< Test::Scalar::gpb::ReqBool >( Test::Scalar::ReqBool{ .value = false } );
        }
        SUBCASE( "optional" )
        {
            gpb_test< Test::Scalar::gpb::OptBool >( Test::Scalar::OptBool{ .value = true } );
            gpb_test< Test::Scalar::gpb::OptBool >( Test::Scalar::OptBool{ .value = false } );
        }
        SUBCASE( "repeated" )
        {
            gpb_test< Test::Scalar::gpb::RepBool >( Test::Scalar::RepBool{ .value = { true } } );
            gpb_test< Test::Scalar::gpb::RepBool >( Test::Scalar::RepBool{ .value = { true, false } } );
        }
    }
    SUBCASE( "int" )
    {
        SUBCASE( "varint32" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqInt32 >( Test::Scalar::ReqInt32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqInt32 >( Test::Scalar::ReqInt32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqInt32 >( Test::Scalar::ReqInt32{ .value = -2 } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptInt32 >( Test::Scalar::OptInt32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptInt32 >( Test::Scalar::OptInt32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptInt32 >( Test::Scalar::OptInt32{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepInt32 >( Test::Scalar::RepInt32{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepInt32 >( Test::Scalar::RepInt32{ .value = { 0x42, 0x3 } } );

                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackInt32 >( Test::Scalar::RepPackInt32{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackInt32 >( Test::Scalar::RepPackInt32{ .value = { 0x42, 0x3 } } );
                }
            }
        }
        SUBCASE( "varint64" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqInt64 >( Test::Scalar::ReqInt64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqInt64 >( Test::Scalar::ReqInt64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqInt64 >( Test::Scalar::ReqInt64{ .value = -2 } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptInt64 >( Test::Scalar::OptInt64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptInt64 >( Test::Scalar::OptInt64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptInt64 >( Test::Scalar::OptInt64{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepInt64 >( Test::Scalar::RepInt64{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepInt64 >( Test::Scalar::RepInt64{ .value = { 0x42, 0x3 } } );

                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackInt64 >( Test::Scalar::RepPackInt64{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackInt64 >( Test::Scalar::RepPackInt64{ .value = { 0x42, 0x3 } } );
                }
            }
        }
        SUBCASE( "svarint32" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqSint32 >( Test::Scalar::ReqSint32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqSint32 >( Test::Scalar::ReqSint32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqSint32 >( Test::Scalar::ReqSint32{ .value = -2 } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptSint32 >( Test::Scalar::OptSint32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptSint32 >( Test::Scalar::OptSint32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptSint32 >( Test::Scalar::OptSint32{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepSint32 >( Test::Scalar::RepSint32{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepSint32 >( Test::Scalar::RepSint32{ .value = { 0x42, -2 } } );

                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackSint32 >( Test::Scalar::RepPackSint32{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackSint32 >( Test::Scalar::RepPackSint32{ .value = { 0x42, -2 } } );
                }
            }
        }
        SUBCASE( "svarint64" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqSint64 >( Test::Scalar::ReqSint64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqSint64 >( Test::Scalar::ReqSint64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqSint64 >( Test::Scalar::ReqSint64{ .value = -2 } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptSint64 >( Test::Scalar::OptSint64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptSint64 >( Test::Scalar::OptSint64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptSint64 >( Test::Scalar::OptSint64{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepSint64 >( Test::Scalar::RepSint64{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepSint64 >( Test::Scalar::RepSint64{ .value = { 0x42, -2 } } );

                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackSint64 >( Test::Scalar::RepPackSint64{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackSint64 >( Test::Scalar::RepPackSint64{ .value = { 0x42, -2 } } );
                }
            }
        }
        SUBCASE( "fixed32" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqFixed32 >( Test::Scalar::ReqFixed32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqFixed32 >( Test::Scalar::ReqFixed32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqFixed32 >( Test::Scalar::ReqFixed32{ .value = uint32_t( -2 ) } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptFixed32 >( Test::Scalar::OptFixed32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptFixed32 >( Test::Scalar::OptFixed32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptFixed32 >( Test::Scalar::OptFixed32{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepFixed32 >( Test::Scalar::RepFixed32{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepFixed32 >( Test::Scalar::RepFixed32{ .value = { 0x42, 0x3 } } );

                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackFixed32 >( Test::Scalar::RepPackFixed32{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackFixed32 >( Test::Scalar::RepPackFixed32{ .value = { 0x42, 0x3 } } );
                }
            }
        }
        SUBCASE( "fixed64" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqFixed64 >( Test::Scalar::ReqFixed64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqFixed64 >( Test::Scalar::ReqFixed64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqFixed64 >( Test::Scalar::ReqFixed64{ .value = uint64_t( -2 ) } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptFixed64 >( Test::Scalar::OptFixed64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptFixed64 >( Test::Scalar::OptFixed64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptFixed64 >( Test::Scalar::OptFixed64{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepFixed64 >( Test::Scalar::RepFixed64{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepFixed64 >( Test::Scalar::RepFixed64{ .value = { 0x42, 0x3 } } );
                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackFixed64 >( Test::Scalar::RepPackFixed64{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackFixed64 >( Test::Scalar::RepPackFixed64{ .value = { 0x42, 0x3 } } );
                }
            }
        }
        SUBCASE( "sfixed32" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqSfixed32 >( Test::Scalar::ReqSfixed32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqSfixed32 >( Test::Scalar::ReqSfixed32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqSfixed32 >( Test::Scalar::ReqSfixed32{ .value = -2 } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptSfixed32 >( Test::Scalar::OptSfixed32{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptSfixed32 >( Test::Scalar::OptSfixed32{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptSfixed32 >( Test::Scalar::OptSfixed32{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepSfixed32 >( Test::Scalar::RepSfixed32{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepSfixed32 >( Test::Scalar::RepSfixed32{ .value = { 0x42, 0x3 } } );

                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackSfixed32 >( Test::Scalar::RepPackSfixed32{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackSfixed32 >( Test::Scalar::RepPackSfixed32{ .value = { 0x42, 0x3 } } );
                }
            }
        }
        SUBCASE( "sfixed64" )
        {
            SUBCASE( "required" )
            {
                gpb_test< Test::Scalar::gpb::ReqSfixed64 >( Test::Scalar::ReqSfixed64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::ReqSfixed64 >( Test::Scalar::ReqSfixed64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::ReqSfixed64 >( Test::Scalar::ReqSfixed64{ .value = -2 } );
            }
            SUBCASE( "optional" )
            {
                gpb_test< Test::Scalar::gpb::OptSfixed64 >( Test::Scalar::OptSfixed64{ .value = 0x42 } );
                gpb_test< Test::Scalar::gpb::OptSfixed64 >( Test::Scalar::OptSfixed64{ .value = 0xff } );
                gpb_test< Test::Scalar::gpb::OptSfixed64 >( Test::Scalar::OptSfixed64{ .value = -2 } );
            }
            SUBCASE( "repeated" )
            {
                gpb_test< Test::Scalar::gpb::RepSfixed64 >( Test::Scalar::RepSfixed64{ .value = { 0x42 } } );
                gpb_test< Test::Scalar::gpb::RepSfixed64 >( Test::Scalar::RepSfixed64{ .value = { 0x42, 0x3 } } );

                SUBCASE( "packed" )
                {
                    gpb_test< Test::Scalar::gpb::RepPackSfixed64 >( Test::Scalar::RepPackSfixed64{ .value = { 0x42 } } );
                    gpb_test< Test::Scalar::gpb::RepPackSfixed64 >( Test::Scalar::RepPackSfixed64{ .value = { 0x42, 0x3 } } );
                }
            }
        }
    }
    SUBCASE( "float" )
    {
        SUBCASE( "required" )
        {
            gpb_test< Test::Scalar::gpb::ReqFloat >( Test::Scalar::ReqFloat{ .value = 42.0 } );
        }
        SUBCASE( "optional" )
        {
            gpb_test< Test::Scalar::gpb::OptFloat >( Test::Scalar::OptFloat{ .value = 42.3 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_test< Test::Scalar::gpb::RepFloat >( Test::Scalar::RepFloat{ .value = { 42.3 } } );
            gpb_test< Test::Scalar::gpb::RepFloat >( Test::Scalar::RepFloat{ .value = { 42.0, 42.3 } } );
        }
    }
    SUBCASE( "double" )
    {
        SUBCASE( "required" )
        {
            gpb_test< Test::Scalar::gpb::ReqDouble >( Test::Scalar::ReqDouble{ .value = 42.0 } );
        }
        SUBCASE( "optional" )
        {
            gpb_test< Test::Scalar::gpb::OptDouble >( Test::Scalar::OptDouble{ .value = 42.3 } );
        }
        SUBCASE( "repeated" )
        {
            gpb_test< Test::Scalar::gpb::RepDouble >( Test::Scalar::RepDouble{ .value = { 42.3 } } );
            gpb_test< Test::Scalar::gpb::RepDouble >( Test::Scalar::RepDouble{ .value = { 42.3, 3.0 } } );
        }
    }
    SUBCASE( "bytes" )
    {
        SUBCASE( "required" )
        {
            gpb_test< Test::Scalar::gpb::ReqBytes >( Test::Scalar::ReqBytes{ .value = to_bytes( "hello" ) } );
            gpb_test< Test::Scalar::gpb::ReqBytes >( Test::Scalar::ReqBytes{ .value = to_bytes( "\x00\x01\x02"sv ) } );
            gpb_test< Test::Scalar::gpb::ReqBytes >( Test::Scalar::ReqBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) } );
        }
        SUBCASE( "optional" )
        {
            gpb_test< Test::Scalar::gpb::OptBytes >( Test::Scalar::OptBytes{ .value = to_bytes( "hello" ) } );
            gpb_test< Test::Scalar::gpb::OptBytes >( Test::Scalar::OptBytes{ .value = to_bytes( "\x00\x01\x02"sv ) } );
            gpb_test< Test::Scalar::gpb::OptBytes >( Test::Scalar::OptBytes{ .value = to_bytes( "\x00\x01\x02\x03\x04"sv ) } );
        }
        SUBCASE( "repeated" )
        {
            gpb_test< Test::Scalar::gpb::RepBytes >( Test::Scalar::RepBytes{ .value = { to_bytes( "hello" ) } } );
            gpb_test< Test::Scalar::gpb::RepBytes >( Test::Scalar::RepBytes{ .value = { to_bytes( "\x00\x01\x02"sv ) } } );
            gpb_test< Test::Scalar::gpb::RepBytes >( Test::Scalar::RepBytes{ .value = { to_bytes( "\x00\x01\x02\x03\x04"sv ) } } );
        }
    }
    SUBCASE( "variant" )
    {
        SUBCASE( "int" )
        {
            auto gpb            = Test::gpb::Variant( );
            auto spb            = Test::Variant{ .oneof_field = 0x42U };
            auto sds_serialized = spb::pb::serialize( spb );

            REQUIRE( gpb.ParseFromString( sds_serialized ) );
            CHECK( gpb.var_int( ) == std::get< 0 >( spb.oneof_field ) );

            SUBCASE( "deserialize" )
            {
                auto gpb_serialized = std::string( );
                REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
                CHECK( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
            }
        }
        SUBCASE( "string" )
        {
            auto gpb            = Test::gpb::Variant( );
            auto spb            = Test::Variant{ .oneof_field = "hello" };
            auto sds_serialized = spb::pb::serialize( spb );

            REQUIRE( gpb.ParseFromString( sds_serialized ) );
            CHECK( gpb.var_string( ) == std::get< 1 >( spb.oneof_field ) );

            SUBCASE( "deserialize" )
            {
                auto gpb_serialized = std::string( );
                REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
                CHECK( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
            }
        }
        SUBCASE( "bytes" )
        {
            auto gpb            = Test::gpb::Variant( );
            auto spb            = Test::Variant{ .oneof_field = to_bytes( "hello" ) };
            auto sds_serialized = spb::pb::serialize( spb );

            REQUIRE( gpb.ParseFromString( sds_serialized ) );
            CHECK( gpb.var_bytes( ) == std::get< 2 >( spb.oneof_field ) );

            SUBCASE( "deserialize" )
            {
                auto gpb_serialized = std::string( );
                REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
                CHECK( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
            }
        }
        SUBCASE( "name" )
        {
            auto gpb            = Test::gpb::Variant( );
            auto spb            = Test::Variant{ .oneof_field = Test::Name{ .name = "John" } };
            auto sds_serialized = spb::pb::serialize( spb );

            REQUIRE( gpb.ParseFromString( sds_serialized ) );
            REQUIRE( gpb.has_name( ) );

            CHECK( gpb.name( ).name( ) == std::get< 3 >( spb.oneof_field ).name );

            SUBCASE( "deserialize" )
            {
                auto gpb_serialized = std::string( );
                REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
                CHECK( spb::pb::deserialize< Test::Variant >( gpb_serialized ) == spb );
            }
        }
    }
    SUBCASE( "person" )
    {
        SUBCASE( "serialize" )
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
            auto sds_serialized = spb::pb::serialize( spb );

            REQUIRE( gpb.ParseFromString( sds_serialized ) );
            CHECK( gpb.name( ) == spb.name );
            CHECK( gpb.id( ) == spb.id );
            CHECK( gpb.email( ) == spb.email );
            REQUIRE( gpb.phones_size( ) == 2 );
            for( auto i = 0; i < gpb.phones_size( ); i++ )
            {
                CHECK( gpb.phones( i ).number( ) == spb.phones[ i ].number );
                CHECK( int( gpb.phones( i ).type( ) ) == int( spb.phones[ i ].type.value( ) ) );
            }

            SUBCASE( "deserialize" )
            {
                auto gpb_serialized = std::string( );
                REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
                CHECK( spb::pb::deserialize< PhoneBook::Person >( gpb_serialized ) == spb );
            }
        }
    }
}
