/***************************************************************************\
* Name        : ast render                                                  *
* Description : resolve types in ast tree                                   *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "proto-file.h"
#include <filesystem>
#include <span>
#include <string_view>

/**
 * @brief return true if type is a scalar type
 *        https://protobuf.dev/programming-guides/proto3/#scalar
 *
 * @param type
 * @return true if type is a scalar type
 */
[[nodiscard]] auto is_scalar_type( std::string_view type ) -> bool;

/**
 * @brief sort all messages in a proto file so if message A depends on message B
 *        then message B must be defined before message A
 *
 */
void resolve_messages( proto_file & file );
