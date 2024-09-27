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

#include "../from_chars.h"
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
#include <spb/io/buffer-io.hpp>
#include <spb/io/io.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace spb::json::detail
{
using namespace std::literals;

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

struct istream
{
private:
    spb::io::buffered_reader reader;

    //- current char
    int m_current = -1;

    std::string_view m_current_key;

    /**
     * @brief gets the next char from the stream
     *
     * @param skip_white_space if true, skip white spaces
     */
    void update_current( bool skip_white_space )
    {
        for( ;; )
        {
            auto view = reader.view( 1 );
            if( view.empty( ) )
            {
                m_current = 0;
                return;
            }
            m_current = view[ 0 ];
            if( !skip_white_space )
            {
                return;
            }
            size_t spaces = 0;
            for( auto c : view )
            {
                if( !isspace( c ) )
                {
                    m_current = c;
                    reader.skip( spaces );
                    return;
                }
                spaces += 1;
            }
            reader.skip( spaces );
        }
    }

    [[nodiscard]] auto eof( ) -> bool
    {
        return current_char( ) == 0;
    }

public:
    istream( spb::io::reader reader )
        : reader( reader )
    {
    }

    [[nodiscard]] auto deserialize( std::string_view key, auto & value ) -> bool;
    template < size_t ordinal, typename T >
    [[nodiscard]] auto deserialize_variant( std::string_view key, T & variant ) -> bool;
    [[nodiscard]] auto deserialize_int( ) -> int32_t;
    [[nodiscard]] auto deserialize_string_or_int( size_t min_size, size_t max_size ) -> std::variant< std::string_view, int32_t >;
    [[nodiscard]] auto deserialize_key( size_t min_size, size_t max_size ) -> std::string_view;
    [[nodiscard]] auto current_key( ) const -> std::string_view;

    [[nodiscard]] auto current_char( ) -> char
    {
        if( m_current < 0 )
        {
            update_current( true );
        }

        return m_current;
    }
    /**
     * @brief consumes `current char` if its equal to c
     *
     * @param c consumed char
     * @return true if char was consumed
     */
    [[nodiscard]] auto consume( char c ) -> bool
    {
        if( current_char( ) == c )
        {
            consume_current_char( true );
            return true;
        }
        return false;
    }

    /**
     * @brief consumes an `token`
     *
     * @param token consumed `token` (whole word)
     * @return true if `token` was consumed
     */
    [[nodiscard]] auto consume( std::string_view token ) -> bool
    {
        if( current_char( ) != token[ 0 ] )
        {
            return false;
        }

        if( !reader.view( token.size( ) ).starts_with( token ) )
        {
            return false;
        }
        auto token_view = reader.view( token.size( ) + 1 ).substr( 0, token.size( ) + 1 );
        if( token_view.size( ) == token.size( ) ||
            isspace( token_view.back( ) ) ||
            ( !isalnum( token_view.back( ) ) && token_view.back( ) != '_' ) )
        {
            reader.skip( token.size( ) );
            update_current( true );
            return true;
        }
        return false;
    }

    [[nodiscard]] auto view( size_t size ) -> std::string_view
    {
        auto result = reader.view( size );
        if( result.empty( ) )
        {
            throw std::runtime_error( "unexpected end of stream" );
        }
        return result;
    }

    void consume_current_char( bool skip_white_space ) noexcept
    {
        reader.skip( 1 );
        update_current( skip_white_space );
    }

    void skip( size_t size )
    {
        reader.skip( size );
        m_current = -1;
    }
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

static inline void ignore_string( istream & stream )
{
    if( stream.current_char( ) != '"' )
    {
        throw std::runtime_error( "expecting '\"'" );
    }

    auto last = escape;
    for( ;; )
    {
        auto view   = stream.view( UINT32_MAX );
        auto length = 0U;
        for( auto current : view )
        {
            length += 1;
            if( current == '"' && last != escape )
            {
                stream.skip( length );
                return;
            }
            //- handle \\"
            last = current != escape || last != escape ? current : ' ';
        }
        stream.skip( view.size( ) );
    }
}

static inline auto deserialize_string_view( istream & stream, size_t min_size, size_t max_size ) -> std::string_view
{
    if( stream.current_char( ) != '"' )
    {
        throw std::runtime_error( "expecting '\"'" );
    }

    //- +2 for '"'
    auto view   = stream.view( max_size + 2 );
    auto last   = escape;
    auto length = size_t( 0 );
    for( auto current : view )
    {
        length += 1;

        if( current == '"' && last != escape )
        {
            stream.skip( length );

            if( ( length - 2 ) >= min_size &&
                ( length - 2 ) <= max_size )
            {
                return view.substr( 1, length - 2 );
            }

            return { };
        }
        //- handle \\"
        last = current != escape || last != escape ? current : ' ';
    }

    ignore_string( stream );
    return { };
}

static inline void unescape( std::string & str )
{
    if( str.find( escape ) == std::string::npos )
    {
        return;
    }

    auto value  = std::string_view( str );
    auto result = std::string( );
    result.reserve( value.size( ) );
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
            throw std::runtime_error( "invalid escape sequence" );
        }
        value.remove_prefix( 1 );
    }
    result += value;
    str.swap( result );
}

static inline void deserialize( istream & stream, std::string & value )
{
    if( stream.current_char( ) != '"' )
    {
        throw std::runtime_error( "expecting '\"'" );
    }

    stream.consume_current_char( false );
    value.clear( );

    auto last = '"';
    for( ;; )
    {
        auto view   = stream.view( UINT32_MAX );
        auto length = size_t( 0 );
        for( auto current : view )
        {
            if( current == '"' && last != escape )
            {
                value.append( view.substr( 0, length ) );
                unescape( value );
                stream.skip( length + 1 );
                return;
            }
            //- handle \\"
            last = current != escape || last != escape ? current : ' ';
            length += 1;
        }
        stream.skip( view.size( ) );
    }
}

static inline void deserialize_number( istream & stream, auto & value )
{
    if( stream.current_char( ) == '"' ) [[unlikely]]
    {
        //- https://protobuf.dev/programming-guides/proto2/#json
        //- number can be a string
        auto view   = deserialize_string_view( stream, 1, UINT32_MAX );
        auto result = spb_std_emu::from_chars( view.data( ), view.data( ) + view.size( ), value );
        if( result.ec != std::errc{ } )
        {
            throw std::runtime_error( "invalid number" );
        }
        return;
    }
    auto view   = stream.view( UINT32_MAX );
    auto result = spb_std_emu::from_chars( view.begin( ), view.end( ), value );
    if( result.ec != std::errc{ } )
    {
        throw std::runtime_error( "invalid number" );
    }
    stream.skip( result.ptr - view.begin( ) );
}

static inline void deserialize( istream & stream, bool & value )
{
    if( stream.consume( "true"sv ) )
    {
        value = true;
    }
    else if( stream.consume( "false"sv ) )
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
    if( stream.consume( "null"sv ) )
    {
        value.clear( );
        return;
    }

    if constexpr( std::is_same_v< T, std::byte > )
    {
        auto encoded = std::string( );
        deserialize( stream, encoded );
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
void deserialize_map_key( istream & stream, T & map_key )
{
    if constexpr( std::is_same_v< T, std::string > )
    {
        return deserialize( stream, map_key );
    }
    auto str_key_map = deserialize_string_view( stream, 1, UINT32_MAX );
    auto reader      = [ ptr = str_key_map.data( ), end = str_key_map.data( ) + str_key_map.size( ) ]( void * data, size_t size ) mutable -> size_t
    {
        size_t bytes_left = end - ptr;
        size              = std::min( size, bytes_left );
        memcpy( data, ptr, size );
        ptr += size;
        return size;
    };
    auto key_stream = istream( reader );
    deserialize( key_stream, map_key );
}

template < typename keyT, typename valueT >
static inline void deserialize( istream & stream, std::map< keyT, valueT > & value )
{
    if( stream.consume( "null"sv ) )
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
        auto map_key = keyT( );
        deserialize_map_key( stream, map_key );
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
    if( stream.consume( "null"sv ) )
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
    if( stream.consume( "null"sv ) )
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
    //- '{' was already checked by caller
    stream.consume_current_char( true );

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
    //- '[' was already checked by caller
    stream.consume_current_char( true );

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

static inline void ignore_number( istream & stream )
{
    auto value = double{ };
    deserialize( stream, value );
}

static inline void ignore_bool( istream & stream )
{
    auto value = bool{ };
    deserialize( stream, value );
}

static inline void ignore_null( istream & stream )
{
    if( !stream.consume( "null"sv ) )
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
        //
        //- deserialize_value is generated by the sprotoc
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

inline auto istream::deserialize_key( size_t min_size, size_t max_size ) -> std::string_view
{
    m_current_key = deserialize_string_view( *this, min_size, max_size );
    if( !consume( ':' ) )
    {
        throw std::runtime_error( "expecting ':'" );
    }
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

inline auto istream::deserialize_string_or_int( size_t min_size, size_t max_size ) -> std::variant< std::string_view, int32_t >
{
    if( current_char( ) == '"' )
    {
        return deserialize_string_view( *this, min_size, max_size );
    }
    return deserialize_int( );
}

inline auto istream::deserialize_int( ) -> int32_t
{
    auto result = int32_t{ };
    detail::deserialize( *this, result );
    return result;
}

static inline void deserialize( auto & value, spb::io::reader reader )
{
    auto stream = detail::istream( reader );
    detail::deserialize( stream, value );
}

}// namespace spb::json::detail
