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
        CHECK( pb_serialize( true ) == "\x08\x01" );
        CHECK( pb_serialize( false ) == "\x08\x00"sv );
        SUBCASE( "optional" )
        {
            CHECK( pb_serialize< std::optional< bool > >( std::nullopt ) == "" );
            CHECK( pb_serialize< std::optional< bool > >( true ) == "\x08\x01" );
            CHECK( pb_serialize< std::optional< bool > >( false ) == "\x08\x00"sv );
        }
        SUBCASE( "array" )
        {
            CHECK( pb_serialize< std::vector< bool > >( { true, false } ) == "\x08\x01\x08\x00"sv );
            CHECK( pb_serialize< std::vector< bool > >( { } ) == "" );
        }
    }
    SUBCASE( "int" )
    {
        CHECK( pb_serialize( 0x42 ) == "\x08\x42" );
        CHECK( pb_serialize( 0xff ) == "\x08\xff\x01" );
        SUBCASE( "optional" )
        {
            CHECK( pb_serialize< std::optional< int > >( std::nullopt ) == "" );
            CHECK( pb_serialize< std::optional< int > >( 0x42 ) == "\x08\x42" );
        }
        SUBCASE( "array" )
        {
            CHECK( pb_serialize< std::vector< int > >( { 0x42 } ) == "\x08\x42" );
            CHECK( pb_serialize< std::vector< int > >( { 0x42, 0x3 } ) == "\x08\x42\x08\x03" );
            CHECK( pb_serialize< std::vector< int > >( { } ) == ""sv );
        }
    }
    SUBCASE( "double" )
    {
        CHECK( pb_serialize( 42.0 ) == "\x09\x00\x00\x00\x00\x00\x00\x45\x40"sv );
        SUBCASE( "optional" )
        {
            CHECK( pb_serialize< std::optional< double > >( std::nullopt ) == "" );
            CHECK( pb_serialize< std::optional< double > >( 42.3 ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40" );
        }
        SUBCASE( "array" )
        {
            CHECK( pb_serialize< std::vector< double > >( { 42.3 } ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40" );
            CHECK( pb_serialize< std::vector< double > >( { 42.3, 3.0 } ) == "\x09\x66\x66\x66\x66\x66\x26\x45\x40\x09\x00\x00\x00\x00\x00\x00\x08\x40"sv );
            CHECK( pb_serialize< std::vector< double > >( { } ) == "" );
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
}
