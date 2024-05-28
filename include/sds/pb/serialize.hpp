/***************************************************************************\
* Name        : serialize library for protobuf                              *
* Description : all protobuf serialization functions                        *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "wire-types.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <sys/types.h>
#include <type_traits>
#include <vector>

namespace sds::pb::detail
{
struct ostream
{
private:
    std::byte * begin;
    std::byte * end;

public:
    /**
     * @brief Construct a new ostream object
     *
     * @param p_start if null, stream will skip all writes but will still count number of written bytes
     */
    ostream( void * p_start = nullptr ) noexcept
        : begin( ( std::byte * ) p_start )
        , end( begin )
    {
    }

    void write( const void * p_data, size_t size ) noexcept
    {
        if( begin != nullptr )
        {
            memcpy( end, p_data, size );
        }

        end += size;
    }

    [[nodiscard]] auto size( ) const noexcept -> size_t
    {
        return static_cast< size_t >( end - begin );
    }

    void serialize( uint32_t field_number, const auto & value );

    template < scalar_encoder encoder >
    void serialize_as( uint32_t field_number, const auto & value );
};

static inline auto serialize_size( const auto & value ) noexcept -> size_t;
static inline auto serialize( const auto & value ) -> std::string;
static inline auto serialize( const auto & value, void * buffer ) -> size_t;

using namespace std::literals;

static inline void serialize_varint( ostream & stream, uint64_t value )
{
    size_t i = 0;
    uint8_t buffer[ 10 ];

    do
    {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        byte |= value > 0 ? 0x80U : 0;
        buffer[ i++ ] = byte;
    } while( value > 0 );

    return stream.write( buffer, i );
}
static inline void serialize_svarint( ostream & stream, int64_t value )
{
    auto tmp = uint64_t( ( value << 1 ) ^ ( value >> 63 ) );
    return serialize_varint( stream, tmp );
}

static inline void serialize_tag( ostream & stream, uint32_t field_number, wire_type type )
{
    const auto tag = ( field_number << 3 ) | uint32_t( type );
    serialize_varint( stream, tag );
}

template < class T >
concept is_struct = ::std::is_class_v< T >;

template < class T >
concept is_enum = ::std::is_enum_v< T >;

static inline void serialize( ostream & stream, uint32_t field_number, const is_struct auto & value );
static inline void serialize( ostream & stream, uint32_t field_number, const std::string_view & value );
static inline void serialize( ostream & stream, uint32_t field_number, const std::string & value );
static inline void serialize( ostream & stream, uint32_t field_number, const std::vector< std::byte > & value );

template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, std::span< const T > value );
template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, const std::vector< T > & value );

template < scalar_encoder encoder, typename T >
static inline void serialize_as( ostream & stream, uint32_t field_number, const std::vector< T > & value );

static inline void serialize( ostream & stream, uint32_t field_number, const std::floating_point auto & value );

template < scalar_encoder encoder, typename keyT, typename valueT >
static inline void serialize_as( ostream & stream, uint32_t field_number, const std::map< keyT, valueT > & value );

template < scalar_encoder encoder >
static inline void serialize( ostream & stream, std::integral auto value );

template < scalar_encoder encoder, typename T >
    requires std::is_integral_v< T >
static inline void serialize_as( ostream & stream, uint32_t field_number, const T & value )
{
    serialize_tag( stream, field_number, wire_type_from_scalar_encoder( encoder ) );
    serialize_as< encoder >( stream, value );
}

static inline void serialize( ostream & stream, uint32_t field_number, const std::floating_point auto & value )
{
    serialize_tag( stream, field_number, sizeof( value ) == sizeof( float ) ? wire_type::fixed32 : wire_type::fixed64 );
    stream.write( &value, sizeof( value ) );
}

template < scalar_encoder encoder >
static inline void serialize_as( ostream & stream, std::integral auto value )
{
    constexpr auto type = type1( encoder );

    if constexpr( type == scalar_encoder::varint )
    {
        using u_int = std::make_unsigned_t< decltype( value ) >;

        return serialize_varint( stream, u_int( value ) );
    }
    else if constexpr( type == scalar_encoder::svarint )
    {
        static_assert( std::is_signed_v< decltype( value ) > );

        return serialize_svarint( stream, value );
    }
    else if constexpr( type == scalar_encoder::i32 )
    {
        static_assert( sizeof( value ) == sizeof( uint32_t ) );
        return stream.write( &value, sizeof( value ) );
    }
    else if constexpr( type == scalar_encoder::i64 )
    {
        static_assert( sizeof( value ) == sizeof( uint64_t ) );
        return stream.write( &value, sizeof( value ) );
    }
}

static inline void serialize( ostream & stream, uint32_t field_number, const std::string & value )
{
    serialize( stream, field_number, std::string_view( value ) );
}

static inline void serialize( ostream & stream, uint32_t field_number, const bool & value )
{
    serialize_tag( stream, field_number, wire_type::varint );
    const uint8_t tmp = value ? 1 : 0;
    stream.write( &tmp, 1 );
}

template < scalar_encoder encoder >
static inline void serialize_as( ostream & stream, uint32_t field_number, const bool & value )
{
    serialize( stream, field_number, value );
}

static inline void serialize( ostream & stream, uint32_t field_number, const std::string_view & value )
{
    if( !value.empty( ) )
    {
        serialize_tag( stream, field_number, wire_type::length_delimited );
        serialize_varint( stream, value.size( ) );
        stream.write( value.data( ), value.size( ) );
    }
}

static inline void serialize( ostream & stream, uint32_t field_number, const std::vector< std::byte > & value )
{
    if( !value.empty( ) )
    {
        serialize_tag( stream, field_number, wire_type::length_delimited );
        serialize_varint( stream, value.size( ) );
        stream.write( value.data( ), value.size( ) );
    }
}

template < scalar_encoder encoder, typename keyT, typename valueT >
static inline void serialize_as( ostream & stream, const std::map< keyT, valueT > & value )
{
    const auto key_encoder   = type1( encoder );
    const auto value_encoder = type2( encoder );

    for( const auto & [ k, v ] : value )
    {
        if constexpr( std::is_integral_v< keyT > )
        {
            serialize_as< key_encoder >( stream, 1, k );
        }
        else
        {
            serialize( stream, 1, k );
        }
        if constexpr( std::is_integral_v< valueT > )
        {
            serialize_as< value_encoder >( stream, 2, v );
        }
        else
        {
            serialize( stream, 2, v );
        }
    }
}

template < scalar_encoder encoder, typename keyT, typename valueT >
static inline void serialize_as( ostream & stream, uint32_t field_number, const std::map< keyT, valueT > & value )
{
    auto size_stream = ostream( );
    serialize_as< encoder >( size_stream, value );
    const auto size = size_stream.size( );

    serialize_tag( stream, field_number, wire_type::length_delimited );
    serialize_varint( stream, size );
    serialize_as< encoder >( stream, value );
}

template < scalar_encoder encoder, typename T >
static inline void serialize_packed_as( ostream & stream, const std::vector< T > & value )
{
    for( const auto & v : value )
    {
        serialize_as< encoder >( stream, v );
    }
}

template < scalar_encoder encoder, typename T >
static inline void serialize_as( ostream & stream, uint32_t field_number, const std::vector< T > & value )
{
    if constexpr( is_packed( encoder ) )
    {
        if( value.empty( ) )
        {
            return;
        }

        auto size_stream = ostream( );
        serialize_packed_as< encoder >( size_stream, value );
        const auto size = size_stream.size( );

        serialize_tag( stream, field_number, wire_type::length_delimited );
        serialize_varint( stream, size );
        serialize_packed_as< encoder >( stream, value );
    }
    else
    {
        for( const auto & v : value )
        {
            if constexpr( std::is_integral_v< T > )
            {
                serialize_as< encoder >( stream, field_number, v );
            }
            else
            {
                serialize( stream, field_number, v );
            }
        }
    }
}

template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, const std::vector< T > & value )
{
    for( const auto & v : value )
    {
        serialize( stream, field_number, v );
    }
}

template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, const std::optional< T > & p_value )
{
    if( p_value )
    {
        return serialize( stream, field_number, *p_value );
    }
}

template < scalar_encoder encoder, typename T >
    requires std::is_integral_v< T >
static inline void serialize_as( ostream & stream, uint32_t field_number, const std::optional< T > & p_value )
{
    if( p_value )
    {
        return serialize_as< encoder >( stream, field_number, *p_value );
    }
}

template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, const std::unique_ptr< T > & p_value )
{
    if( p_value )
    {
        return serialize( stream, field_number, *p_value );
    }
}

static inline void serialize( ostream & stream, uint32_t field_number, const is_struct auto & value )
{
    if( const auto size = serialize_size( value ); size > 0 )
    {
        serialize_tag( stream, field_number, wire_type::length_delimited );
        serialize_varint( stream, size );
        //
        //- serialize is generated by the sds-protoc
        //
        serialize( stream, value );
    }
}

static inline void serialize( ostream & stream, uint32_t field_number, const is_enum auto & value )
{
    serialize_tag( stream, field_number, wire_type::varint );
    serialize_varint( stream, int32_t( value ) );
}

static inline auto serialize_size( const auto & value ) noexcept -> size_t
{
    auto stream = ostream( nullptr );

    serialize( stream, value );
    return stream.size( );
}

static inline auto serialize( const auto & value ) -> std::string
{
    auto result = std::string( serialize_size( value ), '\0' );
    auto stream = ostream( result.data( ) );

    serialize( stream, value );
    return result;
}

static inline auto serialize( const auto & value, void * buffer ) -> size_t
{
    auto stream = ostream( buffer );

    serialize( stream, value );
    return stream.size( );
}

void ostream::serialize( uint32_t field_number, const auto & value )
{
    detail::serialize( *this, field_number, value );
}

template < scalar_encoder encoder >
void ostream::serialize_as( uint32_t field_number, const auto & value )
{
    detail::serialize_as< encoder >( *this, field_number, value );
}

}// namespace sds::pb::detail