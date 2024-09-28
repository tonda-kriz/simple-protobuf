#include <optional>
#include <spb/json/serialize.hpp>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <spb/json/base64.h>

namespace
{
auto base64_encode_size( std::string_view value ) -> size_t
{
    auto stream = spb::json::detail::ostream( nullptr );
    spb::json::detail::base64_encode( stream, { reinterpret_cast< const std::byte * >( value.data( ) ), value.size( ) } );
    return stream.size( );
}

auto base64_encode( std::string_view value ) -> std::string
{
    auto encode_size = base64_encode_size( value );
    auto result      = std::string( encode_size, '\0' );
    auto writer      = [ ptr = result.data( ) ]( const void * data, size_t size ) mutable
    {
        memcpy( ptr, data, size );
        ptr += size;
    };
    auto stream = spb::json::detail::ostream( writer );
    spb::json::detail::base64_encode( stream, { reinterpret_cast< const std::byte * >( value.data( ) ), value.size( ) } );
    return result;
}

auto base64_decode( std::string_view value ) -> std::string
{
    auto result = std::vector< std::byte >( );

    CHECK( spb::json::detail::base64_decode( result, value ) );

    return { std::string( reinterpret_cast< const char * >( result.data( ) ), reinterpret_cast< const char * >( result.data( ) + result.size( ) ) ) };
}

}// namespace

TEST_CASE( "base64" )
{
    SUBCASE( "base64_encode" )
    {
        CHECK( base64_encode( "hello world" ) == "aGVsbG8gd29ybGQ=" );

        CHECK( base64_encode( "" ) == "" );
        CHECK( base64_encode( "f" ) == "Zg==" );
        CHECK( base64_encode( "fo" ) == "Zm8=" );
        CHECK( base64_encode( "foo" ) == "Zm9v" );
        CHECK( base64_encode( "foob" ) == "Zm9vYg==" );
        CHECK( base64_encode( "fooba" ) == "Zm9vYmE=" );
        CHECK( base64_encode( "foobar" ) == "Zm9vYmFy" );
    }
    SUBCASE( "base64_decode" )
    {
        SUBCASE( "invalid" )
        {
            auto result = std::vector< std::byte >( );
            CHECK( spb::json::detail::base64_decode( result, "Zg=" ) == false );
            CHECK( spb::json::detail::base64_decode( result, "Zg" ) == false );
            CHECK( spb::json::detail::base64_decode( result, "Z" ) == false );
        }

        CHECK( base64_decode( "" ) == "" );
        CHECK( base64_decode( "Zg==" ) == "f" );
        CHECK( base64_decode( "Zm8=" ) == "fo" );
        CHECK( base64_decode( "Zm9v" ) == "foo" );
        CHECK( base64_decode( "Zm9vYg==" ) == "foob" );
        CHECK( base64_decode( "Zm9vYmE=" ) == "fooba" );
        CHECK( base64_decode( "Zm9vYmFy" ) == "foobar" );
    }
}