/***************************************************************************\
* Name        : ast options resolver                                        *
* Description : map proto_options to spb_options                            *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#include "ast-options.h"
#include "dumper/header.h"
#include <initializer_list>
#include <span>
#include <spb/to_from_chars.h>

namespace
{
using namespace std::literals;

auto option_value( std::span< const std::string_view > path, const proto_option & option )
    -> const proto_option *;

auto option_value( std::span< const std::string_view > path, const proto_options & options )
    -> const proto_option *
{
    if( path.empty( ) )
        return nullptr;

    auto p_option = options.find( path.front( ) );
    if( p_option == options.end( ) )
        return nullptr;

    if( auto result = option_value( path.subspan( 1 ), p_option->second ); result )
        return result;

    return nullptr;
}

auto option_value( std::span< const std::string_view > path, const proto_option & option )
    -> const proto_option *
{
    if( path.empty( ) )
        return &option;

    for( const auto & options : option.options )
    {
        if( auto result = option_value( path, options ); result )
            return result;
    }

    return nullptr;
}

auto option_value( std::initializer_list< const std::string_view > path,
                   const proto_options & options ) -> std::string_view
{
    auto option = option_value( std::span( path.begin( ), path.size( ) ), options );
    return option && !option->value.empty( ) ? *option->value.begin( ) : std::string_view{ };
}

auto option_value_bool( const proto_file & file,
                        std::initializer_list< const std::string_view > full_name,
                        const proto_options & options ) -> std::optional< bool >
{
    auto option = option_value( full_name, options );
    if( option.empty( ) )
        return { };

    if( option == "true" )
        return true;

    if( option == "false" )
        return false;

    throw_parse_error( file, option, "Expecting bool (\"true\", \"false\")" );
}

template < typename T >
auto option_value_int( const proto_file & file,
                       std::initializer_list< const std::string_view > full_name,
                       const proto_options & options ) -> std::optional< T >
{
    auto option = option_value( full_name, options );
    if( option.empty( ) )
        return { };

    auto value  = T{ };
    auto result = spb_std_emu::from_chars( option.begin( ), option.end( ), value );
    if( result.ec == std::errc{ } ) [[likely]]
        return value;

    throw_parse_error( file, option, "Expecting integer (1, 3, ...)" );
}

void convert_gpb_options( const proto_file & file, spb_options & options_out,
                          const proto_options & options_in )
{
    if( auto value = option_value( { "default" }, options_in ); !value.empty( ) )
        options_out.default_ = value;

    if( auto value = option_value_bool( file, { "deprecated" }, options_in ); value.has_value( ) )
        options_out.deprecated = *value;

    if( auto value = option_value_bool( file, { "packed" }, options_in ); value.has_value( ) )
        options_out.packed = *value;

    if( auto value = option_value( { "json_name" }, options_in ); !value.empty( ) )
        options_out.json_name = value;
}

void convert_spb_options( const proto_file & file, spb_options & options_out,
                          const proto_options & options_in )
{
    if( auto value = option_value( { "field", "type" }, options_in ); !value.empty( ) )
        options_out.type = value;

    if( auto value = option_value( { "(spb)", "type" }, options_in ); !value.empty( ) )
        options_out.type = value;

    if( auto value = option_value( { "enum", "type" }, options_in ); !value.empty( ) )
        options_out.enum_ = value;

    if( auto value = option_value( { "(spb)", "enum" }, options_in ); !value.empty( ) )
        options_out.enum_ = value;

    if( auto value = option_value( { "optional", "type" }, options_in ); !value.empty( ) )
        options_out.optional = value;

    if( auto value = option_value( { "(spb)", "optional" }, options_in ); !value.empty( ) )
        options_out.optional = value;

    if( auto value = option_value( { "repeated", "type" }, options_in ); !value.empty( ) )
        options_out.repeated = value;

    if( auto value = option_value( { "(spb)", "repeated" }, options_in ); !value.empty( ) )
        options_out.repeated = value;

    if( auto value = option_value( { "pointer", "type" }, options_in ); !value.empty( ) )
        options_out.pointer = value;

    if( auto value = option_value( { "(spb)", "pointer" }, options_in ); !value.empty( ) )
        options_out.pointer = value;

    if( auto value = option_value( { "bytes", "type" }, options_in ); !value.empty( ) )
        options_out.bytes = value;

    if( auto value = option_value( { "(spb)", "bytes" }, options_in ); !value.empty( ) )
        options_out.bytes = value;

    if( auto value = option_value( { "string", "type" }, options_in ); !value.empty( ) )
        options_out.string = value;

    if( auto value = option_value( { "(spb)", "string" }, options_in ); !value.empty( ) )
        options_out.string = value;

    if( auto value = option_value_int< uint32_t >( file, { "(spb)", "max_size" }, options_in );
        value.has_value( ) )
    {
        options_out.max_size = value;
    }
}

void convert_include_options( spb_options & options_out, const proto_options & options_in )
{
    if( auto value = option_value( { "(spb)", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "(nanopb_fileopt)", "include" }, options_in );
        !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "(nanopb_msgopt)", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "(nanopb_enumopt)", "include" }, options_in );
        !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "(nanopb)", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "repeated", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "optional", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "pointer", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "bytes", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );

    if( auto value = option_value( { "string", "include" }, options_in ); !value.empty( ) )
        options_out.include.insert( value );
}

void convert_options( proto_file & file, spb_options & options_out,
                      const proto_options & options_in )
{
    convert_include_options( file.spb_options, options_in );
    convert_gpb_options( file, options_out, options_in );
    convert_spb_options( file, options_out, options_in );
}

void resolve_options( proto_file & file, proto_enum & enum_ )
{
    convert_options( file, enum_.spb_options, enum_.options );
}

void resolve_options( proto_file & file, proto_message & message )
{
    convert_options( file, message.spb_options, message.options );
    for( auto & field : message.fields )
    {
        convert_options( file, field.spb_options, field.options );
    }
    for( auto & message : message.messages )
    {
        resolve_options( file, message );
    }
    for( auto & enum_ : message.enums )
    {
        resolve_options( file, enum_ );
    }
}
}// namespace

void resolve_options( proto_file & file )
{
    convert_options( file, file.spb_options, file.options );
    resolve_options( file, file.package );
}
