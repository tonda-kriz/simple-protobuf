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
#include "ast/proto-message.h"
#include "io/file.h"
#include "parser/parser.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <spb/json/deserialize.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace std::literals;

namespace
{

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

auto contains_label( const proto_messages & messages, proto_field::Label label ) -> bool
{
    for( const auto & message : messages )
    {
        for( const auto & field : message.fields )
        {
            if( field.label == label )
            {
                return true;
            }
        }
        if( contains_label( message.messages, label ) )
        {
            return true;
        }
    }
    return false;
}

auto contains_type( const proto_messages & messages, std::string_view type ) -> bool
{
    for( const auto & message : messages )
    {
        for( const auto & field : message.fields )
        {
            if( field.type == type )
            {
                return true;
            }
        }
        for( const auto & map : message.maps )
        {
            if( map.value_type == type )
            {
                return true;
            }
        }
        for( const auto & oneof : message.oneofs )
        {
            for( const auto & field : oneof.fields )
            {
                if( field.type == type )
                {
                    return true;
                }
            }
        }
        if( contains_type( message.messages, type ) )
        {
            return true;
        }
    }
    return false;
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

void dump_std_includes( std::ostream & stream, const proto_file & file )
{
    stream << "#include <spb/json.hpp>\n#include <spb/pb.hpp>\n#include <cstdint>\n#include <string>\n";

    if( contains_label( file.package.messages, proto_field::Label::LABEL_REPEATED ) ||
        contains_type( file.package.messages, "bytes" ) )
    {
        stream << "#include <vector>\n#include <cstddef>\n";
    }
    if( contains_label( file.package.messages, proto_field::Label::LABEL_OPTIONAL ) )
    {
        stream << "#include <optional>\n";
    }
    if( contains_label( file.package.messages, proto_field::Label::LABEL_PTR ) )
    {
        stream << "#include <memory>\n";
    }
    if( contains_map( file.package.messages ) )
    {
        stream << "#include <map>\n";
    }
    if( contains_oneof( file.package.messages ) )
    {
        stream << "#include <variant>\n";
    }
    stream << "\n";
}

void dump_options_includes( std::ostream & stream, const proto_options & options )
{
    auto include = options.find( "include" );
    if( include == options.end( ) )
    {
        return;
    }
    stream << "#include " << include->second << "\n";
}

void dump_message_includes( std::ostream & stream, const proto_message & message )
{
    for( const auto & field : message.fields )
    {
        dump_options_includes( stream, field.options );
    }

    for( const auto & m : message.messages )
    {
        dump_message_includes( stream, m );
    }
}

void dump_user_includes( std::ostream & stream, const proto_file & file )
{
    dump_message_includes( stream, file.package );
}

void dump_imports( std::ostream & stream, const proto_file & file )
{
    for( const auto & import : file.file_imports )
    {
        stream << "#include \"" << cpp_file_name_from_proto( import.path, ".pb.h" ).string( ) << "\"\n";
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

void dump_enum_field( std::ostream & stream, const proto_base & field )
{
    dump_comment( stream, field.comment );

    stream << field.name << " = " << field.number << ",\n";
}

void dump_enum( std::ostream & stream, const proto_enum & my_enum )
{
    dump_comment( stream, my_enum.comment );

    stream << "enum class " << my_enum.name << " : int32_t\n{\n";
    for( const auto & field : my_enum.fields )
    {
        dump_enum_field( stream, field );
    }
    stream << "};\n";
}

auto type_literal_suffix( std::string_view type ) -> std::string_view
{
    static constexpr auto type_map = std::array< std::pair< std::string_view, std::string_view >, 12 >{ {
        { "int64", "LL" },
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

auto convert_to_ctype( std::string_view type, const proto_options & options = { } ) -> std::string
{
    static constexpr auto type_map = std::array< std::pair< std::string_view, std::string_view >, 12 >{ {
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
        { "string", "std::string" },
        { "bytes", "std::vector< std::byte >" },
    } };

    if( auto p_option_type = options.find( "type" ); p_option_type != options.end( ) )
    {
        return std::string( p_option_type->second );
    }

    for( auto [ proto_type, c_type ] : type_map )
    {
        if( type == proto_type )
        {
            return std::string( c_type );
        }
    }

    return replace( type, ".", "::" );
}

auto has_type_override( const proto_options & options ) -> bool
{
    return options.find( "type" ) != options.end( );
}

void dump_field_type( std::ostream & stream, proto_field::Label label, const proto_field & field )
{
    const auto ctype = convert_to_ctype( field.type, field.options );

    if( has_type_override( field.options ) )
    {
        stream << ctype << " ";
        return;
    }

    switch( label )
    {
    case proto_field::Label::LABEL_NONE:
        stream << ctype << " ";
        break;
    case proto_field::Label::LABEL_OPTIONAL:
        stream << "std::optional< " << ctype << " > ";
        break;
    case proto_field::Label::LABEL_REPEATED:
        stream << "std::vector< " << ctype << " > ";
        break;
    case proto_field::Label::LABEL_PTR:
        stream << "std::unique_ptr< " << ctype << " > ";
        break;
    }
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

void dump_message_field( std::ostream & stream, const proto_field & field )
{
    dump_comment( stream, field.comment );
    dump_deprecated_attribute( stream, field );
    dump_field_type( stream, field.label, field );
    stream << field.name;
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

void dump_message( std::ostream & stream, const proto_message & message )
{
    dump_comment( stream, message.comment );

    stream << "struct " << message.name << "\n{\n";

    dump_forwards( stream, message.forwards );
    for( const auto & sub_enum : message.enums )
    {
        dump_enum( stream, sub_enum );
    }

    for( const auto & sub_message : message.messages )
    {
        dump_message( stream, sub_message );
    }

    for( const auto & field : message.fields )
    {
        dump_message_field( stream, field );
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
        dump_message( stream, message );
    }
}

void dump_enums( std::ostream & stream, const proto_file & file )
{
    for( const auto & my_enum : file.package.enums )
    {
        dump_enum( stream, my_enum );
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
    dump_pragma( stream, file );
    dump_imports( stream, file );
    dump_std_includes( stream, file );
    dump_user_includes( stream, file );
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
