/***************************************************************************\
* Name        : json dumper                                                 *
* Description : C++ template used for json de/serialization                 *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include <string_view>

constexpr std::string_view file_json_cpp_template = R"(
auto serialize_size( const $ & value ) noexcept -> size_t
{
    return detail::serialize_size( value );
}

auto serialize( const $ & value ) -> std::string
{
    return detail::serialize( value );
}

auto serialize( const $ & value, void * buffer ) -> size_t
{
    return detail::serialize( value, buffer );
}

template <>
auto deserialize< $ >( std::string_view json ) -> $
{
    return detail::deserialize< $ >( json );
}

void deserialize( $ & result, std::string_view json )
{
    return detail::deserialize( result, json );
}

)";
