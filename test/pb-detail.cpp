#include "sds/pb/wire-types.h"
#include <cstdint>
#include <memory>
#include <name.pb.h>
#include <person.pb.h>
#include <sds/pb/serialize.hpp>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace Test
{
auto operator==( const Test::Name & lhs, const Test::Name & rhs ) noexcept -> bool
{
    return lhs.name == rhs.name;
}
}// namespace Test

namespace
{
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

using sds::pb::detail::scalar_encoder;

}// namespace
using namespace std::literals;

TEST_CASE( "protobuf" )
{
    SUBCASE( "string" )
    {
        CHECK( pb_serialize< std::string >( "" ) == "" );
        SUBCASE( "optional" )
        {
            CHECK( pb_serialize< std::optional< std::string > >( std::nullopt ) == "" );
            CHECK( pb_serialize< std::optional< std::string > >( "hello" ) == "\x0a\x05hello" );
        }
        SUBCASE( "array" )
        {
            CHECK( pb_serialize< std::vector< std::string > >( { "hello", "world" } ) == "\x0a\x05hello\x0a\x05world" );
            CHECK( pb_serialize< std::vector< std::string > >( { } ) == "" );
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
        CHECK( pb_serialize_as< scalar_encoder::varint >( true ) == "\x08\x01" );
        CHECK( pb_serialize_as< scalar_encoder::varint >( false ) == "\x08\x00"sv );
        SUBCASE( "optional" )
        {
            CHECK( pb_serialize_as< scalar_encoder::varint, std::optional< bool > >( std::nullopt ) == "" );
            CHECK( pb_serialize_as< scalar_encoder::varint, std::optional< bool > >( true ) == "\x08\x01" );
            CHECK( pb_serialize_as< scalar_encoder::varint, std::optional< bool > >( false ) == "\x08\x00"sv );
        }
        SUBCASE( "array" )
        {
            CHECK( pb_serialize_as< scalar_encoder::varint, std::vector< bool > >( { true, false } ) == "\x08\x01\x08\x00"sv );
            CHECK( pb_serialize_as< scalar_encoder::varint, std::vector< bool > >( { } ) == "" );
        }
    }
    SUBCASE( "int" )
    {
        SUBCASE( "varint" )
        {
            CHECK( pb_serialize_as< scalar_encoder::varint >( 0x42 ) == "\x08\x42" );
            CHECK( pb_serialize_as< scalar_encoder::varint >( 0xff ) == "\x08\xff\x01" );
            CHECK( pb_serialize_as< scalar_encoder::varint >( int32_t( -2 ) ) == "\x08\xfe\xff\xff\xff\x0f" );
            CHECK( pb_serialize_as< scalar_encoder::varint >( int64_t( -2 ) ) == "\x08\xfe\xff\xff\xff\xff\xff\xff\xff\xff\x01" );

            SUBCASE( "optional" )
            {
                CHECK( pb_serialize_as< scalar_encoder::varint, std::optional< int > >( std::nullopt ) == "" );
                CHECK( pb_serialize_as< scalar_encoder::varint, std::optional< int > >( 0x42 ) == "\x08\x42" );
            }
            SUBCASE( "array" )
            {
                CHECK( pb_serialize_as< scalar_encoder::varint, std::vector< int > >( { 0x42 } ) == "\x08\x42" );
                CHECK( pb_serialize_as< scalar_encoder::varint, std::vector< int > >( { 0x42, 0x3 } ) == "\x08\x42\x08\x03" );
                CHECK( pb_serialize_as< scalar_encoder::varint, std::vector< int > >( { } ) == ""sv );

                SUBCASE( "packed" )
                {
                    CHECK( pb_serialize_as< combine( scalar_encoder::varint, scalar_encoder::packed ), std::vector< int > >( { 0x42 } ) == "\x0a\x01\x42"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::varint, scalar_encoder::packed ), std::vector< int > >( { 0x42, 0x3 } ) == "\x0a\x02\x42\x03"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::varint, scalar_encoder::packed ), std::vector< int > >( { } ) == ""sv );
                }
            }
        }
        SUBCASE( "svarint" )
        {
            CHECK( pb_serialize_as< scalar_encoder::svarint >( 0x42 ) == "\x08\x84\x01"sv );
            CHECK( pb_serialize_as< scalar_encoder::svarint >( 0xff ) == "\x08\xfe\x03"sv );
            CHECK( pb_serialize_as< scalar_encoder::svarint >( int32_t( -2 ) ) == "\x08\x03"sv );
            CHECK( pb_serialize_as< scalar_encoder::svarint >( int64_t( -2 ) ) == "\x08\x03"sv );

            SUBCASE( "optional" )
            {
                CHECK( pb_serialize_as< scalar_encoder::svarint, std::optional< int > >( std::nullopt ) == "" );
                CHECK( pb_serialize_as< scalar_encoder::svarint, std::optional< int > >( 0x42 ) == "\x08\x84\x01"sv );
            }
            SUBCASE( "array" )
            {
                CHECK( pb_serialize_as< scalar_encoder::svarint, std::vector< int > >( { 0x42 } ) == "\x08\x84\x01"sv );
                CHECK( pb_serialize_as< scalar_encoder::svarint, std::vector< int > >( { 0x42, -2 } ) == "\x08\x84\x01\x08\x03" );
                CHECK( pb_serialize_as< scalar_encoder::svarint, std::vector< int > >( { } ) == ""sv );

                SUBCASE( "packed" )
                {
                    CHECK( pb_serialize_as< combine( scalar_encoder::svarint, scalar_encoder::packed ), std::vector< int > >( { 0x42 } ) == "\x0a\x02\x84\x01"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::svarint, scalar_encoder::packed ), std::vector< int > >( { 0x42, -2 } ) == "\x0a\x03\x84\x01\x03"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::svarint, scalar_encoder::packed ), std::vector< int > >( { } ) == ""sv );
                }
            }
        }
        SUBCASE( "i32" )
        {
            CHECK( pb_serialize_as< scalar_encoder::i32 >( 0x42 ) == "\x0d\x42\x00\x00\x00"sv );
            CHECK( pb_serialize_as< scalar_encoder::i32 >( 0xff ) == "\x0d\xff\x00\x00\x00"sv );
            CHECK( pb_serialize_as< scalar_encoder::i32 >( int32_t( -2 ) ) == "\x0d\xfe\xff\xff\xff"sv );
            SUBCASE( "optional" )
            {
                CHECK( pb_serialize_as< scalar_encoder::i32, std::optional< int > >( std::nullopt ) == "" );
                CHECK( pb_serialize_as< scalar_encoder::i32, std::optional< int > >( 0x42 ) == "\x0d\x42\x00\x00\x00"sv );
            }
            SUBCASE( "array" )
            {
                CHECK( pb_serialize_as< scalar_encoder::i32, std::vector< int > >( { 0x42 } ) == "\x0d\x42\x00\x00\x00"sv );
                CHECK( pb_serialize_as< scalar_encoder::i32, std::vector< int > >( { 0x42, 0x3 } ) == "\x0d\x42\x00\x00\x00\x0d\x03\x00\x00\x00"sv );
                CHECK( pb_serialize_as< scalar_encoder::i32, std::vector< int > >( { } ) == ""sv );
                SUBCASE( "packed" )
                {
                    CHECK( pb_serialize_as< combine( scalar_encoder::i32, scalar_encoder::packed ), std::vector< int > >( { 0x42 } ) == "\x0a\x04\x42\x00\x00\x00"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::i32, scalar_encoder::packed ), std::vector< int > >( { 0x42, 0x3 } ) == "\x0a\x08\x42\x00\x00\x00\x03\x00\x00\x00"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::i32, scalar_encoder::packed ), std::vector< int > >( { } ) == ""sv );
                }
            }
        }
        SUBCASE( "i64" )
        {
            CHECK( pb_serialize_as< scalar_encoder::i64 >( int64_t( 0x42 ) ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
            CHECK( pb_serialize_as< scalar_encoder::i64 >( uint64_t( 0xff ) ) == "\x09\xff\x00\x00\x00\x00\x00\x00\x00"sv );
            CHECK( pb_serialize_as< scalar_encoder::i64 >( int64_t( -2 ) ) == "\x09\xfe\xff\xff\xff\xff\xff\xff\xff"sv );
            SUBCASE( "optional" )
            {
                CHECK( pb_serialize_as< scalar_encoder::i64, std::optional< int64_t > >( std::nullopt ) == "" );
                CHECK( pb_serialize_as< scalar_encoder::i64, std::optional< int64_t > >( 0x42 ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
            }
            SUBCASE( "array" )
            {
                CHECK( pb_serialize_as< scalar_encoder::i64, std::vector< int64_t > >( { 0x42 } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                CHECK( pb_serialize_as< scalar_encoder::i64, std::vector< int64_t > >( { 0x42, 0x3 } ) == "\x09\x42\x00\x00\x00\x00\x00\x00\x00\x09\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                CHECK( pb_serialize_as< scalar_encoder::i64, std::vector< int64_t > >( { } ) == ""sv );
                SUBCASE( "packed" )
                {
                    CHECK( pb_serialize_as< combine( scalar_encoder::i64, scalar_encoder::packed ), std::vector< int64_t > >( { 0x42 } ) == "\x0a\x08\x42\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::i64, scalar_encoder::packed ), std::vector< int64_t > >( { 0x42, 0x3 } ) == "\x0a\x10\x42\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00"sv );
                    CHECK( pb_serialize_as< combine( scalar_encoder::i64, scalar_encoder::packed ), std::vector< int64_t > >( { } ) == ""sv );
                }
            }
        }
    }
    SUBCASE( "double" )
    {
        CHECK( pb_serialize_as< scalar_encoder::i64 >( 42.0 ) == "\x09\x00\x00\x00\x00\x00\x00\x45\x40"sv );
        SUBCASE( "optional" )
        {
            CHECK( pb_serialize_as< scalar_encoder::i64, std::optional< double > >( std::nullopt ) == "" );
            CHECK( pb_serialize_as< scalar_encoder::i64, std::optional< double > >( 42.3 ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40" );
        }
        SUBCASE( "array" )
        {
            CHECK( pb_serialize_as< scalar_encoder::i64, std::vector< double > >( { 42.3 } ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40" );
            CHECK( pb_serialize_as< scalar_encoder::i64, std::vector< double > >( { 42.3, 3.0 } ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40\x09\x00\x00\x00\x00\x00\x00\x08\x40"sv );
            CHECK( pb_serialize_as< scalar_encoder::i64, std::vector< double > >( { } ) == "" );
        }
    }
    SUBCASE( "bytes" )
    {
        CHECK( pb_serialize( std::vector< std::byte >{ std::byte{ 0 }, std::byte{ 1 }, std::byte{ 2 } } ) == "\x0a\x03\x00\x01\x02"sv );

        CHECK( pb_serialize< std::vector< std::byte > >( { std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) } ) == "\x0a\x05\x00\x01\x02\x03\x04"sv );
        CHECK( pb_serialize< std::vector< std::byte > >( { std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } ) == "\x0a\x05hello"sv );
        CHECK( pb_serialize< std::vector< std::byte > >( { } ) == "" );

        SUBCASE( "array" )
        {
            CHECK( pb_serialize< std::vector< std::vector< std::byte > > >( std::vector< std::vector< std::byte > >{ std::vector< std::byte >{ std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) } } ) == "\x0a\x05\x00\x01\x02\x03\x04"sv );
            CHECK( pb_serialize< std::vector< std::vector< std::byte > > >( std::vector< std::vector< std::byte > >{ std::vector< std::byte >{ std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) }, std::vector< std::byte >{ std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } } ) == "\x0a\x05\x00\x01\x02\x03\x04\x0a\x05hello"sv );
            CHECK( pb_serialize< std::vector< std::vector< std::byte > > >( std::vector< std::vector< std::byte > >{ } ) == "" );
        }
        SUBCASE( "optional" )
        {
            CHECK( pb_serialize< std::optional< std::vector< std::byte > > >( std::nullopt ) == "" );
            CHECK( pb_serialize< std::optional< std::vector< std::byte > > >( std::vector< std::byte >{ std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) } ) == "\x0a\x05\x00\x01\x02\x03\x04"sv );
            CHECK( pb_serialize< std::optional< std::vector< std::byte > > >( std::vector< std::byte >{ std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } ) == "\x0a\x05hello"sv );
            CHECK( pb_serialize< std::optional< std::vector< std::byte > > >( std::vector< std::byte >{ } ) == "" );
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
    /*SUBCASE( "map" )
    {
        SUBCASE( "int32/int32" )
        {
            CHECK( sds::pb::detail::serialize_as< combine( sds::pb::detail::scalar_encoder::varint, sds::pb::detail::scalar_encoder::varint ) >( std::map< int32_t, int32_t >{ { 1, 2 } } ) == "\x08\x01\x10\x02" );
            CHECK( sds::pb::detail::serialize( std::map< int32_t, int32_t >{ { 1, 2 }, { 2, 3 } } ) == "\x08\x01\x10\x02\x08\x02\x10\x03" );
            CHECK( sds::pb::detail::serialize( std::map< int32_t, int32_t >{ } ) == "" );
        }
        SUBCASE( "string/string" )
        {
            CHECK( sds::pb::detail::serialize( std::map< std::string, std::string >{ { "hello", "world" } } ) == "\x0a\x05hello\x12\x05world" );
            CHECK( sds::pb::detail::serialize( std::map< std::string, std::string >{ } ) == "" );
        }
        SUBCASE( "int32/string" )
        {
            CHECK( sds::pb::detail::serialize( std::map< int32_t, std::string >{ { 1, "hello" } } ) == "\x08\x01\x12\x05hello" );
            CHECK( sds::pb::detail::serialize( std::map< int32_t, std::string >{ } ) == "" );
        }
        SUBCASE( "string/int32" )
        {
            CHECK( sds::pb::detail::serialize( std::map< std::string, int32_t >{ { "hello", 2 } } ) == "\x0a\x05hello\x10\x02" );
            CHECK( sds::pb::detail::serialize( std::map< std::string, int32_t >{ } ) == "" );
        }
        SUBCASE( "string/name" )
        {
            CHECK( sds::pb::detail::serialize( std::map< std::string, Test::Name >{ { "hello", { .name = "john" } } } ) == "\x0a\x05hello\x12\x06\x0A\x04john" );
        }
    }*/
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
