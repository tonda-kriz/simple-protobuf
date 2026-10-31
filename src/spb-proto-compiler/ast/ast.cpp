/*
 * Protobuf compiler - resolve types in AST.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#include "ast.h"
#include "ast-messages-order.h"
#include "ast-options.h"
#include "ast-types.h"

void resolve_messages(proto_file &file)
{
    resolve_options(file);
    resolve_types(file);
    resolve_messages_order(file);
}
