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

}// namespace

void dump_pb_header( const proto_file & file, std::ostream & stream )
{
    dump_prototypes( stream, file );
}

void dump_pb_cpp( const proto_file & file, const std::filesystem::path & header_file, std::ostream & stream )
{
}