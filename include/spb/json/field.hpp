/*
 * Protobuf serialization library - JSON field attributes.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include <cstddef>
#include <stdexcept>

namespace spb::json::detail
{
struct field_attributes
{
    size_t max_count = 0;
    size_t max_size  = 0;
};

inline void check_size(size_t size, size_t max_size)
{
    if (size > max_size) [[unlikely]]
        throw std::length_error("field is too large");
}

} // namespace spb::json::detail
