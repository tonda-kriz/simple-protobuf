/***************************************************************************\
* Name        : deserialize library for protobuf                            *
* Description : all protobuf deserialization functions                      *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "concepts.h"
#include "wire-types.h"
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace sds::pb::detail
{

struct istream
{
private:
    uint32_t m_tag;
    const uint8_t * m_tag_ptr = nullptr;
    const uint8_t * m_begin;
    const uint8_t * m_end;

    void update_tag( );

public:
    istream( const void * content, size_t size ) noexcept
        : m_begin( static_cast< const uint8_t * >( content ) )
        , m_end( m_begin + size )
    {
    }

    void skip( );

    void deserialize( auto & value );

    template < scalar_encoder encoder >
    void deserialize_as( auto & value );

    void check_wire_type( detail::wire_type type )
    {
        if( wire_type( ) != type )
        {
            throw std::runtime_error( "invalid wire type" );
        }
    }

    auto read_byte( ) -> uint8_t
    {
        if( m_begin >= m_end ) [[unlikely]]
        {
            throw std::runtime_error( "unexpected end of stream" );
        }
        return *m_begin++;
    }
    [[nodiscard]] auto size( ) const -> size_t
    {
        return m_end - m_begin;
    }

    [[nodiscard]] auto data( ) const -> const void *
    {
        return m_begin;
    }

    void read( void * data, size_t size )
    {
        if( this->size( ) < size ) [[unlikely]]
        {
            throw std::runtime_error( "unexpected end of stream" );
        }

        if( data != nullptr )
        {
            memcpy( data, m_begin, size );
        }

        m_begin += size;
    }

    [[nodiscard]] auto field( ) -> uint32_t
    {
        update_tag( );
        return m_tag >> 3;
    }
    [[nodiscard]] auto wire_type( ) -> detail::wire_type
    {
        update_tag( );
        return static_cast< detail::wire_type >( m_tag & 0x07 );
    }
    [[nodiscard]] auto empty( ) const -> bool
    {
        return m_begin >= m_end;
    }
    [[nodiscard]] auto sub_stream( size_t size ) const -> istream
    {
        return { m_begin, size };
    }
};

template < typename T >
[[nodiscard]] static inline auto read_varint( istream & stream ) -> T
{
    if constexpr( std::is_same_v< T, bool > )
    {
        stream.check_wire_type( detail::wire_type::varint );
        switch( stream.read_byte( ) )
        {
        case 0:
            return false;
        case 1:
            return true;
        default:
            throw std::runtime_error( "invalid varint for bool" );
        }
    }
    else
    {
        using unsigned_type = std::make_unsigned_t< T >;

        auto result = unsigned_type( 0 );

        for( auto shift = 0; shift < sizeof( T ) * CHAR_BIT; shift += CHAR_BIT - 1 )
        {
            uint8_t byte = stream.read_byte( );
            result |= unsigned_type( byte & 0x7F ) << shift;
            if( ( byte & 0x80 ) == 0 )
            {
                return result;
            }
        }
        throw std::runtime_error( "invalid varint" );
    }
}

static inline void deserialize( istream & stream, std::string_view & value );
static inline void deserialize( istream & stream, std::string & value );

template < scalar_encoder encoder >
static inline void deserialize_as( istream & stream, is_int_or_float auto & value );

// static inline void deserialize( istream & stream, auto & value );

template < typename T >
static inline void deserialize( istream & stream, std::vector< T > & value );

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::vector< T > & value );

template < scalar_encoder encoder, typename keyT, typename valueT >
static inline void deserialize_as( istream & stream, std::map< keyT, valueT > & value );

template < typename T >
static inline void deserialize( istream & stream, std::optional< T > & p_value );

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::optional< T > & p_value );

template < typename T >
static inline void deserialize( istream & stream, std::unique_ptr< T > & value );

template < scalar_encoder encoder >
static inline void deserialize_as( istream & stream, is_int_or_float auto & value )
{
    using T         = std::remove_cvref_t< decltype( value ) >;
    const auto type = type1( encoder );
    if constexpr( type == scalar_encoder::svarint )
    {
        if constexpr( !is_packed( encoder ) )
        {
            stream.check_wire_type( wire_type::varint );
        }
        auto tmp = read_varint< std::make_unsigned_t< T > >( stream );
        value    = T( ( tmp >> 1 ) ^ ( ~( tmp & 1 ) + 1 ) );
    }
    else if constexpr( type == scalar_encoder::varint )
    {
        if constexpr( !is_packed( encoder ) )
        {
            stream.check_wire_type( wire_type::varint );
        }
        value = read_varint< T >( stream );
    }
    else if constexpr( type == scalar_encoder::i32 )
    {
        if constexpr( !is_packed( encoder ) )
        {
            stream.check_wire_type( wire_type::fixed32 );
        }
        static_assert( sizeof( value ) == sizeof( uint32_t ) );
        stream.read( &value, sizeof( value ) );
    }
    else if constexpr( type == scalar_encoder::i64 )
    {
        if constexpr( !is_packed( encoder ) )
        {
            stream.check_wire_type( wire_type::fixed64 );
        }
        static_assert( sizeof( value ) == sizeof( uint64_t ) );
        stream.read( &value, sizeof( value ) );
    }
}

static inline void deserialize( istream & stream, is_enum auto & value )
{
    using T        = std::remove_cvref_t< decltype( value ) >;
    using int_type = std::underlying_type_t< T >;

    stream.check_wire_type( wire_type::varint );
    value = T( read_varint< int_type >( stream ) );
}

template < typename T >
static inline void deserialize( istream & stream, std::optional< T > & p_value )
{
    auto & value = p_value.emplace( T( ) );
    deserialize( stream, value );
}

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::optional< T > & p_value )
{
    auto & value = p_value.emplace( T( ) );
    deserialize_as< encoder >( stream, value );
}

static inline void deserialize( istream & stream, std::string_view & value )
{
    stream.check_wire_type( wire_type::length_delimited );
    const auto size = read_varint< uint32_t >( stream );
    value           = std::string_view( static_cast< const char * >( stream.data( ) ), size );
    stream.read( nullptr, size );
}

static inline void deserialize( istream & stream, std::string & value )
{
    stream.check_wire_type( wire_type::length_delimited );
    const auto size = read_varint< uint32_t >( stream );
    value.resize( size );
    stream.read( value.data( ), size );
}

template < typename T >
static inline void deserialize( istream & stream, std::unique_ptr< T > & value )
{
    value = std::make_unique< T >( );
    deserialize( stream, *value );
}

template < typename T >
static inline void deserialize( istream & stream, std::vector< T > & value )
{
    deserialize( stream, value.emplace_back( ) );
}

template < scalar_encoder encoder, typename T >
static inline void deserialize_packed_as( istream & stream, std::vector< T > & value )
{
    while( !stream.empty( ) )
    {
        if constexpr( std::is_same_v< T, bool > )
        {
            value.emplace_back( read_varint< bool >( stream ) );
        }
        else
        {
            deserialize_as< encoder >( stream, value.emplace_back( ) );
        }
    }
}

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::vector< T > & value )
{
    if constexpr( is_packed( encoder ) )
    {
        stream.check_wire_type( wire_type::length_delimited );
        const auto size = read_varint< uint32_t >( stream );
        auto substream  = stream.sub_stream( size );
        stream.read( nullptr, size );
        deserialize_packed_as< encoder >( substream, value );
    }
    else
    {
        if constexpr( std::is_same_v< T, bool > )
        {
            value.emplace_back( read_varint< bool >( stream ) );
        }
        else
        {
            deserialize_as< encoder >( stream, value.emplace_back( ) );
        }
    }
}

template < scalar_encoder encoder, typename keyT, typename valueT >
static inline void deserialize_as( istream & stream, std::map< keyT, valueT > & value )
{
    const auto key_encoder   = type1( encoder );
    const auto value_encoder = type2( encoder );

    stream.check_wire_type( wire_type::length_delimited );
    const auto size  = read_varint< uint32_t >( stream );
    auto item_stream = stream.sub_stream( size );
    stream.read( nullptr, size );

    auto pair          = std::pair< keyT, valueT >( );
    auto key_defined   = false;
    auto value_defined = false;
    while( !item_stream.empty( ) )
    {
        switch( item_stream.field( ) )
        {
        case 1:
            if constexpr( std::is_integral_v< keyT > )
            {
                deserialize_as< key_encoder >( stream, pair.first );
            }
            else
            {
                deserialize( stream, pair.first );
            }
            key_defined = true;
            break;
        case 2:
            if constexpr( is_int_or_float< valueT > )
            {
                deserialize_as< value_encoder >( stream, pair.second );
            }
            else
            {
                deserialize( stream, pair.second );
            }
            value_defined = true;
            break;
        default:
            throw std::runtime_error( "invalid field" );
        }
    }
    if( key_defined && value_defined )
    {
        value.insert( std::move( pair ) );
    }
    else
    {
        throw std::runtime_error( "invalid map item" );
    }
}

inline void istream::deserialize( auto & value )
{
    detail::deserialize( *this, value );
}

inline void istream::update_tag( )
{
    if( m_tag_ptr != m_begin )
    {
        m_tag     = read_varint< uint32_t >( *this );
        m_tag_ptr = m_begin;
    }
}

inline void istream::skip( )
{
    switch( wire_type( ) )
    {
    case wire_type::varint:
        return ( void ) read_varint< uint64_t >( *this );
    case wire_type::length_delimited:
        return read( nullptr, read_varint< uint32_t >( *this ) );
    case wire_type::fixed32:
        return read( nullptr, sizeof( uint32_t ) );
    case wire_type::fixed64:
        return read( nullptr, sizeof( uint64_t ) );
    default:
        throw std::runtime_error( "invalid wire type" );
    }
}

template < scalar_encoder encoder >
inline void istream::deserialize_as( auto & value )
{
    detail::deserialize_as< encoder >( *this, value );
}

template < typename Result >
static inline auto deserialize( std::string_view protobuf ) -> Result
{
    auto stream = detail::istream( protobuf.data( ), protobuf.size( ) );
    auto result = Result{ };
    detail::deserialize( stream, result );
    return result;
}

static inline void deserialize( std::string_view string, auto & value )
{
    auto stream = detail::istream( string.data( ), string.size( ) );
    detail::deserialize( stream, value );
}

}// namespace sds::pb::detail
