/***************************************************************************\
* Name        : generic reader and writer                                   *
* Description : generic output stream used for serialization                *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once
#include "function_ref.hpp"
#include <cstdlib>

namespace spb::io
{
//- generic writer used to write exactly `size` number of bytes from `p_data`
using writer = spb::detail::function_ref< void( const void * p_data, size_t size ) >;

//- generic reader used to read exactly `size` number of bytes into `p_data`
using reader = spb::detail::function_ref< void( void * p_data, size_t size ) >;

}// namespace spb::io
