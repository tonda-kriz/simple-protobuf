/***************************************************************************\
* Name        : template concepts                                           *
* Description : general template concepts used by protobuf                  *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/
#pragma once

#include <type_traits>

namespace sds::pb::detail
{
template < class T >
concept is_struct = ::std::is_class_v< T >;

template < class T >
concept is_enum = ::std::is_enum_v< T >;

template < class T >
concept is_int_or_float = ::std::is_integral_v< T > || ::std::is_floating_point_v< T >;

}// namespace sds::pb::detail