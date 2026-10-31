/*
 * Protobuf compiler - proto map storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * https://protobuf.dev/reference/protobuf/proto3-spec/#map_field
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "ast/proto-field.h"
#include "proto-common.h"

struct proto_map : public proto_base
{
    proto_field key;
    proto_field value;
};

using proto_maps = std::vector<proto_map>;
