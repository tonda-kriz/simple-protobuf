/*
 * Protobuf compiler - resolve dependencies between messages.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "proto-file.h"

void resolve_messages_order(proto_file &file);
