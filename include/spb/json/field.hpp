/***************************************************************************\
* Name        : serialize library for protobuf                              *
* Description : field attributes                                            *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include <cstddef>
#include <string_view>

namespace spb::json::detail
{
struct field_attributes
{
    std::string_view name;
    size_t max_count = 0;
    size_t max_size  = 0;
};
}// namespace spb::json::detail
