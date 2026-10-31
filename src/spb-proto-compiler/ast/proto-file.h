/*
 * Protobuf compiler - proto file storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * https://protobuf.dev/reference/protobuf/proto3-spec
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "proto-comment.h"
#include "proto-extend.h"
#include "proto-message.h"
#include "proto-option.h"
#include "proto-service.h"
#include "proto-syntax.h"
#include <filesystem>
#include <string>
#include <vector>

struct proto_file;

using proto_files = std::vector<proto_file>;

struct proto_file
{
    std::filesystem::path path;
    std::string content;
    proto_syntax syntax;
    proto_comment comment;
    proto_files imports;
    proto_message package;
    proto_extends extends;
    proto_options options;
    proto_attributes attributes;

    proto_services services;
};
