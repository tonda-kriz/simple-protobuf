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
concept is_enum = ::std::is_enum_v< T >;

template < class T >
concept is_int_or_float = ::std::is_integral_v< T > || ::std::is_floating_point_v< T >;

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
concept string_container = requires( T container ) {
    { container.data( ) };
    { container.empty( ) };
    { container.size( ) };
    { container.resize( 0 ) };
    { container.begin( ) };
    { container.end( ) };
    { container.clear( ) };
    { container.append( "", 0 ) };
    typename T::value_type;
} && std::is_same_v< typename T::value_type, char >;

template < class T >
concept repeated_container = requires( T container ) {
    { container.emplace_back( ) };
    { container.begin( ) };
    { container.end( ) };
    { container.clear( ) };
    typename T::value_type;
} && !string_container< T > && !bytes_container< T >;

template < class T >
concept optional_container = requires( T container ) {
    { container.has_value( ) };
    { container.reset( ) };
    { *container } -> std::same_as< typename T::value_type & >;
    { container.emplace( typename T::value_type( ) ) } -> std::same_as< typename T::value_type & >;
    typename T::value_type;
};

template < class T >
concept is_struct = ::std::is_class_v< T > &&
    !string_container< T > &&
    !bytes_container< T > &&
    !repeated_container< T > &&
    !optional_container< T >;

}// namespace spb::detail