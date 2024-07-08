#include <parser/parser.h>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace
{
using namespace std::literals;

struct proto_file_test
{
    const std::string_view file_content;
    size_t error_line = 0;
};

constexpr proto_file_test tests[] = {
    { ""sv, 0 },
    { "X"sv, 1 },
    { R"(syntax = "proto1;")"sv, 1 },
    { R"(syntax = "proto2")"sv, 1 },
    { R"(syntax = "proto2";)"sv, 0 },
    { R"(syntax = "proto3";)"sv, 0 },
    { R"(syntax = "proto3")"sv, 1 },
    { R"(syntax = "proto4;")"sv, 1 },

};

void test_proto_file( const proto_file_test & test )
{
    auto file = proto_file{ .content = std::string( test.file_content ) };
    try
    {
        parse_proto_file_content( file );
        REQUIRE( test.error_line == 0 );
    }
    catch( const std::exception & ex )
    {
        auto message = std::string_view( ex.what( ) );
        auto line    = std::to_string( test.error_line ) + ":";
        REQUIRE( message.starts_with( line ) );
    }
}

}// namespace

TEST_CASE( "protoc-parser" )
{
    for( const auto & test : tests )
    {
        test_proto_file( test );
    }
}
