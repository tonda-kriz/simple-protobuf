/*
 * Protobuf compiler - proto extend storage.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "proto-field.h"
#include <unordered_map>

using proto_extends = std::unordered_map<std::string_view, proto_fields>;
