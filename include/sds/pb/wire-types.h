/***************************************************************************\
* Name        : serialize library for protobuf                              *
* Description : encoding in protobuf                                        *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include <cstdint>
#include <type_traits>

namespace sds::pb::detail
{

//- https://protobuf.dev/programming-guides/encoding/
enum class wire_type : uint8_t
{
    //- int32, int64, uint32, uint64, sint32, sint64, bool, enum
    varint = 0,
    //- fixed64, sfixed64, double
    fixed64 = 1,
    //- string, bytes, embedded messages, packed repeated fields
    length_delimited = 2,
    //- not used
    StartGroup = 3,
    //- not used
    EndGroup = 4,
    //- fixed32, sfixed32, float
    fixed32 = 5
};

//- type1, type2 and packed flag
enum class scalar_encoder : uint8_t
{
    //- int32, int64, uint32, uint64, bool, enum
    varint = 0,
    //- zigzag int32 or int64
    svarint = 1,
    //- 4 bytes
    i32 = 2,
    //- 8 bytes
    i64 = 3,
    //- packed array
    packed = 0x80
};

static constexpr auto combine( scalar_encoder a, scalar_encoder b ) noexcept -> scalar_encoder
{
    if( b == scalar_encoder::packed )
    {
        return scalar_encoder( static_cast< std::underlying_type_t< scalar_encoder > >( a ) | static_cast< std::underlying_type_t< scalar_encoder > >( b ) );
    }

    return scalar_encoder( static_cast< std::underlying_type_t< scalar_encoder > >( a ) | ( static_cast< std::underlying_type_t< scalar_encoder > >( b ) << 2 ) );
}

static constexpr auto type1( scalar_encoder a ) noexcept -> scalar_encoder
{
    return scalar_encoder( static_cast< std::underlying_type_t< scalar_encoder > >( a ) & 0x03 );
}

static constexpr auto type2( scalar_encoder a ) noexcept -> scalar_encoder
{
    return scalar_encoder( ( static_cast< std::underlying_type_t< scalar_encoder > >( a ) >> 2 ) & 0x03 );
}

static constexpr auto is_packed( scalar_encoder a ) noexcept -> bool
{
    return static_cast< std::underlying_type_t< scalar_encoder > >( a ) & static_cast< std::underlying_type_t< scalar_encoder > >( scalar_encoder::packed );
}

static constexpr auto wire_type_from_scalar_encoder( scalar_encoder a ) noexcept -> wire_type
{
    switch( type1( a ) )
    {
    case scalar_encoder::i32:
        return wire_type::fixed32;
    case scalar_encoder::i64:
        return wire_type::fixed64;
    default:
        return wire_type::varint;
    }
}

}// namespace sds::pb::detail