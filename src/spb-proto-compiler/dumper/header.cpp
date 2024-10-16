/***************************************************************************\
* Name        : C++ header dumper                                           *
* Description : generate C++ header with all structs and enums              *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#include "header.h"
#include "ast/ast.h"
#include "ast/proto-common.h"
#include "ast/proto-field.h"
#include "ast/proto-file.h"
#include "ast/proto-import.h"
#include "ast/proto-message.h"
#include "io/file.h"
#include "parser/options.h"
#include "parser/parser.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <initializer_list>
#include <spb/json/deserialize.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace std::literals;

namespace
{

using cpp_includes = std::set< std::string >;

void dump_comment( std::ostream & stream, const proto_comment & comment )
{
    for( const auto & comm : comment.comments )
    {
        if( comm.starts_with( "//[[" ) )
        {
            //- ignore options in comments
            continue;
        }

        stream << comm;
        if( comm.back( ) != '\n' )
        {
            stream << '\n';
        }
    }
}

auto trim_include( std::string_view str ) -> std::string
{
    auto p_begin = str.data( );
    auto p_end   = str.data( ) + str.size( );
    while( p_begin < p_end &&
           isspace( *p_begin ) )
    {
        p_begin++;
    }

    while( p_begin < p_end &&
           isspace( p_end[ -1 ] ) )
    {
        p_end--;
    }

    if( p_begin == p_end )
    {
        return { };
    }

    auto add_prefix  = *p_begin != '"' && *p_begin != '<';
    auto add_postfix = p_end[ -1 ] != '"' && p_end[ -1 ] != '>';

    if( add_prefix || add_postfix )
    {
        return '"' + std::string( str ) + '"';
    }
    return std::string( str );
}

void dump_includes( std::ostream & stream, const cpp_includes & includes )
{
    for( auto & include : includes )
    {
        auto file = trim_include( include );
        if( !file.empty( ) )
        {
            stream << "#include " << file << "\n";
        }
    }
    stream << "\n";
}

auto contains_map( const proto_messages & messages ) -> bool
{
    for( const auto & message : messages )
    {
        if( !message.maps.empty( ) )
        {
            return true;
        }

        if( contains_map( message.messages ) )
        {
            return true;
        }
    }
    return false;
}

auto contains_oneof( const proto_messages & messages ) -> bool
{
    for( const auto & message : messages )
    {
        if( !message.oneofs.empty( ) )
        {
            return true;
        }

        if( contains_oneof( message.messages ) )
        {
            return true;
        }
    }
    return false;
}

void get_std_includes( cpp_includes & includes, const proto_file & file )
{
    includes.insert( "<spb/json.hpp>" );
    includes.insert( "<spb/pb.hpp>" );
    includes.insert( "<cstdint>" );
    includes.insert( "<cstddef>" );

    if( contains_map( file.package.messages ) )
    {
        includes.insert( "<map>" );
    }
    if( contains_oneof( file.package.messages ) )
    {
        includes.insert( "<variant>" );
    }
}

void get_include_from_options( cpp_includes & includes, const proto_options & options, const proto_options & message_options, const proto_options & file_options, std::string_view option_include )
{
    if( auto include = options.find( option_include ); include != options.end( ) )
    {
        includes.insert( std::string( include->second ) );
        return;
    }
    if( auto include = message_options.find( option_include ); include != message_options.end( ) )
    {
        includes.insert( std::string( include->second ) );
        return;
    }
    if( auto include = file_options.find( option_include ); include != file_options.end( ) )
    {
        includes.insert( std::string( include->second ) );
        return;
    }
}

void get_message_includes( cpp_includes & includes, const proto_message & message, const proto_file & file )
{
    for( const auto & field : message.fields )
    {
        if( field.label == proto_field::Label::LABEL_OPTIONAL )
        {
            get_include_from_options( includes, field.options, message.options, file.options, option_optional_include );
        }
        if( field.label == proto_field::Label::LABEL_REPEATED )
        {
            get_include_from_options( includes, field.options, message.options, file.options, option_repeated_include );
        }
        if( field.label == proto_field::Label::LABEL_PTR )
        {
            get_include_from_options( includes, field.options, message.options, file.options, option_pointer_include );
        }
        if( field.type == "string"sv )
        {
            get_include_from_options( includes, field.options, message.options, file.options, option_string_include );
        }
        if( field.type == "bytes"sv )
        {
            get_include_from_options( includes, field.options, message.options, file.options, option_bytes_include );
        }
    }

    for( const auto & m : message.messages )
    {
        get_message_includes( includes, m, file );
    }
}

void get_user_includes( cpp_includes & includes, const proto_file & file )
{
    get_message_includes( includes, file.package, file );
}

void get_imports( cpp_includes & includes, const proto_file & file )
{
    for( const auto & import : file.file_imports )
    {
        includes.insert( "\"" + cpp_file_name_from_proto( import.path, ".pb.h" ).string( ) + "\"" );
    }
}

void dump_pragma( std::ostream & stream, const proto_file & )
{
    stream << "#pragma once\n\n";
}

void dump_syntax( std::ostream & stream, const proto_file & file )
{
    dump_comment( stream, file.syntax.comments );
}

auto type_literal_suffix( std::string_view type ) -> std::string_view
{
    static constexpr auto type_map = std::array< std::pair< std::string_view, std::string_view >, 12 >{ {
        { "int64", "LL" },
        { "uint8", "U" },
        { "uint16", "U" },
        { "uint32", "U" },
        { "uint64", "ULL" },
        { "sint64", "LL" },
        { "fixed32", "U" },
        { "fixed64", "ULL" },
        { "sfixed64", "ULL" },
        { "float", "F" },
    } };

    for( auto [ proto_type, suffix ] : type_map )
    {
        if( type == proto_type )
        {
            return suffix;
        }
    }

    return { };
}

auto types_are_compatible( std::string_view type, std::string_view field_type ) -> bool
{
    static constexpr auto type_map = std::array< std::pair< std::string_view, std::array< std::string_view, 4 > >, 16 >{ {
        { "int8", { "int8" } },
        { "uint8", { "uint8" } },
        { "int16", { "int8", "int16" } },
        { "uint16", { "uint8", "uint16" } },
        { "int32", { "int8", "int16", "int32" } },
        { "int64", { "int8", "int16", "int32", "int64" } },
        { "uint32", { "uint8", "uint16", "uint32" } },
        { "uint64", { "uint8", "uint16", "uint32", "uint64" } },
        { "sint32", { "int8", "int16", "int32" } },
        { "sint64", { "int8", "int16", "int32", "int64" } },
        { "fixed32", { "uint8", "uint16", "uint32" } },
        { "fixed64", { "uint8", "uint16", "uint32", "uint64" } },
        { "sfixed32", { "int8", "int16", "int32" } },
        { "sfixed64", { "int8", "int16", "int32", "int64" } },
    } };

    for( auto [ proto_type, types ] : type_map )
    {
        if( type == proto_type )
        {
            return std::find( types.begin( ), types.end( ), field_type ) != types.end( );
        }
    }
    return false;
}

auto remove_bitfield( std::string_view type ) -> std::string_view
{
    return type.substr( 0, type.find( ':' ) );
}

auto get_field_type( const proto_options & options, std::string_view ctype ) -> std::string
{
    if( auto p_name = options.find( option_field_type ); p_name != options.end( ) )
    {
        auto field_type = remove_bitfield( p_name->second );

        if( types_are_compatible( ctype, field_type ) )
        {
            return std::string( field_type );
        }
    }
    return std::string( ctype );
}

auto get_field_bits( const proto_field & field ) -> std::string_view
{
    if( auto p_name = field.options.find( option_field_type ); p_name != field.options.end( ) )
    {
        auto bitfield = p_name->second;
        if( auto index = bitfield.find( ':' ); index != std::string_view::npos )
        {
            const_cast< proto_field & >( field ).bit_field = bitfield.substr( index + 1 );
            return bitfield.substr( index );
        }
    }
    return { };
}

auto get_container_type( const proto_options & options, const proto_options & message_options, const proto_options & file_options, std::string_view option, std::string_view ctype, std::string_view default_type = { } ) -> std::string
{
    if( auto p_name = options.find( option ); p_name != options.end( ) )
    {
        return replace( p_name->second, "$", ctype );
    }

    if( auto p_name = message_options.find( option ); p_name != message_options.end( ) )
    {
        return replace( p_name->second, "$", ctype );
    }

    if( auto p_name = file_options.find( option ); p_name != file_options.end( ) )
    {
        return replace( p_name->second, "$", ctype );
    }

    return replace( default_type, "$", ctype );
}

auto get_enum_type( const proto_options & options, const proto_options & message_options, const proto_options & file_options, std::string_view default_type ) -> std::string_view
{
    static constexpr auto type_map = std::array< std::pair< std::string_view, std::string_view >, 6 >{ {
        { "int8"sv, "int8_t"sv },
        { "uint8"sv, "uint8_t"sv },
        { "int16"sv, "int16_t"sv },
        { "uint16"sv, "uint16_t"sv },
        { "int32"sv, "int32_t"sv },
    } };

    auto ctype_from_pb = [ = ]( std::string_view type )
    {
        for( auto [ proto_type, c_type ] : type_map )
        {
            if( type == proto_type )
            {
                return c_type;
            }
        }
        throw std::runtime_error( "invalid enum type" );
    };

    if( auto p_name = options.find( option_enum_type ); p_name != options.end( ) )
    {
        return ctype_from_pb( p_name->second );
    }

    if( auto p_name = message_options.find( option_enum_type ); p_name != message_options.end( ) )
    {
        return ctype_from_pb( p_name->second );
    }

    if( auto p_name = file_options.find( option_enum_type ); p_name != file_options.end( ) )
    {
        return ctype_from_pb( p_name->second );
    }

    return default_type;
}

auto convert_to_ctype( std::string_view type, const proto_options & options = { }, const proto_options & message_options = { }, const proto_options & file_options = { } ) -> std::string
{
    static constexpr auto type_map = std::array< std::pair< std::string_view, std::string_view >, 16 >{ {
        { "int8", "int8_t" },
        { "uint8", "uint8_t" },
        { "int16", "int16_t" },
        { "uint16", "uint16_t" },
        { "int32", "int32_t" },
        { "int64", "int64_t" },
        { "uint32", "uint32_t" },
        { "uint64", "uint64_t" },
        { "sint32", "int32_t" },
        { "sint64", "int64_t" },
        { "fixed32", "uint32_t" },
        { "fixed64", "uint64_t" },
        { "sfixed32", "int32_t" },
        { "sfixed64", "int64_t" },
    } };

    if( type == "string"sv )
    {
        return get_container_type( options, message_options, file_options, option_string_type, "char", "std::string" );
    }

    if( type == "bytes"sv )
    {
        return get_container_type( options, message_options, file_options, option_bytes_type, "std::byte", "std::vector<$>" );
    }

    auto type_str = get_field_type( options, type );

    for( auto [ proto_type, c_type ] : type_map )
    {
        if( type_str == proto_type )
        {
            return std::string( c_type );
        }
    }

    return replace( type, ".", "::" );
}

void dump_field_type_and_name( std::ostream & stream, const proto_field & field, const proto_message & message, const proto_file & file )
{
    const auto ctype = convert_to_ctype( field.type, field.options, message.options, file.options );

    switch( field.label )
    {
    case proto_field::Label::LABEL_NONE:
        stream << ctype;
        break;
    case proto_field::Label::LABEL_OPTIONAL:
        stream << get_container_type( field.options, message.options, file.options, option_optional_type, ctype, "std::optional<$>" );
        break;
    case proto_field::Label::LABEL_REPEATED:
        stream << get_container_type( field.options, message.options, file.options, option_repeated_type, ctype, "std::vector<$>" );
        break;
    case proto_field::Label::LABEL_PTR:
        stream << get_container_type( field.options, message.options, file.options, option_pointer_type, ctype, "std::unique_ptr<$>" );
        break;
    }
    stream << ' ' << field.name << get_field_bits( field );
}

void dump_enum_field( std::ostream & stream, const proto_base & field )
{
    dump_comment( stream, field.comment );

    stream << field.name << " = " << field.number << ",\n";
}

void dump_enum( std::ostream & stream, const proto_enum & my_enum, const proto_message & message, const proto_file & file )
{
    dump_comment( stream, my_enum.comment );

    stream << "enum class " << my_enum.name << " : " << get_enum_type( my_enum.options, message.options, file.options, "int32_t" ) << "\n{\n";
    for( const auto & field : my_enum.fields )
    {
        dump_enum_field( stream, field );
    }
    stream << "};\n";
}

void dump_message_oneof( std::ostream & stream, const proto_oneof & oneof )
{
    dump_comment( stream, oneof.comment );

    auto put_comma = false;
    stream << "std::variant< ";
    for( const auto & field : oneof.fields )
    {
        if( put_comma )
        {
            stream << ", ";
        }

        stream << convert_to_ctype( field.type );
        put_comma = true;
    }
    stream << " > " << oneof.name << ";\n";
}

void dump_message_map( std::ostream & stream, const proto_map & map )
{
    dump_comment( stream, map.comment );

    stream << "std::map< " << convert_to_ctype( map.key_type ) << ", " << convert_to_ctype( map.value_type ) << " > ";
    stream << map.name << ";\n";
}

void dump_default_value( std::ostream & stream, const proto_field & field )
{
    if( auto p_index = field.options.find( "default" ); p_index != field.options.end( ) )
    {
        if( is_scalar_type( field.type ) )
        {
            if( field.type == "string" &&
                ( p_index->second.size( ) < 2 || p_index->second.front( ) != '"' || p_index->second.back( ) != '"' ) )
            {
                stream << " = \"" << p_index->second << "\"";
            }
            else
            {
                stream << " = " << p_index->second << type_literal_suffix( field.type );
            }
        }
        else//- this has to be an enum type
        {
            stream << " = " << replace( field.type, ".", "::" ) << "::" << p_index->second;
        }
    }
}

void dump_deprecated_attribute( std::ostream & stream, const proto_field & field )
{
    if( auto p_index = field.options.find( "deprecated" ); p_index != field.options.end( ) && p_index->second == "true" )
    {
        stream << "[[deprecated]] ";
    }
}

void dump_message_field( std::ostream & stream, const proto_field & field, const proto_message & message, const proto_file & file )
{
    dump_comment( stream, field.comment );
    dump_deprecated_attribute( stream, field );
    dump_field_type_and_name( stream, field, message, file );
    dump_default_value( stream, field );
    stream << ";\n";
}

void dump_forwards( std::ostream & stream, const forwarded_declarations & forwards )
{
    for( const auto & forward : forwards )
    {
        stream << "struct " << forward << ";\n";
    }
    if( !forwards.empty( ) )
    {
        stream << '\n';
    }
}

void dump_message( std::ostream & stream, const proto_message & message, const proto_file & file )
{
    dump_comment( stream, message.comment );

    stream << "struct " << message.name << "\n{\n";

    dump_forwards( stream, message.forwards );
    for( const auto & sub_enum : message.enums )
    {
        dump_enum( stream, sub_enum, message, file );
    }

    for( const auto & sub_message : message.messages )
    {
        dump_message( stream, sub_message, file );
    }

    for( const auto & field : message.fields )
    {
        dump_message_field( stream, field, message, file );
    }

    for( const auto & map : message.maps )
    {
        dump_message_map( stream, map );
    }

    for( const auto & oneof : message.oneofs )
    {
        dump_message_oneof( stream, oneof );
    }

    //- TODO: is this used in any way?
    // stream << "auto operator == ( const " << message.name << " & ) const noexcept -> bool = default;\n";
    // stream << "auto operator != ( const " << message.name << " & ) const noexcept -> bool = default;\n";
    stream << "};\n";
}

void dump_messages( std::ostream & stream, const proto_file & file )
{
    dump_forwards( stream, file.package.forwards );
    for( const auto & message : file.package.messages )
    {
        dump_message( stream, message, file );
    }
}

void dump_enums( std::ostream & stream, const proto_file & file )
{
    for( const auto & my_enum : file.package.enums )
    {
        dump_enum( stream, my_enum, file.package, file );
    }
}

void dump_package_begin( std::ostream & stream, const proto_file & file )
{
    dump_comment( stream, file.package.comment );
    if( !file.package.name.empty( ) )
    {
        stream << "namespace " << replace( file.package.name, ".", "::" ) << "\n{\n";
    }
}

void dump_package_end( std::ostream & stream, const proto_file & file )
{
    if( !file.package.name.empty( ) )
    {
        stream << "}// namespace " << replace( file.package.name, ".", "::" ) << "\n\n";
    }
}
}// namespace

void dump_cpp_header( const proto_file & file, std::ostream & stream )
{
    auto includes = cpp_includes( );
    dump_pragma( stream, file );
    get_imports( includes, file );
    get_std_includes( includes, file );
    get_user_includes( includes, file );
    dump_includes( stream, includes );
    dump_syntax( stream, file );
    dump_package_begin( stream, file );
    dump_enums( stream, file );
    dump_messages( stream, file );
    dump_package_end( stream, file );
}

auto replace( std::string_view input, std::string_view what, std::string_view with ) -> std::string
{
    auto result = std::string( input );
    auto pos    = size_t{ };

    while( ( pos = result.find( what, pos ) ) != std::string::npos )
    {
        result.replace( pos, what.size( ), with );
        pos += with.size( );
    }

    return result;
}
