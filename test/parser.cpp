#include "ast/ast.h"
#include "io/file.h"
#include <cstdlib>
#include <dumper/dumper.h>
#include <filesystem>
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

void test_proto_file( const proto_file_test & test )
{
    REQUIRE_NOTHROW( save_file( "tmp_test.proto", test.file_content ) );
    try
    {
        auto file   = parse_proto_file( "tmp_test.proto", { { "." } } );
        auto stream = std::stringstream( );
        dump_cpp_header( file, stream );
        dump_cpp( file, "header.pb.h", stream );
        REQUIRE( test.error_line == 0 );
    }
    catch( const std::exception & ex )
    {
        auto message = std::string_view( ex.what( ) );
        REQUIRE( message.starts_with( "tmp_test.proto:" ) );
        message.remove_prefix( message.find( ':' ) + 1 );
        auto line = std::strtoul( message.data( ), nullptr, 10 );
        REQUIRE( test.error_line == line );
    }
    REQUIRE_NOTHROW( std::filesystem::remove( "tmp_test.proto" ) );
}

void test_files( std::span< const proto_file_test > files )
{
    for( const auto & file : files )
    {
        test_proto_file( file );
    }
}
}// namespace

TEST_CASE( "protoc-parser" )
{
    SUBCASE( "api" )
    {
        REQUIRE_THROWS( parse_proto_file( "not_found.proto", { { "../", ".", "/home/" } } ) );
        REQUIRE( cpp_file_name_from_proto( "messages.proto", ".cpp" ) == "messages.cpp" );
    }
    SUBCASE( "import" )
    {
        REQUIRE_NOTHROW( save_file( "empty.proto", "" ) );

        constexpr proto_file_test tests[] = {
            { R"(package UnitTest;
            import "empty.proto";
            )",
              0 },
            { R"(package UnitTest;
            import "empty_not_found.proto";
            )",
              0 },// TODO: fix proper line error for not found imports
            { R"(package UnitTest;
            importX "message.proto";
            )",
              2 },
        };
        test_files( tests );
        REQUIRE_NOTHROW( std::filesystem::remove( "empty.proto" ) );
    }
    SUBCASE( "comment" )
    {
        constexpr proto_file_test tests[] = {
            { "//"sv, 1 },
            { "/*"sv, 1 },
            { "/-"sv, 1 },
            { R"(package UnitTest;
            // comment
            )",
              0 },
            { "/**/"sv, 0 },
            { R"(package UnitTest;
            /* comment
            */;
            )",
              0 },
            { R"(syntax = "proto3";
                message Message
                {
                    required uint32 value = 1; // comment
                })"sv,
              0 },
            { R"(syntax = "proto3";
                message Message
                {
                    // comment
                    required uint32 value = 1; 
                })"sv,
              0 },
            { R"(syntax = "proto3";
                message Message
                {
                    // comment
                    required uint32 value = 1; // comment 
                })"sv,
              0 }
        };
        test_files( tests );
    }
    SUBCASE( "syntax" )
    {
        constexpr proto_file_test tests[] = {
            { ""sv, 0 },
            { "X"sv, 1 },
            { R"(synta = "proto2";)"sv, 1 },
            { R"(syntax = "proto1;")"sv, 1 },
            { R"(syntax = "proto2")"sv, 1 },
            { R"(syntax = "proto2";)"sv, 0 },
            { R"(syntax = "proto3";;)"sv, 0 },
            { R"(syntax = "proto3")"sv, 1 },
            { R"(syntax = "proto4;")"sv, 1 },
            { R"(syntax = "proto2";
                message Message
                {
                    required uint32 value = 1;
                })"sv,
              0 },
            { R"(syntax = "proto3";
                message Message
                {
                    required uint32 value = 1;
                })"sv,
              0 }
        };
        test_files( tests );
    }
    SUBCASE( "scalar" )
    {
        constexpr proto_file_test tests[] = {
            { R"(message Message{
                bool b = 1;
                float f = 2;
                double d = 3;
                int32 i32 = 4;
                sint32 si32 = 5;
                uint32 u32 = 6;
                int64 i64 = 7;
                sint64 si64 = 8;
                uint64 u64 = 9;
                fixed32 f32 = 10;
                sfixed32 sf32 = 11;
                fixed64 f64 = 12;
                sfixed64 sf64 = 13;
                string s = 14;
                bytes by = 15;
                })"sv,
              0 },
            { R"(message Message{
                bool b = 1 [default = true];
                float f = 2 [default = 0.4];
                double d = 3 [default = 0.];
                int32 i32 = 4 [default = -2];
                sint32 si32 = 5 [default = -3];
                uint32 u32 = 6 [default = 5];
                int64 i64 = 7 [default = 7];
                sint64 si64 = 8 [default = 8];
                uint64 u64 = 9 [default = 6];
                fixed32 f32 = 10 [default = 3];
                sfixed32 sf32 = 11 [default = 6];
                fixed64 f64 = 12 [default = 3];
                sfixed64 sf64 = 13 [default = 4];
                string s = 14 [default = "true"];
                bytes by = 15;
                })"sv,
              0 },
            { R"(message Message{
                    bool b = X;
                })"sv,
              2 },
        };
        test_files( tests );
    }
    SUBCASE( "option" )
    {
        constexpr proto_file_test tests[] = {
            { R"(package UnitTest;
            option cc_enable_arenas true;
            )",
              2 },
            { R"(package UnitTest;
            option cc_enable_arenas = true;
            )",
              0 },
            { R"(syntax = "proto3";
                message Message
                {
                    required uint32 value = 1 [default = 2, deprecated = true];
                })"sv,
              0 }
        };
        test_files( tests );
    }
    SUBCASE( "extensions" )
    {
        SUBCASE( "enum type" )
        {
            constexpr proto_file_test tests[] = {
                { R"(syntax = "proto2";
                //[[enum.type = "float"]]
                enum Enum {
                    value = 1;
                })"sv,
                  2 }

            };
            test_files( tests );
        }
        SUBCASE( "bitfield" )
        {
            constexpr proto_file_test tests[] = {
                { R"(syntax = "proto2";
                message ReqUint8_1{
                    //[[ field.type = "uint8:1" ]]
                    required uint32 value = 1;
                })"sv,
                  0 },
                { R"(syntax = "proto2";
                message OptUint8_1{
                    //[[ field.type = "uint8:1" ]]
                    optional uint32 value = 1;
                })",
                  3 },

            };
            test_files( tests );
        }
        SUBCASE( "small int" )
        {
            constexpr proto_file_test tests[] = {
                { R"(package UnitTest;
                message A {
                    // [[ field.type = "uint8" ]]
                    optional uint32 u32 = 1;
                })",
                  0 },
                { R"(package UnitTest;
                message A {
                    // [[ field.type = "uint16" ]]
                    optional uint32 u32 = 1;
                })",
                  0 },
                { R"(package UnitTest;
                message A {
                    // [[ field.type = "uint32" ]]
                    optional uint32 u32 = 1;
                })",
                  0 },
                { R"(package UnitTest;
                message A {
                    // [[ field.type = "uint64" ]]
                    optional uint32 u32 = 1;
                })",
                  3 },
                { R"(syntax = "proto2";
                message ReqUint8_1{
                    //[[ field.type = "int8" ]]
                    required uint32 value = 1;
                })"sv,
                  3 },
            };
            test_files( tests );
        }
    }
    SUBCASE( "dependency" )
    {
        constexpr proto_file_test tests[] = {
            { R"(package UnitTest;
            message A {
                optional A a = 1;
            })",
              0 },
            { R"(package UnitTest;
            message A {
                repeated A a = 1;
            })",
              0 },
            { R"(package UnitTest;
            message A {
                required A a = 1;
            })",
              3 },
            { R"(package UnitTest;
            message A {
                required B b = 1;
            }
            message B {
                required A a = 1;
            })",
              2 },
            { R"(package UnitTest;
            message A {
                message B {
                    required A a = 1;
                }
                required B b = 1;
            })",
              4 },
            { R"(package UnitTest;
            option cc_enable_arenas = true;
            message A {
                message B {
                    repeated A a = 1;
                    required bool b = 2 [ default = true ];
                    map<int32, int32> map_field = 3;
                }
                required B b = 1;
            })",
              0 },
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
            { R"(package UnitTest;
            message A {
                oneof oneof_field {
                    uint32 oneof_uint32 = 1;
                    string oneof_string = 2;
                    bytes oneof_bytes = 3;
                    A a = 4;
                }
            })",
              2 },
        };
        test_files( tests );
    }
    SUBCASE( "oneof" )
    {
        constexpr proto_file_test tests[] = {
            { R"(package UnitTest;
            message A {
                oneof oneof_field {
                    uint32 oneof_uint32 = 1;
                    string oneof_string = 2;
                    bytes oneof_bytes = 3;
                }
            })",
              0 },

        };
        test_files( tests );
    }
    SUBCASE( "reserved" )
    {
        constexpr proto_file_test tests[] = {
            { R"(package UnitTest;
            message A {
                reserved 4;
                reserved 5,6,7;
                reserved 8 to 10;
                reserved 10 to max;
                reserved "BB";
            })",
              0 },
        };
        test_files( tests );
    }
    SUBCASE( "enum" )
    {
        constexpr proto_file_test tests[] = {
            { R"(package UnitTest;
            message A {
                enum PhoneType {
                    MOBILE = 0;
                    HOME = 1;
                    WORK = 2;
                }
            })",
              0 },

        };
        test_files( tests );
    }
}
