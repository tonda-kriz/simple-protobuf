/***************************************************************************\
* Name        : proto ast                                                   *
* Description : data structure for map                                      *
* Link        : https://protobuf.dev/reference/protobuf/proto3-spec/#map_field *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "proto-common.h"
#include <string_view>

struct proto_map : public proto_base
{
    std::string_view key_type;
    std::string_view value_type;
};

using proto_maps = std::vector< proto_map >;