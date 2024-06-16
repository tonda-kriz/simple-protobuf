#include <cstdint>
#include <gpb-person.pb.h>
#include <gpb-scalar.pb.h>
#include <memory>
#include <name.pb.h>
#include <optional>
#include <person.pb.h>
#include <scalar.pb.h>
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
void gpb_test( const SDS & sds )
{
    SUBCASE( "serialize" )
    {
        auto gpb            = GPB( );
        auto sds_serialized = sds::pb::serialize( sds );

        REQUIRE( gpb.ParseFromString( sds_serialized ) );
        if constexpr( is_gpb_repeated< GPB > )
        {
            REQUIRE( gpb.value( ).size( ) == opt_size( sds.value ) );
            for( size_t i = 0; i < opt_size( sds.value ); ++i )
            {
                CHECK( gpb.value( i ) == sds.value[ i ] );
            }
        }
        else
        {
            CHECK( sds.value == gpb.value( ) );
        }
        SUBCASE( "deserialize" )
        {
            auto gpb_serialized = std::string( );
            REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
            CHECK( sds::pb::deserialize< SDS >( gpb_serialized ) == sds );
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
    /*SUBCASE( "int" )
    {
        SUBCASE( "varint32" )
        {
            SUBCASE( "required" )
            {
                pb_test( Test::Scalar::ReqInt32{ .value = 0x42 }, "\x08\x42" );
                pb_test( Test::Scalar::ReqInt32{ .value = 0xff }, "\x08\xff\x01" );
                pb_test( Test::Scalar::ReqInt32{ .value = -2 }, "\x08\xfe\xff\xff\xff\x0f"sv );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
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
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackInt32 >( "\x0a\x02\x42" ) );
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackInt32 >( "\x0a\x02\x42\xff" ) );
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
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqInt32 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
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
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackInt64 >( "\x0a\x02\x42" ) );
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackInt64 >( "\x0a\x02\x42\xff" ) );
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
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSint32 >( "\x08" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSint32 >( "\x08\xff" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSint32 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
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
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackSint32 >( "\x0a\x02\x42" ) );
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackSint32 >( "\x0a\x02\x42\xff" ) );
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
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSint64 >( "\x08" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSint64 >( "\x08\xff" ) );
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSint64 >( "\x08\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" ) );
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
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackSint64 >( "\x0a\x02\x84"sv ) );
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackSint64 >( "\x0a\x03\x84\x01"sv ) );
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
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqFixed32 >( "\x0d\x42\x00\x00"sv ) );
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
                    CHECK_THROWS( sds::pb::deserialize< Test::Scalar::RepPackFixed32 >( "\x0a\x08\x42\x00\x00\x00\x03\x00\x00"sv ) );
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
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqFixed32 >( "\x09\x42\x00\x00\x00\x00\x00\x00"sv ) );
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
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSfixed32 >( "\x0d\x42\x00\x00"sv ) );
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
                CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqSfixed64 >( "\x09\x42\x00\x00\x00\x00\x00\x00"sv ) );
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
            pb_test( Test::Scalar::ReqFloat{ .value = 42.0 }, "\x0d\x00\x00\x28\x42"sv );
            CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqFloat >( "\x0d\x00\x00\x28"sv ) );
        }
        SUBCASE( "optional" )
        {
            pb_test( Test::Scalar::OptFloat{ .value = 42.3 }, "\x0d\x33\x33\x29\x42"sv );
        }
        SUBCASE( "repeated" )
        {
            pb_test( Test::Scalar::RepFloat{ .value = { 42.3 } }, "\x0d\x33\x33\x29\x42"sv );
            pb_test( Test::Scalar::RepFloat{ .value = { 42.0, 42.3 } }, "\x0d\x00\x00\x28\x42\x0d\x33\x33\x29\x42"sv );
            pb_test( Test::Scalar::RepFloat{ .value = {} }, ""sv );
        }
    }
    SUBCASE( "double" )
    {
        SUBCASE( "required" )
        {
            pb_test( Test::Scalar::ReqDouble{ .value = 42.0 }, "\x09\x00\x00\x00\x00\x00\x00\x45\x40"sv );
            CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqDouble >( "\x0d\x00\x00\x28"sv ) );
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
            CHECK_THROWS( sds::pb::deserialize< Test::Scalar::ReqBytes >( "\x0a\x05hell"sv ) );
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
            pb_test( Test::Variant{ .oneof_field = std::vector< std::byte >{ std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } }, "\x1A\x05hello" );
        }
        SUBCASE( "name" )
        {
            pb_test( Test::Variant{ .oneof_field = Test::Name{ .name = "John" } }, "\x22\x06\x0A\x04John" );
        }
    }
    */
    SUBCASE( "person" )
    {
        SUBCASE( "serialize" )
        {
            auto gpb = PhoneBook::gpb::Person( );
            auto sds = PhoneBook::Person{
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
            auto sds_serialized = sds::pb::serialize( sds );

            REQUIRE( gpb.ParseFromString( sds_serialized ) );
            CHECK( gpb.name( ) == sds.name );
            CHECK( gpb.id( ) == sds.id );
            CHECK( gpb.email( ) == sds.email );
            CHECK( gpb.phones_size( ) == 2 );
            for( auto i = 0; i < gpb.phones_size( ); i++ )
            {
                CHECK( gpb.phones( i ).number( ) == sds.phones[ i ].number );
                CHECK( int( gpb.phones( i ).type( ) ) == int( sds.phones[ i ].type.value( ) ) );
            }

            SUBCASE( "deserialize" )
            {
                auto gpb_serialized = std::string( );
                REQUIRE( gpb.SerializeToString( &gpb_serialized ) );
                CHECK( sds::pb::deserialize< PhoneBook::Person >( gpb_serialized ) == sds );
            }
        }
    }
}
