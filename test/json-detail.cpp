#include <cstdint>
#include <memory>
#include <person.pb.h>
#include <sds/json/deserialize.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE( "json" )
{
    SUBCASE( "dj2hash" )
    {
        const auto hash  = sds::json::detail::djb2_hash( "hello" );
        const auto hash2 = sds::json::detail::djb2_hash( "world" );
        const auto hash3 = sds::json::detail::djb2_hash( { } );
        CHECK( hash3 != 0 );
        CHECK( hash != 0 );
        CHECK( hash2 != 0 );
        CHECK( hash != hash2 );
        CHECK( hash3 != hash2 );
        CHECK( hash3 != hash );
    }
    SUBCASE( "deserialize" )
    {
        SUBCASE( "person" )
        {
            constexpr std::string_view jsons[] = {
                R"({"name": "John Doe","id": 123,"email": "QXUeh@example.com", "phones": [ {"number": "555-4321","type": "HOME"}]})",
                R"({"name": "John Doe","id": 123,"email": "QXUeh@example.com", "phones": [ {"number": "555-4321","type": 1}]})",
                R"({"name2": "Jack","name": "John Doe","id": 123,"email": "QXUeh@example.com", "phones": [ {"number": "555-4321","type": "HOME"}]})",
            };
            for( auto json : jsons )
            {
                const auto person = sds::json::deserialize< PhoneBook::Person >( json );
                CHECK( person.name == "John Doe" );
                CHECK( person.id == 123 );
                CHECK( person.email == "QXUeh@example.com" );
                CHECK( person.phones.size( ) == 1 );
                CHECK( person.phones[ 0 ].number == "555-4321" );
                CHECK( person.phones[ 0 ].type == PhoneBook::Person::PhoneType::HOME );
            }
        }
        SUBCASE( "bool" )
        {
            CHECK( sds::json::detail::deserialize< bool >( "true" ) == true );
            CHECK( sds::json::detail::deserialize< bool >( "false" ) == false );
            CHECK_THROWS( sds::json::detail::deserialize< bool >( "hello" ) );
            auto value = false;
            CHECK_NOTHROW( sds::json::detail::deserialize( "true", value ) );
            CHECK( value );
            CHECK_NOTHROW( sds::json::detail::deserialize( "false", value ) );
            CHECK( value == false );
            SUBCASE( "array" )
            {
                CHECK( sds::json::detail::deserialize< std::vector< bool > >( R"([true,false])" ) == std::vector< bool >{ true, false } );
                CHECK( sds::json::detail::deserialize< std::vector< bool > >( R"([])" ) == std::vector< bool >{ } );
                CHECK( sds::json::detail::deserialize< std::vector< bool > >( R"(null)" ) == std::vector< bool >{ } );
                CHECK_THROWS( sds::json::detail::deserialize< std::vector< bool > >( R"()" ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::vector< bool > >( R"(true)" ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::vector< bool > >( R"([null])" ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::vector< bool > >( R"("hello")" ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::vector< bool > >( R"([)" ) );
            }
            SUBCASE( "optional" )
            {
                CHECK( sds::json::detail::deserialize< std::optional< bool > >( "true" ) == std::optional< bool >( true ) );
                CHECK( sds::json::detail::deserialize< std::optional< bool > >( "false" ) == std::optional< bool >( false ) );
                CHECK( sds::json::detail::deserialize< std::optional< bool > >( "null" ) == std::optional< bool >( ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::optional< bool > >( "hello" ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::optional< bool > >( R"()" ) );

                auto value = std::optional< bool >( );
                CHECK_NOTHROW( sds::json::detail::deserialize( "true", value ) );
                CHECK( value == true );
                CHECK_NOTHROW( sds::json::detail::deserialize( "false", value ) );
                CHECK( value == false );
            }
            SUBCASE( "ptr" )
            {
                CHECK( *sds::json::detail::deserialize< std::unique_ptr< bool > >( "true" ) == true );
                CHECK( *sds::json::detail::deserialize< std::unique_ptr< bool > >( "false" ) == false );
                CHECK( sds::json::detail::deserialize< std::unique_ptr< bool > >( "null" ) == std::unique_ptr< bool >( ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::unique_ptr< bool > >( "hello" ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::unique_ptr< bool > >( R"()" ) );

                auto value = std::unique_ptr< bool >( );
                CHECK_NOTHROW( sds::json::detail::deserialize( "true", value ) );
                CHECK( *value == true );
                CHECK_NOTHROW( sds::json::detail::deserialize( "false", value ) );
                CHECK( *value == false );
            }
        }
        SUBCASE( "array" )
        {
            CHECK( sds::json::detail::deserialize< std::vector< std::string > >( R"(["hello","world"])" ) == std::vector< std::string >{ "hello", "world" } );
            CHECK( sds::json::detail::deserialize< std::vector< std::string > >( R"([])" ) == std::vector< std::string >{ } );
            CHECK( sds::json::detail::deserialize< std::vector< std::string > >( R"(null)" ) == std::vector< std::string >{ } );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"()" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"(true)" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"("hello"])" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"([)" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"(["hello")" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"(["hello",)" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"(["hello",])" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::string > >( R"(["hello","world")" ) );
        }
        SUBCASE( "bytes" )
        {
            CHECK( sds::json::detail::deserialize< std::vector< std::byte > >( R"("AAECAwQ=")" ) == std::vector< std::byte >{ std::byte( 0 ), std::byte( 1 ), std::byte( 2 ), std::byte( 3 ), std::byte( 4 ) } );
            CHECK( sds::json::detail::deserialize< std::vector< std::byte > >( R"("aGVsbG8=")" ) == std::vector< std::byte >{ std::byte( 'h' ), std::byte( 'e' ), std::byte( 'l' ), std::byte( 'l' ), std::byte( 'o' ) } );
            CHECK( sds::json::detail::deserialize< std::vector< std::byte > >( R"(null)" ) == std::vector< std::byte >{ } );
            CHECK( sds::json::detail::deserialize< std::vector< std::byte > >( R"("")" ) == std::vector< std::byte >{ } );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::byte > >( R"(true)" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::byte > >( R"("AAECAwQ")" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::byte > >( R"([])" ) );
            CHECK_THROWS( sds::json::detail::deserialize< std::vector< std::byte > >( R"()" ) );
        }

        SUBCASE( "string" )
        {
            SUBCASE( "escape" )
            {
                for( int c = 0; c <= 0xff; c++ )
                {
                    const auto esc = c == '"' || c == '\\' || c == '/' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't';

                    CHECK( sds::json::detail::is_escape( c ) == esc );
                    char buffer[] = { '"', '\\', char( c ), '"', 0 };
                    if( !esc )
                    {
                        CHECK_THROWS( sds::json::detail::deserialize< std::string >( buffer ) );
                    }
                }

                CHECK( sds::json::detail::deserialize< std::string >( R"("hello")" ) == "hello" );
                CHECK( sds::json::detail::deserialize< std::string_view >( R"("hello")" ) == "hello" );
                CHECK_THROWS( sds::json::detail::deserialize< std::string >( R"(hello")" ) );
                CHECK_THROWS( sds::json::detail::deserialize< std::string >( R"("hello)" ) );
                SUBCASE( "escaped" )
                {
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\"")" ) == "\"" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\\")" ) == "\\" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\/")" ) == "/" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\b")" ) == "\b" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\f")" ) == "\f" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\n")" ) == "\n" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\r")" ) == "\r" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\t")" ) == "\t" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\"\b\f\n\r\t\"")" ) == "\"\b\f\n\r\t\"" );
                    CHECK( sds::json::detail::deserialize< std::string >( R"("\nhell\to")" ) == "\nhell\to" );
                }
            }
        }
        SUBCASE( "serialize" )
        {
        }
    }
}