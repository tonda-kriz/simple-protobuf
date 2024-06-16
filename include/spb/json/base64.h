/***************************************************************************\
* Name        : base64 library for json                                     *
* Description : RFC 4648 base64 decoder and encoder                         *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace spb::json::detail
{
template < typename ostream >
static inline void base64_encode( ostream & output, std::span< const std::byte > input )
{
    static constexpr char encode_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const auto * p_char = reinterpret_cast< const uint8_t * >( input.data( ) );
    //
    //- +3 means 3 bytes are being processed in one iteration (3 * 8 = 24 bits)
    //
    for( size_t idx = 3; idx <= input.size( ); idx += 3 )
    {
        auto temp = uint32_t( *p_char++ ) << 16U;
        temp += uint32_t( *p_char++ ) << 8U;
        temp += ( *p_char++ );
        output.write( encode_table[ ( temp & 0x00FC0000U ) >> 18U ] );
        output.write( encode_table[ ( temp & 0x0003F000U ) >> 12U ] );
        output.write( encode_table[ ( temp & 0x00000FC0U ) >> 6U ] );
        output.write( encode_table[ ( temp & 0x0000003FU ) ] );
    }
    switch( input.size( ) % 3 )
    {
    case 1:
    {
        auto temp = uint32_t( *p_char++ ) << 16U;
        output.write( encode_table[ ( temp & 0x00FC0000U ) >> 18U ] );
        output.write( encode_table[ ( temp & 0x0003F000U ) >> 12U ] );
        output.write( '=' );
        output.write( '=' );
    }
    break;
    case 2:
    {
        auto temp = uint32_t( *p_char++ ) << 16U;
        temp += uint32_t( *p_char++ ) << 8U;
        output.write( encode_table[ ( temp & 0x00FC0000 ) >> 18 ] );
        output.write( encode_table[ ( temp & 0x0003F000 ) >> 12 ] );
        output.write( encode_table[ ( temp & 0x00000FC0 ) >> 6 ] );
        output.write( '=' );
    }
    break;
    }
}

[[nodiscard]] static inline auto base64_decode( std::vector< std::byte > & output, std::string_view input ) -> bool
{
    static constexpr uint8_t decode_table[ 256 ] = {
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 62, 128, 128, 128, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 128, 128, 128, 128, 128, 128,
        128, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 128, 128, 128, 128, 128,
        128, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
    };

    /*static constexpr uint8_t decode_table2[] = {
        62, 128, 128, 128, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 128, 128, 128, 128, 128, 128,
        128, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 128, 128, 128, 128, 128,
        128, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
    };*/

    if( input.empty( ) )
    {
        return true;
    }

    if( input.size( ) % 4 != 0 )
    {
        return false;
    }

    auto mask = uint8_t( 0 );

    output.resize( ( input.size( ) + 3 ) / 4 * 3 );

    auto * p_out       = output.data( );
    const auto * p_in  = reinterpret_cast< const uint8_t * >( input.data( ) );
    const auto * p_end = p_in + input.size( ) - 4;//- exclude the last 4 chars (possible padding)

    while( p_in < p_end )
    {
        uint8_t v0 = decode_table[ *p_in++ ];
        uint8_t v1 = decode_table[ *p_in++ ];
        uint8_t v2 = decode_table[ *p_in++ ];
        uint8_t v3 = decode_table[ *p_in++ ];
        mask |= ( v0 | v1 | v2 | v3 );

        *p_out++ = std::byte( ( v0 << 2 ) | ( v1 >> 4 ) );
        *p_out++ = std::byte( ( v1 << 4 ) | ( v2 >> 2 ) );
        *p_out++ = std::byte( ( v2 << 6 ) | ( v3 ) );
    }

    //- handle padding
    uint8_t v0 = decode_table[ *p_in++ ];
    uint8_t v1 = decode_table[ *p_in++ ];
    auto i1    = *p_in++;
    uint8_t v2 = i1 == '=' ? 0 : decode_table[ i1 ];
    auto i2    = *p_in++;
    uint8_t v3 = i2 == '=' ? 0 : decode_table[ i2 ];
    mask |= ( v0 | v1 | v2 | v3 );

    mask |= ( i1 == '=' && i2 != '=' ) ? 128 : 0;
    auto padding_size = ( i1 == '=' ? 1 : 0 ) + ( i2 == '=' ? 1 : 0 );

    *p_out++ = std::byte( ( v0 << 2 ) | ( v1 >> 4 ) );
    *p_out++ = std::byte( ( v1 << 4 ) | ( v2 >> 2 ) );
    *p_out++ = std::byte( ( v2 << 6 ) | ( v3 ) );

    output.resize( size_t( p_out - output.data( ) - padding_size ) );
    return ( mask & 128 ) == 0;
}
}// namespace spb::json::detail
