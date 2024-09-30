/***************************************************************************\
* Name        : template concepts                                           *
* Description : general template concepts used by protobuf de/serializer    *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/
#pragma once

#include <cstddef>
#include <type_traits>

namespace spb::detail
{
template < class T >
concept is_struct = ::std::is_class_v< T > && requires( T obj ) {
    { obj.size( ) };
} == false;

template < class T >
concept is_enum = ::std::is_enum_v< T >;

template < class T >
concept is_int_or_float = ::std::is_integral_v< T > || ::std::is_floating_point_v< T >;

template < class T >
concept repeated_container = requires( T container ) {
    { container.emplace_back( ) };
    { container.begin( ) };
    { container.end( ) };
    { container.clear( ) };
    typename T::value_type;
} && !std::is_same_v< typename T::value_type, std::byte > && !std::is_same_v< typename T::value_type, char >;

template < class T >
concept bytes_container = requires( T container ) {
    { container.emplace_back( ) };
    { container.empty( ) };
    { container.begin( ) };
    { container.end( ) };
    { container.clear( ) };
    typename T::value_type;
} && std::is_same_v< typename T::value_type, std::byte >;

template < class T >
concept string_container = requires( T container, size_t new_size, const char * str ) {
    { container.data( ) };
    { container.empty( ) };
    { container.size( ) };
    { container.resize( new_size ) };
    { container.begin( ) };
    { container.end( ) };
    { container.clear( ) };
    { container.append( str, new_size ) };
    typename T::value_type;
} && std::is_same_v< typename T::value_type, char >;

}// namespace spb::detail