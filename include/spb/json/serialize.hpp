/***************************************************************************\
* Name        : serialize library for json                                  *
* Description : all json serialization functions                            *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "../concepts.h"
#include "base64.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <span>
#include <spb/io/io.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <type_traits>
#include <vector>

namespace spb::json::detail
{
struct ostream
{
private:
    size_t bytes_written = 0;
    spb::io::writer on_write;

public:
    //- flag if put ',' before value
    bool put_comma = false;

    /**
     * @brief Construct a new ostream object
     *
     * @param writer if null, stream will skip all writes but will still count number of written chars
     */
    explicit ostream( spb::io::writer writer ) noexcept
        : on_write( writer )
    {
    }

    void write( char c ) noexcept
    {
        if( on_write )
        {
            on_write( &c, 1 );
        }

        bytes_written += 1;
    }

    void write( std::string_view str ) noexcept
    {
        if( on_write )
        {
            on_write( str.data( ), str.size( ) );
        }

        bytes_written += str.size( );
    }

    void write_escaped( std::string_view str ) noexcept
    {
        if( has_escape_chars( str ) )
        {
            using namespace std::literals;

            for( auto c : str )
            {
                switch( c )
                {
                case '"':
                    write( R"(\")"sv );
                    break;
                case '\\':
                    write( R"(\\)"sv );
                    break;
                case '/':
                    write( R"(\/)"sv );
                    break;
                case '\b':
                    write( R"(\b)"sv );
                    break;
                case '\f':
                    write( R"(\f)"sv );
                    break;
                case '\n':
                    write( R"(\n)"sv );
                    break;
                case '\r':
                    write( R"(\r)"sv );
                    break;
                case '\t':
                    write( R"(\t)"sv );
                    break;
                default:
                    write( c );
                }
            }
        }
        else
        {
            write( str );
        }
    }

    void serialize( std::string_view key, const auto & value );
    void serialize( std::string_view value );

    [[nodiscard]] auto size( ) const noexcept -> size_t
    {
        return bytes_written;
    }

private:
    static auto is_escape( char c ) -> bool
    {
        return c == '\\' || c == '"' || c == '/' || c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t';
    }

    static auto has_escape_chars( std::string_view str ) -> bool
    {
        return std::any_of( str.begin( ), str.end( ), is_escape );
    }
};

using namespace std::literals;

static inline void serialize_key( ostream & stream, std::string_view key )
{
    if( stream.put_comma )
    {
        stream.write( ',' );
    }
    stream.put_comma = true;

    if( !key.empty( ) )
    {
        stream.write( '"' );
        stream.write_escaped( key );
        stream.write( R"(":)"sv );
    }
}

static inline void serialize( ostream & stream, std::string_view key, const bool & value );
static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_field_int_or_float auto & value );
static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_message auto & value );
static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_enum auto & value );
static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_field_string auto & value );
static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_field_bytes auto & value );
static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_label_repeated auto & value );
template < typename keyT, typename valueT >
static inline void serialize( ostream & stream, std::string_view key, const std::map< keyT, valueT > & map );

static inline void serialize( ostream & stream, bool value );
static inline void serialize( ostream & stream, spb::detail::proto_field_int_or_float auto value );
static inline void serialize( ostream & stream, const spb::detail::proto_field_string auto & value );

static inline void serialize( ostream & stream, bool value )
{
    stream.write( value ? "true"sv : "false"sv );
}

static inline void serialize( ostream & stream, const std::string_view & value )
{
    stream.write( '"' );
    stream.write_escaped( value );
    stream.write( '"' );
}

static inline void serialize( ostream & stream, const spb::detail::proto_field_string auto & value )
{
    serialize( stream, std::string_view( value.data( ), value.size( ) ) );
}

static inline void serialize( ostream & stream, spb::detail::proto_field_int_or_float auto value )
{
    auto buffer = std::array< char, 32 >( );

    auto result = std::to_chars( buffer.data( ), buffer.data( ) + sizeof( buffer ), value );
    stream.write( std::string_view( buffer.data( ), static_cast< size_t >( result.ptr - buffer.data( ) ) ) );
}

static inline void serialize( ostream & stream, std::string_view key, const bool & value )
{
    serialize_key( stream, key );
    serialize( stream, value );
}

static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_field_int_or_float auto & value )
{
    serialize_key( stream, key );
    serialize( stream, value );
}

static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_field_string auto & value )
{
    if( !value.empty( ) )
    {
        serialize_key( stream, key );
        serialize( stream, value );
    }
}

static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_field_bytes auto & value )
{
    if( !value.empty( ) )
    {
        serialize_key( stream, key );
        stream.write( '"' );
        base64_encode( stream, value );
        stream.write( '"' );
    }
}
static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_label_repeated auto & value )
{
    if( value.empty( ) )
    {
        return;
    }

    serialize_key( stream, key );
    stream.write( '[' );
    stream.put_comma = false;
    for( const auto & v : value )
    {
        if constexpr( std::is_same_v< typename std::decay_t< decltype( value ) >::value_type, bool > )
        {
            serialize( stream, { }, bool( v ) );
        }
        else
        {
            serialize( stream, { }, v );
        }
    }
    stream.write( ']' );
    stream.put_comma = true;
}

static constexpr std::string_view no_name = { };

template < typename keyT, typename valueT >
static inline void serialize( ostream & stream, std::string_view key, const std::map< keyT, valueT > & map )
{
    if( map.empty( ) )
    {
        return;
    }
    serialize_key( stream, key );
    stream.write( '{' );
    stream.put_comma = false;
    for( auto & [ map_key, map_value ] : map )
    {
        if constexpr( std::is_same_v< keyT, std::string_view > || std::is_same_v< keyT, std::string > )
        {
            serialize_key( stream, map_key );
        }
        else
        {
            if( stream.put_comma )
            {
                stream.write( ',' );
            }

            stream.write( '"' );
            serialize( stream, map_key );
            stream.write( R"(":)"sv );
        }
        stream.put_comma = false;
        serialize( stream, no_name, map_value );
        stream.put_comma = true;
    }
    stream.write( '}' );
    stream.put_comma = true;
}

static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_label_optional auto & p_value )
{
    if( p_value.has_value( ) )
    {
        return serialize( stream, key, *p_value );
    }
}

template < typename T >
static inline void serialize( ostream & stream, std::string_view key, const std::unique_ptr< T > & p_value )
{
    if( p_value )
    {
        return serialize( stream, key, *p_value );
    }
}

static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_message auto & value )
{
    serialize_key( stream, key );
    stream.write( '{' );
    stream.put_comma = false;

    //
    //- serialize_value is generated by the spb-protoc
    //
    serialize_value( stream, value );
    stream.write( '}' );
    stream.put_comma = true;
}

static inline void serialize( ostream & stream, std::string_view key, const spb::detail::proto_enum auto & value )
{
    serialize_key( stream, key );

    //
    //- serialize_value is generated by the spb-protoc
    //
    serialize_value( stream, value );
}

static inline auto serialize( const auto & value, spb::io::writer on_write ) -> size_t
{
    auto stream = ostream( on_write );
    serialize( stream, no_name, value );
    return stream.size( );
}

void ostream::serialize( std::string_view key, const auto & value )
{
    detail::serialize( *this, key, value );
}

inline void ostream::serialize( std::string_view value )
{
    detail::serialize( *this, value );
}

}// namespace spb::json::detail
