/***************************************************************************\
* Name        : serialize library for json                                  *
* Description : all json serialization functions                            *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "../concepts.h"

#include "../to_from_chars.h"
#include "base64.h"
#include "field.hpp"
#include "options.h"
#include "spb/utf8.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cinttypes>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <spb/io/io.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <type_traits>

namespace spb::json::detail
{
struct ostream_size
{
    static constexpr bool size_only = true;

    const serialize_options options = {};
    size_t size                     = 0;
    bool put_comma                  = false;

    void write(uint8_t) noexcept
    {
        size += sizeof(uint8_t);
    }

    void write(const void *, size_t data_size) noexcept
    {
        size += data_size;
    }
};

struct ostream_buffer
{
    static constexpr bool size_only = false;

    const serialize_options options;
    uint8_t *p_buffer;
    bool put_comma = false;

    void write(uint8_t byte) noexcept
    {
        *p_buffer++ = byte;
    }

    void write(const void *data, size_t data_size) noexcept
    {
        memcpy(p_buffer, data, data_size);
        p_buffer += data_size;
    }
};

struct ostream_writer
{
    static constexpr bool size_only = false;

    const serialize_options options;
    const spb::io::writer on_write;
    size_t size    = 0;
    bool put_comma = false;

    void write(uint8_t byte)
    {
        write(&byte, 1);
    }

    void write(const void *data, size_t data_size)
    {
        on_write(data, data_size);
        size += data_size;
    }
};

template <field_attributes = field_attributes{}>
size_t serialize_size(const auto &value, const serialize_options &options);

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// Returns true if x round-trips through being cast to a double, i.e., if
// x is represenable exactly as a double. This is a slightly weaker condition
// than x < 2^52.
template <typename Int> bool RoundTripsThroughDouble(Int x)
{
    auto d = static_cast<double>(x);
    // d has guaranteed to be finite with no fractional part, because it came from
    // an integer, so we only need to check that it is not outside of the
    // representable range of `int`. The way to do this is somewhat not obvious:
    // UINT64_MAX isn't representable, and what it gets rounded to when we go
    // int->double is unspecified!
    //
    // Thus, we have to go through ldexp.
    double min          = 0;
    double max_plus_one = std::ldexp(1.0, sizeof(Int) * 8);
    if (std::is_signed<Int>::value)
    {
        max_plus_one /= 2;
        min = -max_plus_one;
    }

    if (d < min || d >= max_plus_one)
    {
        return false;
    }

    return static_cast<Int>(d) == x;
}

void write_uft8(auto &stream, uint32_t codepoint)
{
    char buffer[4];

    const auto size = spb::detail::utf8::encode_point(codepoint, buffer);
    stream.write(buffer, size);
}

void write_unicode(auto &stream, uint32_t codepoint)
{
    if (codepoint <= 0xffff)
    {
        char buffer[8] = {};
        auto size      = snprintf(buffer, sizeof(buffer), "\\u%04" PRIx32, codepoint);
        return stream.write(buffer, size);
    }
    if (codepoint <= 0x10FFFF)
    {
        codepoint -= 0x10000;

        auto high       = static_cast<uint16_t>((codepoint >> 10) + 0xD800);
        auto low        = static_cast<uint16_t>((codepoint & 0x3FF) + 0xDC00);
        char buffer[16] = {};
        auto size       = snprintf(buffer, sizeof(buffer), "\\u%04" PRIx16 "\\u%04" PRIx16, high, low);
        return stream.write(buffer, size);
    }
    throw std::invalid_argument("invalid utf8");
}

template <size_t N> void write_string(auto &stream, const char (&string)[N])
{
    stream.write(string, N - 1);
}

inline constexpr auto is_ascii_escape(uint8_t c) -> bool
{
    switch (c)
    {
    case '\\':
    case '\"':
    case '<':
    case '>':
        return true;
    default:
        return (c < ' ') || c >= 0x7f;
    }
}

inline constexpr auto is_printable_ascii(std::string_view str) -> bool
{
    return std::none_of(str.begin(), str.end(), is_ascii_escape);
}

inline constexpr auto is_escape(uint32_t c) -> bool
{
    constexpr std::array<std::pair<uint32_t, uint32_t>, 20> escapes = {
        {{0, 31},        {34, 34},       {60, 60},         {62, 62},         {92, 92},
         {127, 159},     {173, 173},     {1536, 1539},     {1757, 1757},     {1807, 1807},
         {6068, 6069},   {8203, 8207},   {8232, 8238},     {8288, 8292},     {8298, 8303},
         {65279, 65279}, {65529, 65531}, {119155, 119162}, {917505, 917505}, {917536, 917631}}};

    auto it = std::lower_bound(escapes.begin(), escapes.end(), c,
                               [](auto range, auto c) { return range.second < c; });
    return it != escapes.end() && it->first <= c;
}

void write_escaped(auto &stream, std::string_view str)
{
    if (is_printable_ascii(str)) [[likely]]
    {
        stream.write(str.data(), str.size());
        return;
    }

    uint32_t codepoint = 0;
    uint32_t state     = spb::detail::utf8::ok;
    for (uint8_t c : str)
    {
        if (spb::detail::utf8::decode_point(&state, &codepoint, c) != spb::detail::utf8::ok) [[unlikely]]
            continue;

        if (is_escape(codepoint)) [[unlikely]]
        {
            switch (codepoint)
            {
            case '"':
                write_string(stream, R"(\")");
                break;
            case '\\':
                write_string(stream, R"(\\)");
                break;
            case '\b':
                write_string(stream, R"(\b)");
                break;
            case '\f':
                write_string(stream, R"(\f)");
                break;
            case '\n':
                write_string(stream, R"(\n)");
                break;
            case '\r':
                write_string(stream, R"(\r)");
                break;
            case '\t':
                write_string(stream, R"(\t)");
                break;
            default:
                write_unicode(stream, codepoint);
            }
        }
        else
        {
            write_uft8(stream, codepoint);
        }
    }

    if (state != spb::detail::utf8::ok) [[unlikely]]
        throw std::runtime_error("invalid utf8");
}

void put_comma_if_needed(auto &stream)
{
    if (stream.put_comma)
        stream.write(',');

    stream.put_comma = true;
}

void serialize_key(auto &stream, std::string_view key)
{
    put_comma_if_needed(stream);

    stream.write('"');
    stream.write(key.data(), key.size());
    write_string(stream, R"(":)");
}

template <field_attributes> void serialize(auto &stream, const bool &value);
template <field_attributes> void serialize(auto &stream, const bool &value, std::string_view field);

template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_field_int_or_float auto &value);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_field_int_or_float auto &value, std::string_view field);

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_message auto &value);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_message auto &value, std::string_view field);

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_enum auto &value);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_enum auto &value, std::string_view field);

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_field_string auto &value);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_field_string auto &value, std::string_view field);

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_field_bytes auto &value);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_field_bytes auto &value, std::string_view field);

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_optional auto &p_value);
template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_optional auto &p_value, std::string_view field);

template <field_attributes attributes, typename T>
void serialize(auto &stream, const std::unique_ptr<T> &p_value);
template <field_attributes attributes, typename T>
void serialize(auto &stream, const std::unique_ptr<T> &p_value, std::string_view field);

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_label_repeated auto &value);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_label_repeated auto &value, std::string_view field);

template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_label_repeated_fixed_size auto &value);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_label_repeated_fixed_size auto &value,
               std::string_view field);

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_map auto &map);
template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_map auto &map, std::string_view field);

template <field_attributes> void serialize(auto &stream, const bool &value)
{
    return value ? write_string(stream, "true") : write_string(stream, "false");
}

void serialize_enum(auto &stream, std::string_view value)
{
    stream.write('"');
    stream.write(value.data(), value.size());
    stream.write('"');
}

template <field_attributes attributes> void serialize(auto &stream, const std::string_view &value)
{
    using stream_type = std::remove_cvref_t<decltype(stream)>;

    if constexpr (!stream_type::size_only && attributes.max_size)
        check_size(value.size(), attributes.max_size);

    stream.write('"');
    write_escaped(stream, value);
    stream.write('"');
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_field_string auto &value)
{
    serialize<attributes>(stream, std::string_view(value.data(), value.size()));
}

template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_field_int_or_float auto &value)
{
    using value_type = std::remove_cvref_t<decltype(value)>;

    auto buffer = std::array<char, 32>();
    auto result = spb_std_emu::to_chars(buffer.data(), buffer.data() + sizeof(buffer), value);
    if constexpr (std::is_same_v<value_type, int64_t> || std::is_same_v<value_type, uint64_t>)
    {
        if (!stream.options.quote_inexact_int64 || RoundTripsThroughDouble(value)) [[likely]]
        {
            stream.write(buffer.data(), static_cast<size_t>(result.ptr - buffer.data()));
        }
        else
        {
            stream.write('"');
            stream.write(buffer.data(), static_cast<size_t>(result.ptr - buffer.data()));
            stream.write('"');
        }
    }
    else
    {
        stream.write(buffer.data(), static_cast<size_t>(result.ptr - buffer.data()));
    }
}

template <field_attributes attributes> void serialize(auto &stream, const bool &value, std::string_view field)
{
    serialize_key(stream, field);
    serialize<attributes>(stream, value);
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_field_int_or_float auto &value, std::string_view field)
{
    serialize_key(stream, field);
    serialize<attributes>(stream, value);
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_field_string auto &value, std::string_view field)
{
    if (value.empty())
        return;

    serialize_key(stream, field);
    serialize<attributes>(stream, value);
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_field_bytes auto &value)
{
    using stream_type = std::remove_cvref_t<decltype(stream)>;

    if constexpr (!stream_type::size_only && attributes.max_size)
        check_size(value.size(), attributes.max_size);

    stream.write('"');
    base64_encode(stream, value);
    stream.write('"');
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_field_bytes auto &value, std::string_view field)
{
    if (value.empty())
        return;

    serialize_key(stream, field);
    serialize<attributes>(stream, value);
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_repeated auto &value, std::string_view field)
{
    if (value.empty())
        return;

    serialize_key(stream, field);
    serialize<attributes>(stream, value);
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_repeated auto &value)
{
    using stream_type = std::remove_cvref_t<decltype(stream)>;

    if constexpr (!stream_type::size_only && attributes.max_count)
        check_size(value.size(), attributes.max_count);

    stream.write('[');
    stream.put_comma = false;
    for (const auto &v : value)
    {
        put_comma_if_needed(stream);

        if constexpr (std::is_same_v<typename std::decay_t<decltype(value)>::value_type, bool>)
        {
            serialize<attributes>(stream, bool(v));
        }
        else
        {
            serialize<attributes>(stream, v);
        }
    }
    stream.write(']');
    stream.put_comma = true;
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_repeated_fixed_size auto &value)
{
    stream.write('[');
    stream.put_comma = false;
    for (size_t i = 0; i < value.size(); i++)
    {
        put_comma_if_needed(stream);

        if constexpr (std::is_same_v<typename std::decay_t<decltype(value)>::value_type, bool>)
        {
            serialize<attributes>(stream, bool(value[i]));
        }
        else
        {
            serialize<attributes>(stream, value[i]);
        }
    }
    stream.write(']');
    stream.put_comma = true;
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_repeated_fixed_size auto &value,
               std::string_view field)
{
    serialize_key(stream, field);
    serialize<attributes>(stream, value);
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_map auto &map, std::string_view field)
{
    if (map.empty())
        return;

    serialize_key(stream, field);
    serialize<attributes>(stream, map);
}

template <field_attributes attributes> void serialize(auto &stream, const spb::detail::proto_map auto &map)
{
    using map_type = std::remove_cvref_t<decltype(map)>;
    using key_type = typename map_type::key_type;

    stream.write('{');
    stream.put_comma = false;
    for (const auto &[map_key, map_value] : map)
    {
        if constexpr (std::is_same_v<key_type, std::string_view> || std::is_same_v<key_type, std::string>)
        {
            serialize_key(stream, map_key);
        }
        else
        {
            if (stream.put_comma)
                stream.write(',');

            stream.write('"');
            serialize<attributes>(stream, map_key);
            write_string(stream, R"(":)");
        }
        stream.put_comma = false;
        serialize<attributes>(stream, map_value);
        stream.put_comma = true;
    }
    stream.write('}');
    stream.put_comma = true;
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_optional auto &p_value)
{
    if (p_value.has_value())
        return serialize<attributes>(stream, *p_value);
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_label_optional auto &p_value, std::string_view field)
{
    if (p_value.has_value())
        return serialize<attributes>(stream, *p_value, field);
}

template <field_attributes attributes, typename T>
void serialize(auto &stream, const std::unique_ptr<T> &p_value)
{
    if (p_value)
        return serialize<attributes>(stream, *p_value);
}

template <field_attributes attributes, typename T>
void serialize(auto &stream, const std::unique_ptr<T> &p_value, std::string_view field)
{
    if (p_value)
        return serialize<attributes>(stream, *p_value, field);
}

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_message auto &value)
{
    stream.write('{');
    stream.put_comma = false;
    serialize_value(stream, value);
    stream.write('}');
    stream.put_comma = true;
}

template <field_attributes attributes>
void serialize(auto &stream, const spb::detail::proto_message auto &value, std::string_view field)
{
    serialize_key(stream, field);
    serialize<attributes>(stream, value);
}

template <field_attributes> void serialize(auto &stream, const spb::detail::proto_enum auto &value)
{
    serialize_value(stream, value);
}

template <field_attributes>
void serialize(auto &stream, const spb::detail::proto_enum auto &value, std::string_view field)
{
    serialize_key(stream, field);
    serialize_value(stream, value);
}

template <field_attributes attributes>
size_t serialize_size(const auto &value, const serialize_options &options)
{
    auto stream = ostream_size{options};
    serialize<attributes>(stream, value);
    return stream.size;
}

} // namespace spb::json::detail
