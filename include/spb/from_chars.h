/***************************************************************************\
* Name        : std::from_chars emulation, #if __cpp_lib_to_chars < 201611L *
* Description : string to number conversion, (std::from_chars or strtoX)    *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once
#include <charconv>

#if __cpp_lib_to_chars >= 201611L
namespace spb_std_emu = std;
#else
#include <algorithm>
#include <cassert>
#include <cctype>
#include <concepts>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace spb::std_emu
{
struct from_chars_result
{
    const char * ptr;
    std::errc ec;
};

static inline auto from_chars( const char * first, const char * last, std::integral auto & number, int base = 10 ) -> from_chars_result
{
    assert( first <= last );
    char buffer[ 32 ];
    auto copy_size = std::min< size_t >( last - first, sizeof( buffer ) - 1 );
    memcpy( buffer, first, copy_size );
    buffer[ copy_size ] = '\0';
    const char * end    = nullptr;

    static_assert( sizeof( number ) == 4 || sizeof( number ) == 8, "unsupported size" );
    errno       = 0;
    auto result = std::decay_t< decltype( number ) >{ 0 };

    if constexpr( std::is_signed_v< decltype( number ) > )
    {
        if constexpr( sizeof( number ) == 4 )
        {
            result = strtol( buffer, ( char ** ) &end, base );
        }
        else if constexpr( sizeof( number ) == 8 )
        {
            result = strtoll( buffer, ( char ** ) &end, base );
        }
    }
    else
    {
        if constexpr( sizeof( number ) == 4 )
        {
            result = strtoul( buffer, ( char ** ) &end, base );
        }
        else if constexpr( sizeof( number ) == 8 )
        {
            result = strtoull( buffer, ( char ** ) &end, base );
        }
    }
    end = first + ( end - buffer );
    if( end == first )
    {
        return { first, std::errc::invalid_argument };
    }
    if( errno == ERANGE )
    {
        return { end, std::errc::result_out_of_range };
    }
    number = result;
    return { end, std::errc( ) };
}

static inline auto from_chars( const char * first, const char * last, std::floating_point auto & number ) -> from_chars_result
{
    assert( first <= last );
    char buffer[ 256 ];
    auto copy_size = std::min< size_t >( last - first, sizeof( buffer ) - 1 );
    memcpy( buffer, first, copy_size );
    buffer[ copy_size ] = '\0';
    const char * end    = nullptr;

    static_assert( sizeof( number ) == 4 || sizeof( number ) == 8, "unsupported size" );
    errno       = 0;
    auto result = std::decay_t< decltype( number ) >{ 0 };
    if constexpr( sizeof( number ) == 4 )
    {
        result = strtof( buffer, ( char ** ) &end );
    }
    else if constexpr( sizeof( number ) == 8 )
    {
        result = strtod( buffer, ( char ** ) &end );
    }
    end = first + ( end - buffer );
    if( end == first )
    {
        return { end, std::errc::invalid_argument };
    }
    if( errno == ERANGE )
    {
        return { end, std::errc::result_out_of_range };
    }
    number = result;
    return { end, std::errc( ) };
}

}// namespace spb::std_emu
namespace spb_std_emu = spb::std_emu;
#endif
