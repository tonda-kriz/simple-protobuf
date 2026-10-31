/*
 * Protobuf compiler - proto syntax storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Reference: https://protobuf.dev/reference/protobuf/proto3-spec/#syntax
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "proto-comment.h"
#include <cstdint>

struct proto_syntax
{
    uint32_t version = 2;
    proto_comment comments;
};
