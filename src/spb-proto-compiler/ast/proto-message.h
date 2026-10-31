/*
 * Protobuf compiler - proto message storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * https://protobuf.dev/reference/protobuf/proto3-spec/#message_definition
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "proto-common.h"
#include "proto-enum.h"
#include "proto-field.h"
#include "proto-map.h"
#include "proto-oneof.h"
#include <set>
#include <string_view>
#include <vector>

struct proto_message;
using proto_messages         = std::vector<proto_message>;
using forwarded_declarations = std::set<std::string_view>;

struct proto_message : public proto_base
{
    proto_fields fields;
    proto_reserved_range extensions;
    proto_messages messages;
    proto_maps maps;
    proto_oneofs oneofs;
    proto_enums enums;
    proto_reserved reserved;

    //- this will be used for type dependency checking
    //- its an "resolved order" of the message in the .proto file
    //- so we can later sort messages based on their dependencies
    //- on other messages
    size_t resolved = 0;
    //- forwarded declarations needed for type dependency checking
    forwarded_declarations forwards;
};
