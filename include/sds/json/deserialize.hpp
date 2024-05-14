/***************************************************************************\
* Name        : deserialize library for json                                *
* Description : all json deserialization functions                          *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "../istream.h"
#include "base64.h"
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
#include <type_traits>
#include <variant>
#include <vector>

namespace sds::json::detail
{
static const auto escape = '\\';

/**
 * @brief helper for std::variant visit
 *        https://en.cppreference.com/w/cpp/utility/variant/visit
 *
 */
template < class... Ts >
struct overloaded : Ts...
{
    using Ts::operator( )...;
};
// explicit deduction guide (not needed as of C++20)
template < class... Ts >
overloaded( Ts... ) -> overloaded< Ts... >;

/**
 * @brief djb2_hash for strings
 *        http://www.cse.yorku.ca/~oz/hash.html
 *
 * @param str
 * @return uint32_t
 */
static constexpr inline auto djb2_hash( std::string_view str ) noexcept -> uint32_t
{
    uint32_t hash = 5381U;

    for( auto c : str )
    {
        hash = ( ( hash << 5U ) + hash ) + uint8_t( c ); /* hash * 33 + c */
    }

    return hash;
}

struct istream : public sds::istream
{
private:
    std::string_view m_current_key;

public:
    istream( std::string_view content ) noexcept
        : sds::istream( content.data( ) )
    {
    }

    auto deserialize( std::string_view key, auto & value ) -> bool;
    template < size_t ordinal, typename T >
    auto deserialize_variant( std::string_view key, T & variant ) -> bool;
    auto deserialize_string( ) -> std::string_view;
    auto deserialize_int( ) -> int32_t;
    auto deserialize_string_or_int( ) -> std::variant< std::string_view, int32_t >;
    void deserialize_key( );
    [[nodiscard]] auto current_key( ) const -> std::string_view;
};

static inline auto is_escape( char c ) -> bool
{
    static constexpr std::string_view escape_chars = R"("\/bfnrt)";
    return escape_chars.find( c ) != std::string_view::npos;
}

template < class T >
concept is_enum_only = ::std::is_enum_v< T >;

static inline void deserialize( istream & stream, is_enum_only auto & value )
{
    deserialize_value( stream, value );
}

static inline void deserialize( istream & stream, std::string_view & value )
{
    if( stream.current_char( ) != '"' )
    {
        throw std::runtime_error( "expecting '\"'" );
    }

    stream.consume_current_char( false );
    const auto * start = stream.begin( );
    auto current       = stream.current_char( );
    while( ( current != 0 ) && current != '"' )
    {
        if( current == escape )
        {
            stream.consume_current_char( false );
            current = stream.current_char( );
            if( is_escape( current ) )
            {
                stream.consume_current_char( false );
                current = stream.current_char( );
                continue;
            }

            throw std::runtime_error( "invalid esc sequence" );
        }
        stream.consume_current_char( false );
        current = stream.current_char( );
    }

    if( current != '"' )
    {
        throw std::runtime_error( "expecting '\"'" );
    }

    value = std::string_view( start, static_cast< size_t >( stream.begin( ) - start ) );
    stream.consume_current_char( true );
}

static inline auto unescape( std::string_view value, std::string & result ) -> std::string
{
    result.clear( );
    result.reserve( std::size( value ) );
    for( auto esc_offset = value.find( escape ); esc_offset != std::string_view::npos; esc_offset = value.find( escape ) )
    {
        result += value.substr( 0, esc_offset );
        value.remove_prefix( esc_offset + 1 );
        switch( value.front( ) )
        {
        case '"':
            result += '"';
            break;
        case '\\':
            result += '\\';
            break;
        case '/':
            result += '/';
            break;
        case 'b':
            result += '\b';
            break;
        case 'f':
            result += '\f';
            break;
        case 'n':
            result += '\n';
            break;
        case 'r':
            result += '\r';
            break;
        case 't':
            result += '\t';
            break;
        default:
            result += ' ';
        }
        value.remove_prefix( 1 );
    }
    result += value;
    return result;
}

static inline void deserialize( istream & stream, std::string & value )
{
    auto view = std::string_view{ };
    deserialize( stream, view );
    unescape( view, value );
}

static inline void deserialize_number( istream & stream, auto & value )
{
    auto result = std::from_chars( stream.begin( ), stream.end( ), value );
    if( result.ec != std::errc{ } )
    {
        throw std::runtime_error( "invalid number" );
    }
    stream.skip_to( result.ptr );
}

static inline void deserialize( istream & stream, bool & value )
{
    if( stream.consume( "true" ) )
    {
        value = true;
    }
    else if( stream.consume( "false" ) )
    {
        value = false;
    }
    else
    {
        throw std::runtime_error( "expecting 'true' or 'false'" );
    }
}

static inline void deserialize( istream & stream, std::floating_point auto & value )
{
    deserialize_number( stream, value );
}

static inline void deserialize( istream & stream, std::integral auto & value )
{
    deserialize_number( stream, value );
}

static inline void deserialize( istream & stream, auto & value );

template < typename keyT, typename valueT >
static inline void deserialize( istream & stream, std::map< keyT, valueT > & value );

template < typename T >
static inline void deserialize( istream & stream, std::optional< T > & value );

template < typename T >
static inline void deserialize( istream & stream, std::vector< T > & value )
{
    if( stream.consume( "null" ) )
    {
        value.clear( );
        return;
    }

    if constexpr( std::is_same_v< T, std::byte > )
    {
        auto encoded = stream.deserialize_string( );
        if( !base64_decode( value, encoded ) )
        {
            throw std::runtime_error( "invalid base64" );
        }
    }
    else
    {
        if( !stream.consume( '[' ) )
        {
            throw std::runtime_error( "expecting '['" );
        }

        if( stream.consume( ']' ) )
        {
            return;
        }

        do
        {
            if constexpr( std::is_same_v< T, bool > )
            {
                auto b = false;
                deserialize( stream, b );
                value.push_back( b );
            }
            else
            {
                deserialize( stream, value.emplace_back( ) );
            }
        } while( stream.consume( ',' ) );

        if( !stream.consume( ']' ) )
        {
            throw std::runtime_error( "expecting ']'" );
        }
    }
}

template < typename T >
auto deserialize_map_key( istream & stream ) -> T
{
    if constexpr( std::is_same_v< T, std::string > )
    {
        return std::string( stream.deserialize_string( ) );
    }
    else
    {
        auto str_key_map = stream.deserialize_string( );
        auto map_key     = T( );
        auto key_stream  = istream( str_key_map );
        deserialize( key_stream, map_key );
        return map_key;
    }
}

template < typename keyT, typename valueT >
static inline void deserialize( istream & stream, std::map< keyT, valueT > & value )
{
    if( stream.consume( "null" ) )
    {
        value.clear( );
        return;
    }
    if( !stream.consume( '{' ) )
    {
        throw std::runtime_error( "expecting '{'" );
    }

    if( stream.consume( '}' ) )
    {
        return;
    }

    do
    {
        const auto map_key = deserialize_map_key< keyT >( stream );
        if( !stream.consume( ':' ) )
        {
            throw std::runtime_error( "expecting ':'" );
        }
        auto map_value = valueT( );
        deserialize( stream, map_value );
        value.emplace( std::move( map_key ), std::move( map_value ) );
    } while( stream.consume( ',' ) );

    if( !stream.consume( '}' ) )
    {
        throw std::runtime_error( "expecting '}'" );
    }
}

template < typename T >
static inline void deserialize( istream & stream, std::optional< T > & value )
{
    if( stream.consume( "null" ) )
    {
        value.reset( );
        return;
    }

    if( value.has_value( ) )
    {
        deserialize( stream, *value );
    }
    else
    {
        deserialize( stream, value.emplace( T{ } ) );
    }
}

template < typename T >
static inline void deserialize( istream & stream, std::unique_ptr< T > & value )
{
    if( stream.consume( "null" ) )
    {
        value.reset( );
        return;
    }

    if( value )
    {
        deserialize( stream, *value );
    }
    else
    {
        value = std::make_unique< T >( );
        deserialize( stream, *value );
    }
}

static inline void ignore_value( istream & stream );
static inline void ignore_string( istream & stream );
static inline void ignore_key_and_value( istream & stream )
{
    ignore_string( stream );
    if( !stream.consume( ':' ) )
    {
        throw std::runtime_error( "expecting ':'" );
    }
    ignore_value( stream );
}

static inline void ignore_object( istream & stream )
{
    if( !stream.consume( '{' ) )
    {
        throw std::runtime_error( "expecting '{'" );
    }

    if( stream.consume( '}' ) )
    {
        return;
    }

    do
    {
        ignore_key_and_value( stream );
    } while( stream.consume( ',' ) );

    if( !stream.consume( '}' ) )
    {
        throw std::runtime_error( "expecting '}'" );
    }
}

static inline void ignore_array( istream & stream )
{
    if( !stream.consume( '[' ) )
    {
        throw std::runtime_error( "expecting '['" );
    }

    if( stream.consume( ']' ) )
    {
        return;
    }

    do
    {
        ignore_value( stream );
    } while( stream.consume( ',' ) );

    if( !stream.consume( ']' ) )
    {
        throw std::runtime_error( "expecting ']" );
    }
}

static inline void ignore_string( istream & stream )
{
    auto value = std::string_view{ };
    deserialize( stream, value );
}

static inline void ignore_number( istream & stream )
{
    auto value = double{ };
    deserialize( stream, value );
}

static inline void ignore_bool( istream & stream )
{
    if( !stream.consume( "true" ) &&
        !stream.consume( "false" ) )
    {
        throw std::runtime_error( "expecting 'true' or 'false'" );
    }
}

static inline void ignore_null( istream & stream )
{
    if( !stream.consume( "null" ) )
    {
        throw std::runtime_error( "expecting 'null'" );
    }
}

static inline void ignore_value( istream & stream )
{
    switch( stream.current_char( ) )
    {
    case '{':
        return ignore_object( stream );
    case '[':
        return ignore_array( stream );
    case '"':
        return ignore_string( stream );
    case 'n':
        return ignore_null( stream );
    case 't':
    case 'f':
        return ignore_bool( stream );
    default:
        return ignore_number( stream );
    }
}

[[nodiscard]] static inline auto deserialize( istream & stream, std::string_view parsed_key, std::string_view key, auto & value ) -> bool
{
    if( parsed_key != key )
    {
        return false;
    }

    deserialize( stream, value );
    return true;
}

template < size_t ordinal, typename T >
[[nodiscard]] static inline auto deserialize_variant( istream & stream, std::string_view parsed_key, std::string_view key, T & variant ) -> bool
{
    if( parsed_key != key )
    {
        return false;
    }

    deserialize( stream, variant.template emplace< ordinal >( ) );
    return true;
}

static inline void deserialize( istream & stream, auto & value )
{
    if( !stream.consume( '{' ) )
    {
        throw std::runtime_error( "expecting '{'" );
    }

    if( stream.consume( '}' ) )
    {
        return;
    }

    for( ;; )
    {
        stream.deserialize_key( );

        //
        //- deserialize_value is generated by the sds-proto-compiler
        //
        if( !deserialize_value( stream, value ) )
        {
            ignore_value( stream );
        }

        if( stream.consume( ',' ) )
        {
            continue;
        }

        if( stream.consume( '}' ) )
        {
            return;
        }

        throw std::runtime_error( "expecting '}' or ','" );
    }
}

inline void istream::deserialize_key( )
{
    detail::deserialize( *this, m_current_key );
    if( !consume( ':' ) )
    {
        throw std::runtime_error( "expecting ':'" );
    }
}
inline auto istream::current_key( ) const -> std::string_view
{
    return m_current_key;
}

inline auto istream::deserialize( std::string_view key, auto & value ) -> bool
{
    return detail::deserialize( *this, m_current_key, key, value );
}

template < size_t ordinal, typename T >
inline auto istream::deserialize_variant( std::string_view key, T & variant ) -> bool
{
    return detail::deserialize_variant< ordinal >( *this, m_current_key, key, variant );
}

inline auto istream::deserialize_string_or_int( ) -> std::variant< std::string_view, int32_t >
{
    if( current_char( ) == '"' )
    {
        return deserialize_string( );
    }
    return deserialize_int( );
}

inline auto istream::deserialize_int( ) -> int32_t
{
    auto result = int32_t{ };
    detail::deserialize( *this, result );
    return result;
}

inline auto istream::deserialize_string( ) -> std::string_view
{
    auto result = std::string_view{ };
    detail::deserialize( *this, result );
    return result;
}

static inline void deserialize( std::string_view string, auto & value )
{
    auto stream = detail::istream( string );
    detail::deserialize( stream, value );
}

template < typename Result >
static inline auto deserialize( std::string_view json ) -> Result
{
    auto stream = detail::istream( json );
    auto result = Result{ };
    detail::deserialize( stream, result );
    return result;
}

}// namespace sds::json::detail
