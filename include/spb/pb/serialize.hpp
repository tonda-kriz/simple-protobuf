/***************************************************************************\
* Name        : serialize library for protobuf                              *
* Description : all protobuf serialization functions                        *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include "../concepts.h"
#include "wire-types.h"
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <spb/io/io.hpp>
#include <sys/types.h>
#include <type_traits>

namespace spb::pb::detail
{
struct ostream
{
  private:
    size_t bytes_written = 0;
    spb::io::writer on_write;

  public:
    /**
     * @brief Construct a new ostream object
     *
     * @param writer if null, stream will skip all writes but will still count number of written
     * bytes
     */
    explicit ostream(spb::io::writer writer = nullptr) noexcept : on_write(writer)
    {
    }

    void write(const void *p_data, size_t size)
    {
        if (on_write)
            on_write(p_data, size);

        bytes_written += size;
    }

    [[nodiscard]] auto size() const noexcept -> size_t
    {
        return bytes_written;
    }

    void serialize(const field_attributes &field, const auto &value);

    template <scalar_encoder encoder> void serialize_as(const field_attributes &field, const auto &value);
};

inline auto serialize_size(const auto &value) -> size_t;
template <scalar_encoder encoder> inline auto serialize_size_as(const auto &value) -> size_t;

using namespace std::literals;

inline void serialize_varint(ostream &stream, uint64_t value)
{
    uint8_t i = 0;
    uint8_t buffer[10];

    while (value >= 0x80)
    {
        buffer[i++] = (uint8_t)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    buffer[i++] = (uint8_t)(value);

    return stream.write(buffer, i);
}
inline void serialize_svarint(ostream &stream, int64_t value)
{
    const auto tmp = uint64_t((value << 1) ^ (value >> 63));
    return serialize_varint(stream, tmp);
}

inline void serialize_tag(ostream &stream, uint32_t field_number, wire_type type)
{
    const auto tag = (field_number << 3) | uint32_t(type);
    serialize_varint(stream, tag);
}

inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_message auto &value);
inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_field_string auto &value);
inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_field_bytes auto &value);
inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_label_repeated auto &value);

template <scalar_encoder encoder, spb::detail::proto_label_repeated C>
inline void serialize_as(ostream &stream, const field_attributes &field, const C &value);

template <scalar_encoder encoder, spb::detail::proto_label_repeated_fixed_size C>
inline void serialize_as(ostream &stream, const field_attributes &field, const C &value);

template <scalar_encoder encoder, typename keyT, typename valueT>
inline void serialize_as(ostream &stream, const field_attributes &field, const std::map<keyT, valueT> &value);

template <scalar_encoder encoder>
inline void serialize_as(ostream &stream, const field_attributes &field,
                         spb::detail::proto_field_number auto value)
{
    serialize_tag(stream, field.number, wire_type_from_scalar_encoder(encoder));
    serialize_as<encoder>(stream, value);
}

template <scalar_encoder encoder>
inline void serialize_as(ostream &stream, const spb::detail::proto_enum auto &value)
{
    serialize_varint(stream, int32_t(value));
}

template <scalar_encoder encoder>
inline void serialize_as(ostream &stream, spb::detail::proto_field_int_or_float auto value)
{
    using T = std::remove_cvref_t<decltype(value)>;

    const auto type = scalar_encoder_type1(encoder);

    if constexpr (type == scalar_encoder::varint)
    {
        static_assert(std::is_integral_v<T>);

        if constexpr (std::is_same_v<bool, T>)
        {
            const uint8_t tmp = value ? 1 : 0;
            return stream.write(&tmp, 1);
        }
        else if constexpr (std::is_signed_v<T>)
        {
            //- GPB is serializing all negative ints always as int64_t
            const auto u_value = uint64_t(int64_t(value));
            return serialize_varint(stream, u_value);
        }
        else
        {
            return serialize_varint(stream, value);
        }
    }
    else if constexpr (type == scalar_encoder::svarint)
    {
        static_assert(std::is_signed_v<T> && std::is_integral_v<T>);

        return serialize_svarint(stream, value);
    }
    else if constexpr (type == scalar_encoder::i32)
    {
        if constexpr (sizeof(value) == sizeof(uint32_t))
        {
            return stream.write(&value, sizeof(value));
        }
        else
        {
            const auto tmp = uint32_t(value);
            return stream.write(&tmp, sizeof(tmp));
        }
    }
    else if constexpr (type == scalar_encoder::i64)
    {
        if constexpr (sizeof(value) == sizeof(uint64_t))
        {
            return stream.write(&value, sizeof(value));
        }
        else
        {
            const auto tmp = uint64_t(value);
            return stream.write(&tmp, sizeof(tmp));
        }
    }
}

inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_field_string auto &value)
{
    if (value.empty())
        return;

    if (field.max_size) [[unlikely]]
        check_max_size(field, value.size());

    serialize_tag(stream, field.number, wire_type::length_delimited);
    serialize_varint(stream, value.size());
    stream.write(value.data(), value.size());
}

inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_field_bytes auto &value)
{
    if (value.empty())
        return;

    if (field.max_size) [[unlikely]]
        check_max_size(field, value.size());

    serialize_tag(stream, field.number, wire_type::length_delimited);
    serialize_varint(stream, value.size());
    stream.write(value.data(), value.size());
}

template <scalar_encoder encoder, typename keyT, typename valueT>
inline void serialize_as(ostream &stream, const std::map<keyT, valueT> &value)
{
    const auto key_encoder = scalar_encoder_type1(encoder);
    const auto value_encoder = scalar_encoder_type2(encoder);

    for (const auto &[k, v] : value)
    {
        if constexpr (std::is_integral_v<keyT>)
        {
            serialize_as<key_encoder>(stream, field_attributes{.number = 1}, k);
        }
        else
        {
            serialize(stream, field_attributes{.number = 1}, k);
        }
        if constexpr (spb::detail::proto_field_number<valueT>)
        {
            serialize_as<value_encoder>(stream, field_attributes{.number = 2}, v);
        }
        else
        {
            serialize(stream, field_attributes{.number = 2}, v);
        }
    }
}

template <scalar_encoder encoder, typename keyT, typename valueT>
inline void serialize_as(ostream &stream, const field_attributes &field, const std::map<keyT, valueT> &value)
{
    const auto size = serialize_size_as<encoder>(value);

    serialize_tag(stream, field.number, wire_type::length_delimited);
    serialize_varint(stream, size);
    serialize_as<encoder>(stream, value);
}

template <scalar_encoder encoder, spb::detail::proto_label_repeated_fixed_size C>
inline void serialize_packed_as(ostream &stream, const C &container)
{
    for (size_t i = 0; i < container.size(); i++)
    {
        if constexpr (std::is_same_v<typename C::value_type, bool>)
        {
            serialize_as<encoder>(stream, bool(container[i]));
        }
        else
        {
            serialize_as<encoder>(stream, container[i]);
        }
    }
}

template <scalar_encoder encoder, spb::detail::proto_label_repeated C>
inline void serialize_packed_as(ostream &stream, const C &container)
{
    for (const auto &v : container)
    {
        if constexpr (std::is_same_v<typename C::value_type, bool>)
        {
            serialize_as<encoder>(stream, bool(v));
        }
        else
        {
            serialize_as<encoder>(stream, v);
        }
    }
}

template <scalar_encoder encoder, spb::detail::proto_label_repeated C>
inline void serialize_as(ostream &stream, const field_attributes &field, const C &value)
{
    if (field.max_count) [[unlikely]]
        check_max_count(field, value.size());

    if constexpr (scalar_encoder_is_packed(encoder))
    {
        if (value.empty())
            return;

        const auto size = serialize_size_as<encoder>(value);

        serialize_tag(stream, field.number, wire_type::length_delimited);
        serialize_varint(stream, size);
        serialize_packed_as<encoder>(stream, value);
    }
    else
    {
        for (const auto &v : value)
        {
            if constexpr (std::is_same_v<typename C::value_type, bool>)
            {
                serialize_as<encoder>(stream, field, bool(v));
            }
            else
            {
                serialize_as<encoder>(stream, field, v);
            }
        }
    }
}

template <scalar_encoder encoder, spb::detail::proto_label_repeated_fixed_size C>
inline void serialize_as(ostream &stream, const field_attributes &field, const C &value)
{
    static_assert(scalar_encoder_is_packed(encoder),
                  "repeated field with fixed size has to have attribute 'packed'");

    const auto size = serialize_size_as<encoder>(value);

    serialize_tag(stream, field.number, wire_type::length_delimited);
    serialize_varint(stream, size);
    serialize_packed_as<encoder>(stream, value);
}

inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_label_repeated auto &value)
{
    if (field.max_count) [[unlikely]]
        check_max_count(field, value.size());

    for (const auto &v : value)
    {
        if constexpr (std::is_same_v<typename std::decay_t<decltype(value)>::value_type, bool>)
        {
            serialize_as<scalar_encoder::varint>(stream, field, bool(v));
        }
        else
        {
            serialize(stream, field, v);
        }
    }
}

inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_label_optional auto &p_value)
{
    if (p_value.has_value())
        return serialize(stream, field, *p_value);
}

template <scalar_encoder encoder>
inline void serialize_as(ostream &stream, const field_attributes &field,
                         const spb::detail::proto_label_optional auto &p_value)
{
    if (p_value.has_value())
        return serialize_as<encoder>(stream, field, *p_value);
}

template <typename T>
inline void serialize(ostream &stream, const field_attributes &field, const std::unique_ptr<T> &p_value)
{
    if (p_value)
        return serialize(stream, field, *p_value);
}

inline void serialize(ostream &stream, const field_attributes &field,
                      const spb::detail::proto_message auto &value)
{
    const auto size = serialize_size(value);
    if (!size) [[unlikely]]
        return;

    serialize_tag(stream, field.number, wire_type::length_delimited);
    serialize_varint(stream, size);
    serialize_value(stream, value);
}

inline auto serialize(const auto &value, spb::io::writer on_write) -> size_t
{
    auto stream = ostream(on_write);
    serialize(stream, value);
    return stream.size();
}

inline auto serialize_size(const auto &value) -> size_t
{
    auto stream = ostream(nullptr);
    serialize_value(stream, value);
    return stream.size();
}

template <scalar_encoder encoder> inline auto serialize_size_as(const auto &value) -> size_t
{
    auto stream = ostream();
    if constexpr (scalar_encoder_is_packed(encoder))
    {
        serialize_packed_as<encoder>(stream, value);
    }
    else
    {
        serialize_as<encoder>(stream, value);
    }
    return stream.size();
}

void ostream::serialize(const field_attributes &field, const auto &value)
{
    detail::serialize(*this, field, value);
}

template <scalar_encoder encoder> void ostream::serialize_as(const field_attributes &field, const auto &value)
{
    detail::serialize_as<encoder>(*this, field, value);
}

} // namespace spb::pb::detail