/*
 * Protobuf compiler - proto comments storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * https://protobuf.dev/programming-guides/proto3/
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include <string_view>
#include <vector>

struct proto_comment
{
    //- points to .proto file
    std::vector<std::string_view> comments;
};
