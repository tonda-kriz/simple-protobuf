/*
 * Protobuf compiler - proto oneof storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Reference: https://protobuf.dev/reference/protobuf/proto3-spec/#oneof_and_oneof_field
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "proto-common.h"
#include "proto-field.h"
#include <vector>

struct proto_oneof : public proto_base
{
    proto_fields fields;
};

using proto_oneofs = std::vector<proto_oneof>;
