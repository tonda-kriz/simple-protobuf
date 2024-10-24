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

#include "../bits.h"
#include "../concepts.h"
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
#include <spb/io/buffer-io.hpp>
#include <spb/io/io.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

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

static constexpr inline auto fnv1a_hash( std::string_view str ) noexcept -> uint64_t
{
    uint64_t hash        = 14695981039346656037ULL;
    const uint64_t prime = 1099511628211ULL;

    for( auto c : str )
    {
        hash *= prime;
        hash ^= c;
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

    void deserialize( auto & value );
    template < size_t ordinal, typename T >
    void deserialize_variant( T & variant );
    template < typename T >
    [[nodiscard]] auto deserialize_bitfield( uint32_t bits ) -> T;
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
    void skip_value( );
};

static inline auto is_escape( char c ) -> bool
{
    static constexpr std::string_view escape_chars = R"("\/bfnrt)";
    return escape_chars.find( c ) != std::string_view::npos;
}

static inline void deserialize( istream & stream, spb::detail::proto_enum auto & value )
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

static inline void unescape( spb::detail::proto_field_string auto & str )
{
    auto value = std::string_view( str.data( ), str.size( ) );

    if( value.find( escape ) == std::string_view::npos )
    {
        return;
    }

    auto result = str;
    result.clear( );
    for( auto esc_offset = value.find( escape ); esc_offset != std::string_view::npos; esc_offset = value.find( escape ) )
    {
        result.append( value.data( ), esc_offset );
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
    result.append( value.data( ), value.size( ) );
    str = result;
}

static inline void deserialize( istream & stream, spb::detail::proto_field_string auto & value )
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
                value.append( view.data( ), length );
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

static inline void deserialize( istream & stream, spb::detail::proto_field_int_or_float auto & value )
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
    auto result = spb_std_emu::from_chars( view.data( ), view.data( ) + view.size( ), value );
    if( result.ec != std::errc{ } )
    {
        throw std::runtime_error( "invalid number" );
    }
    stream.skip( result.ptr - view.data( ) );
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

static inline void deserialize( istream & stream, auto & value );

template < typename keyT, typename valueT >
static inline void deserialize( istream & stream, std::map< keyT, valueT > & value );

static inline void deserialize( istream & stream, spb::detail::proto_label_optional auto & value );

template < spb::detail::proto_label_repeated C >
static inline void deserialize( istream & stream, C & value )
{
    if( stream.consume( "null"sv ) )
    {
        value.clear( );
        return;
    }

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
        if constexpr( std::is_same_v< typename C::value_type, bool > )
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

static inline void deserialize( istream & stream, spb::detail::proto_field_bytes auto & value )
{
    if( stream.consume( "null"sv ) )
    {
        value.clear( );
        return;
    }

    base64_decode_string( value, stream );
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

static inline void deserialize( istream & stream, spb::detail::proto_label_optional auto & p_value )
{
    if( stream.consume( "null"sv ) )
    {
        p_value.reset( );
        return;
    }

    if( p_value.has_value( ) )
    {
        deserialize( stream, *p_value );
    }
    else
    {
        deserialize( stream, p_value.emplace( typename std::decay_t< decltype( p_value ) >::value_type( ) ) );
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

template < typename T >
inline auto deserialize_bitfield( istream & stream, uint32_t bits ) -> T
{
    auto value = T( );
    deserialize( stream, value );
    spb::detail::check_if_value_fit_in_bits( value, bits );
    return value;
}

template < size_t ordinal, typename T >
static inline void deserialize_variant( istream & stream, T & variant )
{
    deserialize( stream, variant.template emplace< ordinal >( ) );
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
        deserialize_value( stream, value );

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

inline void istream::deserialize( auto & value )
{
    return detail::deserialize( *this, value );
}

template < size_t ordinal, typename T >
inline void istream::deserialize_variant( T & variant )
{
    return detail::deserialize_variant< ordinal >( *this, variant );
}

template < typename T >
inline auto istream::deserialize_bitfield( uint32_t bits ) -> T
{
    return detail::deserialize_bitfield< T >( *this, bits );
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

inline void istream::skip_value( )
{
    return detail::ignore_value( *this );
}

static inline void deserialize( auto & value, spb::io::reader reader )
{
    auto stream = detail::istream( reader );
    return detail::deserialize( stream, value );
}

}// namespace spb::json::detail
