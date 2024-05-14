#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/json.pb.h"
#include <cstdio>

TEST_CASE( "first" )
{
    const auto fl = first_last{
        .first = "22first22"
    };

    printf( "serialized size: %d\n", int( sds::json::serialize_size( fl ) ) );

    const auto json = sds::json::serialize( fl );
    printf( "serialized: %s\n", json.c_str( ) );

    auto fl2 = sds::json::deserialize< first_last >( json );
    CHECK( fl == fl2 );
}

TEST_CASE( "last" )
{
    const auto fl = first_last{
        .last = "33last33"
    };

    printf( "serialized size: %d\n", int( sds::json::serialize_size( fl ) ) );

    const auto json = sds::json::serialize( fl );
    printf( "serialized: %s\n", json.c_str( ) );

    auto fl2 = sds::json::deserialize< first_last >( json );
    CHECK( fl == fl2 );
}

TEST_CASE( "empty" )
{
    const auto fl = first_last{ };

    printf( "serialized size: %d\n", int( sds::json::serialize_size( fl ) ) );

    const auto json = sds::json::serialize( fl );
    printf( "serialized: %s\n", json.c_str( ) );

    auto fl2 = sds::json::deserialize< first_last >( json );
    CHECK( fl == fl2 );
}
