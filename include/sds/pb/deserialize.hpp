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
    const uint8_t * m_begin;
    const uint8_t * m_end;

public:
    istream( const void * content, size_t size ) noexcept
        : m_begin( static_cast< const uint8_t * >( content ) )
        , m_end( m_begin + size )
    {
    }

    void skip( uint32_t tag );

    void deserialize( auto & value, uint32_t tag );

    template < scalar_encoder encoder >
    void deserialize_as( auto & value, uint32_t tag );

    template < size_t ordinal, typename T >
    void deserialize_variant( T & variant, uint32_t tag );

    template < size_t ordinal, scalar_encoder encoder, typename T >
    void deserialize_variant_as( T & variant, uint32_t tag );

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

    [[nodiscard]] auto empty( ) const -> bool
    {
        return m_begin >= m_end;
    }
    [[nodiscard]] auto sub_stream( size_t size ) -> istream
    {
        auto result = istream( m_begin, size );
        read( nullptr, size );
        return result;
    }
};

[[nodiscard]] static inline auto wire_type_from_tag( uint32_t tag ) -> wire_type
{
    return wire_type( tag & 0x07 );
}

[[nodiscard]] static inline auto field_from_tag( uint32_t tag ) -> uint32_t
{
    return tag >> 3;
}

static inline void check_wire_type( wire_type type1, wire_type type2 )
{
    if( type1 != type2 )
    {
        throw std::runtime_error( "invalid wire type" );
    }
}

template < typename T >
[[nodiscard]] static inline auto read_varint( istream & stream ) -> T
{
    if constexpr( std::is_same_v< T, bool > )
    {
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

        for( auto shift = 0U; shift < sizeof( T ) * CHAR_BIT; shift += CHAR_BIT - 1 )
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

static inline void deserialize( istream & stream, std::string_view & value, wire_type type );
static inline void deserialize( istream & stream, std::string & value, wire_type type );
static inline void deserialize( istream & stream, is_struct auto & value, wire_type type );

template < scalar_encoder encoder >
static inline void deserialize_as( istream & stream, is_int_or_float auto & value, wire_type type );

static inline void deserialize( istream & stream, std::vector< std::byte > & value, wire_type type );

template < typename T >
static inline void deserialize( istream & stream, std::vector< T > & value, wire_type type );

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::vector< T > & value, wire_type type );

template < scalar_encoder encoder, typename keyT, typename valueT >
static inline void deserialize_as( istream & stream, std::map< keyT, valueT > & value, wire_type type );

template < typename T >
static inline void deserialize( istream & stream, std::optional< T > & p_value, wire_type type );

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::optional< T > & p_value, wire_type type );

template < typename T >
static inline void deserialize( istream & stream, std::unique_ptr< T > & value, wire_type type );

template < scalar_encoder encoder >
static inline void deserialize_as( istream & stream, is_int_or_float auto & value, wire_type type )
{
    using T = std::remove_cvref_t< decltype( value ) >;

    if constexpr( type1( encoder ) == scalar_encoder::svarint )
    {
        if constexpr( !is_packed( encoder ) )
        {
            check_wire_type( type, wire_type::varint );
        }
        auto tmp = read_varint< std::make_unsigned_t< T > >( stream );
        value    = T( ( tmp >> 1 ) ^ ( ~( tmp & 1 ) + 1 ) );
    }
    else if constexpr( type1( encoder ) == scalar_encoder::varint )
    {
        if constexpr( !is_packed( encoder ) )
        {
            check_wire_type( type, wire_type::varint );
        }
        value = read_varint< T >( stream );
    }
    else if constexpr( type1( encoder ) == scalar_encoder::i32 )
    {
        if constexpr( !is_packed( encoder ) )
        {
            check_wire_type( type, wire_type::fixed32 );
        }
        static_assert( sizeof( value ) == sizeof( uint32_t ) );
        stream.read( &value, sizeof( value ) );
    }
    else if constexpr( type1( encoder ) == scalar_encoder::i64 )
    {
        if constexpr( !is_packed( encoder ) )
        {
            check_wire_type( type, wire_type::fixed64 );
        }
        static_assert( sizeof( value ) == sizeof( uint64_t ) );
        stream.read( &value, sizeof( value ) );
    }
}

static inline void deserialize( istream & stream, is_enum auto & value, wire_type type )
{
    using T        = std::remove_cvref_t< decltype( value ) >;
    using int_type = std::underlying_type_t< T >;

    check_wire_type( type, wire_type::varint );
    value = T( read_varint< int_type >( stream ) );
}

template < typename T >
static inline void deserialize( istream & stream, std::optional< T > & p_value, wire_type type )
{
    auto & value = p_value.emplace( T( ) );
    deserialize( stream, value, type );
}

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::optional< T > & p_value, wire_type type )
{
    auto & value = p_value.emplace( T( ) );
    deserialize_as< encoder >( stream, value, type );
}

static inline void deserialize( istream & stream, std::string_view & value, wire_type type )
{
    check_wire_type( type, wire_type::length_delimited );
    value = std::string_view( static_cast< const char * >( stream.data( ) ), stream.size( ) );
    stream.read( nullptr, stream.size( ) );
}

static inline void deserialize( istream & stream, std::string & value, wire_type type )
{
    check_wire_type( type, wire_type::length_delimited );
    value.resize( stream.size( ) );
    stream.read( value.data( ), stream.size( ) );
}

template < typename T >
static inline void deserialize( istream & stream, std::unique_ptr< T > & value, wire_type type )
{
    value = std::make_unique< T >( );
    deserialize( stream, *value, type );
}

static inline void deserialize( istream & stream, std::vector< std::byte > & value, wire_type type )
{
    check_wire_type( type, wire_type::length_delimited );
    value.resize( stream.size( ) );
    stream.read( value.data( ), stream.size( ) );
}

template < typename T >
static inline void deserialize( istream & stream, std::vector< T > & value, wire_type type )
{
    deserialize( stream, value.emplace_back( ), type );
}

template < scalar_encoder encoder, typename T >
static inline void deserialize_packed_as( istream & stream, std::vector< T > & value, wire_type type )
{
    while( !stream.empty( ) )
    {
        if constexpr( std::is_same_v< T, bool > )
        {
            value.emplace_back( read_varint< bool >( stream ) );
        }
        else
        {
            deserialize_as< encoder >( stream, value.emplace_back( ), type );
        }
    }
}

template < scalar_encoder encoder, typename T >
static inline void deserialize_as( istream & stream, std::vector< T > & value, wire_type type )
{
    if constexpr( is_packed( encoder ) )
    {
        deserialize_packed_as< encoder >( stream, value, wire_type_from_scalar_encoder( encoder ) );
    }
    else
    {
        if constexpr( std::is_same_v< T, bool > )
        {
            value.emplace_back( read_varint< bool >( stream ) );
        }
        else
        {
            deserialize_as< encoder >( stream, value.emplace_back( ), type );
        }
    }
}

template < scalar_encoder encoder, typename keyT, typename valueT >
static inline void deserialize_as( istream & stream, std::map< keyT, valueT > & value, wire_type type )
{
    const auto key_encoder   = type1( encoder );
    const auto value_encoder = type2( encoder );

    check_wire_type( type, wire_type::length_delimited );

    auto pair          = std::pair< keyT, valueT >( );
    auto key_defined   = false;
    auto value_defined = false;
    while( !stream.empty( ) )
    {
        const auto tag        = read_varint< uint32_t >( stream );
        const auto field      = field_from_tag( tag );
        const auto field_type = wire_type_from_tag( tag );

        switch( field )
        {
        case 1:
            if constexpr( std::is_integral_v< keyT > )
            {
                deserialize_as< key_encoder >( stream, pair.first, field_type );
            }
            else
            {
                if( field_type == wire_type::length_delimited )
                {
                    const auto size = read_varint< uint32_t >( stream );
                    auto substream  = stream.sub_stream( size );
                    deserialize( substream, pair.first, field_type );
                }
                else
                {
                    deserialize( stream, pair.first, field_type );
                }
            }
            key_defined = true;
            break;
        case 2:
            if constexpr( is_int_or_float< valueT > )
            {
                deserialize_as< value_encoder >( stream, pair.second, field_type );
            }
            else
            {
                if( field_type == wire_type::length_delimited )
                {
                    const auto size = read_varint< uint32_t >( stream );
                    auto substream  = stream.sub_stream( size );
                    deserialize( substream, pair.second, field_type );
                }
                else
                {
                    deserialize( stream, pair.second, field_type );
                }
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

template < size_t ordinal, typename T >
static inline void deserialize_variant( istream & stream, T & variant, wire_type type )
{
    deserialize( stream, variant.template emplace< ordinal >( ), type );
}

template < size_t ordinal, scalar_encoder encoder, typename T >
static inline void deserialize_variant_as( istream & stream, T & variant, wire_type type )
{
    deserialize_as< encoder >( stream, variant.template emplace< ordinal >( ), type );
}

static inline void deserialize( istream & stream, is_struct auto & value, wire_type type )
{
    check_wire_type( type, wire_type::length_delimited );

    while( !stream.empty( ) )
    {
        const auto tag        = read_varint< uint32_t >( stream );
        const auto field_type = wire_type_from_tag( tag );

        if( field_type == wire_type::length_delimited )
        {
            const auto size = read_varint< uint32_t >( stream );
            auto substream  = stream.sub_stream( size );
            deserialize_value( substream, value, tag );
        }
        else
        {
            deserialize_value( stream, value, tag );
        }
    }
}

inline void istream::deserialize( auto & value, uint32_t tag )
{
    detail::deserialize( *this, value, wire_type_from_tag( tag ) );
}

inline void istream::skip( uint32_t tag )
{
    switch( wire_type_from_tag( tag ) )
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
inline void istream::deserialize_as( auto & value, uint32_t tag )
{
    detail::deserialize_as< encoder >( *this, value, wire_type_from_tag( tag ) );
}

template < size_t ordinal, typename T >
inline void istream::deserialize_variant( T & variant, uint32_t tag )
{
    detail::deserialize_variant< ordinal >( *this, variant, wire_type_from_tag( tag ) );
}

template < size_t ordinal, scalar_encoder encoder, typename T >
inline void istream::deserialize_variant_as( T & variant, uint32_t tag )
{
    detail::deserialize_variant_as< ordinal, encoder >( *this, variant, wire_type_from_tag( tag ) );
}

template < typename Result >
static inline auto deserialize( std::string_view protobuf ) -> Result
{
    auto stream = detail::istream( protobuf.data( ), protobuf.size( ) );
    auto result = Result{ };
    detail::deserialize( stream, result, detail::wire_type::length_delimited );
    return result;
}

static inline void deserialize( auto & value, std::string_view string )
{
    using T = std::remove_cvref_t< decltype( value ) >;
    static_assert( is_struct< T > );
    auto stream = detail::istream( string.data( ), string.size( ) );
    detail::deserialize( stream, value, detail::wire_type::length_delimited );
}

}// namespace sds::pb::detail
