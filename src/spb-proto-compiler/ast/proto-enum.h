/*
 * Protobuf compiler - proto enum storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * https://protobuf.dev/reference/protobuf/proto3-spec/#enum_definition *
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "proto-common.h"
#include <vector>

struct proto_enum : public proto_base
{
    proto_bases fields;
    proto_reserved reserved;
};

using proto_enums = std::vector<proto_enum>;
