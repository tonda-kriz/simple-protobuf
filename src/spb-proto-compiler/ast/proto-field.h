/***************************************************************************\
* Name        : proto ast                                                   *
* Description : data structure for a field                                  *
* Link        : https://protobuf.dev/reference/protobuf/proto3-spec/#normal_field *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "proto-common.h"
#include <string_view>
#include <vector>

struct proto_field : public proto_base
{
    enum Label
    {
        //- no type modifier, only `type`
        LABEL_NONE = 0,
        //- std::optional< type >
        LABEL_OPTIONAL = 1,
        //- std::vector< type >
        LABEL_REPEATED = 2,
        //- std::unique_ptr< type >, used to break up circular references
        LABEL_PTR = 3,
    };

    Label label = LABEL_OPTIONAL;
    std::string_view type;

    std::string_view bit_field;
};

using proto_fields = std::vector< proto_field >;
