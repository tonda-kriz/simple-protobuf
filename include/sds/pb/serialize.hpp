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

    template < wire_encoder encoder >
    void serialize_as( uint32_t field_number, const auto & value );
};

/**
 * @brief return protobuf serialized size in bytes
 *
 * @param value to be serialized
 * @return serialized size in bytes
 */
static inline auto serialize_size( const auto & value ) noexcept -> size_t;

/**
 * @brief serialize value into json-string
 *
 * @param value to be serialized
 * @return serialized json
 */
static inline auto serialize( const auto & value ) -> std::string;

/**
 * @brief serialize value into buffer.
 *        Warning: user is responsible for the buffer to be big enough.
 *        Use `auto buffer_size = serialize_size( value );` to obtain the buffer size
 *
 * @param value to be serialized
 * @param buffer buffer with atleast serialize_size(value) bytes
 */
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

static inline void serialize_tag( ostream & stream, uint32_t field_number, wire_type type )
{
    const auto tag = ( field_number << 3 ) | uint32_t( type );
    serialize_varint( stream, tag );
}

template < class T >
concept is_struct = ::std::is_class_v< T >;

template < class T >
concept is_enum = ::std::is_enum_v< T >;

template < typename T >
static inline constexpr auto wire_type_from( ) -> wire_type
{
    if constexpr( is_enum< T > || std::is_integral_v< T > || std::is_same_v< T, bool > )
    {
        return wire_type::varint;
    }
    if constexpr( std::is_same_v< T, float > )
    {
        return wire_type::fixed32;
    }
    if constexpr( std::is_same_v< T, double > )
    {
        return wire_type::fixed64;
    }
    return wire_type::length_delimited;
}

static inline void serialize( ostream & stream, uint32_t field_number, const auto & value );
static inline void serialize( ostream & stream, uint32_t field_number, const is_struct auto & value );
static inline void serialize( ostream & stream, uint32_t field_number, const std::string_view & value );
static inline void serialize( ostream & stream, uint32_t field_number, const std::string & value );
static inline void serialize( ostream & stream, uint32_t field_number, const std::vector< std::byte > & value );

template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, std::span< const T > value );
template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, const std::vector< T > & value );

template < typename keyT, typename valueT >
static inline void serialize( ostream & stream, const std::map< keyT, valueT > & value );

static inline void serialize( ostream & stream, std::floating_point auto value );
static inline void serialize( ostream & stream, std::integral auto value );

template < typename T >
static inline void serialize( ostream & stream, uint32_t field_number, const T & value )
{
    serialize_tag( stream, field_number, wire_type_from< T >( ) );
    serialize( stream, value );
}

static inline void serialize( ostream & stream, std::floating_point auto value )
{
    stream.write( &value, sizeof( value ) );
}

static inline void serialize( ostream & stream, std::integral auto value )
{
    serialize_varint( stream, value );
}

static inline void serialize( ostream & stream, uint32_t field_number, const std::string & value )
{
    serialize( stream, field_number, std::string_view( value ) );
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

template < typename keyT, typename valueT >
static inline void serialize( ostream & stream, const std::map< keyT, valueT > & value )
{
    for( const auto & [ k, v ] : value )
    {
        serialize( stream, 1, k );
        serialize( stream, 2, v );
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

}// namespace sds::pb::detail