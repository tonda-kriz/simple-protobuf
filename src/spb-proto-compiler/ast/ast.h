/*
 * Protobuf compiler - resolve types in AST.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "ast/proto-field.h"
#include "proto-file.h"

/**
 * @brief return scalar type or no type
 *        https://protobuf.dev/programming-guides/proto3/#scalar
 *
 * @param type_name
 * @return scalar type or no type
 */
[[nodiscard]] auto get_encoder(const proto_field &field) -> proto_field::Type;

/**
 * @brief resolve types and sort all messages in a proto file
 *        so if message A depends on message B
 *        then message B must be defined before message A
 *
 */
void resolve_messages(proto_file &file);
