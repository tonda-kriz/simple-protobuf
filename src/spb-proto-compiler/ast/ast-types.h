/*
 * Protobuf compiler - field types.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "ast/proto-field.h"
#include "proto-file.h"

[[nodiscard]] auto is_scalar(const proto_field::Type &type) -> bool;
[[nodiscard]] auto is_packed_array(const proto_file &file, const proto_field &field) -> bool;

/**
 * @brief resolve types in a proto file
 *
 */
void resolve_types(proto_file &file);
