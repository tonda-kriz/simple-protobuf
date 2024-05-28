/***************************************************************************\
* Name        : json dumper                                                 *
* Description : generate C++ src files for json de/serialization            *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#include "dumper.h"
#include "ast/ast.h"
#include "ast/proto-field.h"
#include "ast/proto-file.h"
#include "io/file.h"
#include "parser/parser.h"
#include "template-cpp.h"
#include "template-h.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <format>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace std::literals;

namespace
{

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

void dump_prototypes( std::ostream & stream, std::string_view type )
{
    stream << replace( file_pb_header_template, "$", type );
}

void dump_prototypes( std::ostream & stream, const proto_message & message, std::string_view parent )
{
    const auto message_with_parent = std::string( parent ) + "::" + std::string( message.name );
    dump_prototypes( stream, message_with_parent );
}

void dump_prototypes( std::ostream & stream, const proto_enum & my_enum, std::string_view parent )
{
    const auto enum_with_parent = std::string( parent ) + "::" + std::string( my_enum.name );
    dump_prototypes( stream, enum_with_parent );
}

void dump_prototypes( std::ostream & stream, const proto_enums & enums, std::string_view parent )
{
    for( const auto & my_enum : enums )
    {
        dump_prototypes( stream, my_enum, parent );
    }
}

void dump_prototypes( std::ostream & stream, const proto_messages & messages, std::string_view parent )
{
    for( const auto & message : messages )
    {
        dump_prototypes( stream, message, parent );
    }

    for( const auto & message : messages )
    {
        if( message.messages.empty( ) )
        {
            continue;
        }
        const auto message_with_parent = std::string( parent ) + "::" + std::string( message.name );
        dump_prototypes( stream, message.messages, message_with_parent );
    }

    for( const auto & message : messages )
    {
        if( message.enums.empty( ) )
        {
            continue;
        }
        const auto message_with_parent = std::string( parent ) + "::" + std::string( message.name );
        dump_prototypes( stream, message.enums, message_with_parent );
    }
}

void dump_prototypes( std::ostream & stream, const proto_file & file )
{
    const auto package_name = replace( file.package.name, ".", "::" );
    dump_prototypes( stream, file.package.messages, package_name );
    dump_prototypes( stream, file.package.enums, package_name );
}

void dump_cpp_includes( std::ostream & stream, std::string_view header_file_path )
{
    stream << "#include \"" << header_file_path << "\"\n"
           <<//"#include <sds/pb/deserialize.hpp>\n"
        "#include <sds/pb/serialize.hpp>\n"
        "#include <type_traits>\n\n";
}

void dump_cpp_prototypes( std::ostream & stream, std::string_view type )
{
    stream << replace( file_pb_cpp_template, "$", type );
}

void dump_cpp_close_namespace( std::ostream & stream, std::string_view name )
{
    stream << "} // namespace " << name << "\n";
}

void dump_cpp_open_namespace( std::ostream & stream, std::string_view name )
{
    stream << "namespace " << name << "\n{\n";
}
auto is_packed_array( const proto_field & field ) -> bool
{
    if( field.label != proto_field::Label::LABEL_REPEATED )
    {
        return false;
    }
    auto p_packed = field.options.find( "packed" );
    return p_packed != field.options.end( ) && p_packed->second == "true";
}

auto scalar_encoder_from_type( std::string_view type ) -> std::string_view
{
    if( type == "int32" ||
        type == "uint32" ||
        type == "int64" ||
        type == "uint64" ||
        type == "bool" )
    {
        return "scalar_encoder::varint";
    }

    if( type == "sint32" ||
        type == "sint64" )
    {
        return "scalar_encoder::svarint";
    }

    if( type == "fixed32" ||
        type == "sfixed32" ||
        type == "float" )
    {
        return "scalar_encoder::i32";
    }

    if( type == "fixed64" ||
        type == "sfixed64" ||
        type == "double" )
    {
        return "scalar_encoder::i64";
    }

    return "scalar_encoder::varint";
}

auto map_encoder_type( std::string_view key_type, std::string_view value_type ) -> std::string
{
    return std::format( "_as< combine( {}, {} ) >", scalar_encoder_from_type( key_type ), scalar_encoder_from_type( value_type ) );
}

auto encoder_type( const proto_field & field ) -> std::string
{
    if( field.type == "bool" ||
        field.type == "int32" ||
        field.type == "uint32" ||
        field.type == "int64" ||
        field.type == "uint64" )
    {
        if( is_packed_array( field ) )
        {
            return "_as< combine( scalar_encoder::varint, scalar_encoder::packed ) >";
        }
        return "_as< scalar_encoder::varint >";
    }

    if( field.type == "sint32" ||
        field.type == "sint64" )
    {
        if( is_packed_array( field ) )
        {
            return "_as< combine( scalar_encoder::svarint, scalar_encoder::packed ) >";
        }
        return "_as< scalar_encoder::svarint >";
    }
    if( field.type == "fixed32" ||
        field.type == "sfixed32" ||
        field.type == "float" )
    {
        if( is_packed_array( field ) )
        {
            return "_as< combine( scalar_encoder::i32, scalar_encoder::packed ) >";
        }
        return "_as< scalar_encoder::i32 >";
    }
    if( field.type == "fixed64" ||
        field.type == "sfixed64" ||
        field.type == "double" )
    {
        if( is_packed_array( field ) )
        {
            return "_as< combine( scalar_encoder::i64, scalar_encoder::packed ) >";
        }
        return "_as< scalar_encoder::i64 >";
    }
    return "";
}

/*void dump_cpp_is_empty( std::ostream & stream, const proto_enum &, std::string_view full_name )
{
    stream << "auto is_empty( const " << full_name << " & ) noexcept -> bool\n{\n\treturn false;\n}\n\n";
}*/

void dump_cpp_serialize_value( std::ostream & stream, const proto_oneof & oneof )
{
    stream << "\t{\n\t\tconst auto index = value." << oneof.name << ".index( );\n";
    stream << "\t\tswitch( index )\n\t\t{\n";
    for( size_t i = 0; i < oneof.fields.size( ); ++i )
    {
        stream << "\t\t\tcase " << i << ":\n\t\t\t\treturn stream.serialize" << encoder_type( oneof.fields[ i ] ) << "( " << oneof.fields[ i ].number << ", std::get< " << i << " >( value." << oneof.name << ") );\n";
    }
    stream << "\t\t}\n\t}\n\n";
}

void dump_cpp_serialize_value( std::ostream & stream, const proto_message & message, std::string_view full_name )
{
    if( message.fields.empty( ) && message.maps.empty( ) && message.oneofs.empty( ) )
    {
        stream << "void serialize( detail::ostream & , const " << full_name << " & )\n{\n}\n\n";
        return;
    }

    stream << "void serialize( detail::ostream & stream, const " << full_name << " & value )\n{\n";
    for( const auto & field : message.fields )
    {
        stream << "\tstream.serialize" << encoder_type( field ) << "( " << field.number << ", value." << field.name << " );\n";
    }
    for( const auto & map : message.maps )
    {
        stream << "\tstream.serialize" << map_encoder_type( map.key_type, map.value_type ) << "( " << map.number << ", value." << map.name << " );\n";
    }
    for( const auto & oneof : message.oneofs )
    {
        dump_cpp_serialize_value( stream, oneof );
    }
    stream << "}\n";
}

void dump_cpp_deserialize_value( std::ostream & stream, const proto_message & message, std::string_view full_name )
{
    if( message.fields.empty( ) && message.maps.empty( ) && message.oneofs.empty( ) )
    {
        stream << "auto deserialize_value( detail::istream &, " << full_name << " & ) -> bool\n{\n";
        stream << "\treturn true;\n}\n\n";
        return;
    }

    /*stream << "auto deserialize_value( detail::istream & stream, " << full_name << " & value ) -> bool\n{\n";
    stream << "\tswitch( djb2_hash( stream.current_key() ) )\n\t{\n";

    //- json deserializer needs to accept both camelCase (parsed_name) and the original field name
    struct one_field
    {
        std::string parsed_name;
        std::string_view name;
        size_t oneof_index = SIZE_MAX;
    };

    auto name_map = std::multimap< uint32_t, one_field >( );
    for( const auto & field : message.fields )
    {
        const auto field_name = json_field_name_or_camelCase( field );
        name_map.emplace( sds::json::detail::djb2_hash( field_name ),
                          one_field{
                              .parsed_name = field_name,
                              .name        = field.name,
                          } );
        if( field_name != field.name )
        {
            name_map.emplace( sds::json::detail::djb2_hash( field.name ),
                              one_field{
                                  .parsed_name = std::string( field.name ),
                                  .name        = field.name,
                              } );
        }
    }
    for( const auto & field : message.maps )
    {
        const auto field_name = json_field_name_or_camelCase( field );
        name_map.emplace( sds::json::detail::djb2_hash( field_name ),
                          one_field{
                              .parsed_name = field_name,
                              .name        = field.name,
                          } );
        if( field_name != field.name )
        {
            name_map.emplace( sds::json::detail::djb2_hash( field.name ),
                              one_field{ .parsed_name = std::string( field.name ),
                                         .name        = field.name } );
        }
    }
    for( const auto & oneof : message.oneofs )
    {
        for( size_t i = 0; i < oneof.fields.size( ); ++i )
        {
            const auto field_name = json_field_name_or_camelCase( oneof.fields[ i ] );
            name_map.emplace( sds::json::detail::djb2_hash( field_name ),
                              one_field{
                                  .parsed_name = field_name,
                                  .name        = oneof.name,
                                  .oneof_index = i,
                              } );
            if( field_name != oneof.fields[ i ].name )
            {
                name_map.emplace( sds::json::detail::djb2_hash( oneof.fields[ i ].name ),
                                  one_field{
                                      .parsed_name = std::string( oneof.fields[ i ].name ),
                                      .name        = oneof.name,
                                      .oneof_index = i,
                                  } );
            }
        }
    }

    auto last_hash = name_map.begin( )->first + 1;
    auto put_or    = false;
    for( const auto & [ hash, field ] : name_map )
    {
        if( hash != last_hash )
        {
            if( put_or )
            {
                stream << ";\n";
            }
            last_hash = hash;
            stream << "\t\tcase detail::djb2_hash( \"" << field.parsed_name << "\"sv ):\n\t\t\treturn\n";
            put_or = false;
        }
        if( put_or )
        {
            stream << " ||\n";
        }
        if( field.oneof_index == SIZE_MAX )
        {
            stream << "\t\t\t\tstream.deserialize( \"" << field.parsed_name << "\"sv, value." << field.name << " )";
        }
        else
        {
            stream << "\t\t\t\tstream.deserialize_variant<" << field.oneof_index << ">( \"" << field.parsed_name << "\"sv, value." << field.name << " )";
        }
        put_or = true;
    }

    stream << ";\n\t}\n\treturn false;\n}\n";*/
}

void dump_cpp_is_empty( std::ostream & stream, const proto_message &, std::string_view full_name )
{
    stream << "auto is_empty( const " << full_name << " &  ) noexcept -> bool\n{\n\treturn false;\n}\n\n";
}

void dump_cpp_messages( std::ostream & stream, const proto_messages & messages, std::string_view parent );

void dump_cpp_message( std::ostream & stream, const proto_message & message, std::string_view parent )
{
    const auto full_name = std::string( parent ) + "::" + std::string( message.name );

    dump_cpp_prototypes( stream, full_name );
    dump_cpp_open_namespace( stream, "detail" );
    dump_cpp_is_empty( stream, message, full_name );
    dump_cpp_serialize_value( stream, message, full_name );
    dump_cpp_deserialize_value( stream, message, full_name );
    dump_cpp_close_namespace( stream, "detail" );

    dump_cpp_messages( stream, message.messages, full_name );
}

void dump_cpp_messages( std::ostream & stream, const proto_messages & messages, std::string_view parent )
{
    for( const auto & message : messages )
    {
        dump_cpp_message( stream, message, parent );
    }
}

void dump_cpp( std::ostream & stream, const proto_file & file )
{
    const auto str_namespace = "::" + replace( file.package.name, ".", "::" );
    dump_cpp_messages( stream, file.package.messages, str_namespace );
}

}// namespace

void dump_pb_header( const proto_file & file, std::ostream & stream )
{
    dump_prototypes( stream, file );
}

void dump_pb_cpp( const proto_file & file, const std::filesystem::path & header_file, std::ostream & stream )
{
    dump_cpp_includes( stream, header_file.string( ) );
    dump_cpp_open_namespace( stream, "sds::pb" );
    dump_cpp( stream, file );
    dump_cpp_close_namespace( stream, "sds::pb" );
}