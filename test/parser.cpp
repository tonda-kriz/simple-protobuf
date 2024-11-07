#include "ast/ast.h"
#include <cstdlib>
#include <dumper/dumper.h>
#include <parser/parser.h>
#include <sstream>
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
    { R"(syntax = "proto2";
        message ReqUint8_1
        {
            //[[ field.type = "uint8:1" ]]
            required uint32 value = 1;
        })"sv,
      0 },
    { R"(syntax = "proto2";
        message Message
        {
            required uint32 value = 1;
        })"sv,
      0 },
    { R"(syntax = "proto2";
        message ReqUint8_1
        {
            //[[ field.type = "int8" ]]
            required uint32 value = 1;
        })"sv,
      4 },
    { R"(syntax = "proto2";
        //[[enum.type = "float"]]
        enum Enum
        {
            value = 1;
        })"sv,
      2 },
    { R"(syntax = "proto2";
        message OptUint8_1
        {
            //[[ field.type = "uint8:1" ]]
            optional uint32 value = 1;
        })",
      4 },
    { R"(package UnitTest.dependency;
        message A {
        message B { optional C c = 1; }
        message C { optional int32 b = 1; }
        message D { optional A a = 1; }
        message E {
            repeated F f = 1;
            optional int32 b = 2 [default = -2];
        }
        message F {
            repeated E e = 1;
            optional int32 c = 2;
        }
        message G {
            optional H h = 1;
            optional int32 b = 2;
        }
        message H {
            optional G g = 1;
            optional int32 c = 2 [default = 2];
        }
        optional int32 a = 1;
    })",
      0 },
    {
        R"(package UnitTest;
            message A {
                optional A a = 1;
            }
        )",
        0 },
    {
        R"(package UnitTest;
            message A {
                repeated A a = 1;
            }
        )",
        0 },
    {
        R"(package UnitTest;
            message A {
                required A a = 1;
            }
        )",
        3 },
    {
        R"(package UnitTest;
            message A {
                required B b = 1;
            }
            message B {
                required A a = 1;
            }
        )",
        2 },
    {
        R"(package UnitTest;
            message A {
                message B {
                    required A a = 1;
                }
                required B b = 1;
            }
        )",
        4 },
    {
        R"(package UnitTest;
            option cc_enable_arenas = true;
            message A {
                message B {
                    repeated A a = 1;
                    required bool b = 2 [ default = true ];
                    map<int32, int32> map_field = 3;
                }
                required B b = 1;
            }
        )",
        0 },
    {
        R"(package UnitTest;
            option cc_enable_arenas true;
            )",
        2 },
    {
        R"(package UnitTest;
            message A {
                oneof oneof_field {
                    uint32 oneof_uint32 = 1;
                    string oneof_string = 2;
                    bytes oneof_bytes = 3;
                }
            }
        )",
        0 },
    {
        R"(package UnitTest;
            message A {
                oneof oneof_field {
                    uint32 oneof_uint32 = 1;
                    string oneof_string = 2;
                    bytes oneof_bytes = 3;
                    A a = 4;
                }
            }
        )",
        2 }

};

void test_proto_file( const proto_file_test & test )
{
    auto file = proto_file{ .content = std::string( test.file_content ) };
    try
    {
        auto stream = std::stringstream( );
        parse_proto_file_content( file );
        resolve_messages( file );
        dump_cpp_header( file, stream );
        dump_cpp( file, "header.pb.h", stream );
        REQUIRE( test.error_line == 0 );
    }
    catch( const std::exception & ex )
    {
        auto line = std::strtoul( ex.what( ), nullptr, 10 );
        REQUIRE( test.error_line == line );
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
