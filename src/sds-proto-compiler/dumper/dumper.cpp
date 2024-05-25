/***************************************************************************\
* Name        : CPP dumper                                                  *
* Description : generate C++ src files for de/serialization                 *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#include "dumper.h"
#include "header.h"
#include "io/file.h"
#include "pb/dumper.h"
#include "json/dumper.h"

void dump_cpp_header( const proto_file & file, const std::filesystem::path & file_path )
{
    auto stream = std::stringstream( );

    dump_cpp_header( file, stream );
    dump_json_header( file, stream );
    dump_pb_header( file, stream );

    save_file( file_path, stream.str( ) );
}

void dump_cpp( const proto_file & file, const std::filesystem::path & header_file, const std::filesystem::path & file_path )
{
    auto stream = std::stringstream( );

    dump_json_cpp( file, header_file.string( ), stream );

    save_file( file_path, stream.str( ) );
}
