/*
 * Protobuf serialization library - Public API for JSON.
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "spb/io/io.hpp"
#include "spb/json/field.hpp"
#include "json/deserialize.hpp"
#include "json/field.hpp"
#include "json/options.h"
#include "json/serialize.hpp"
#include <cstdlib>

namespace spb::json
{
/**
 * @brief serialize message via writer
 *
 * @param[in] message to be serialized
 * @param[in] on_write function for handling the writes
 * @param[in] options
 * @return serialized size in bytes
 * @throws exceptions only from `on_write`
 */
size_t serialize(const auto &message, spb::io::writer on_write, const serialize_options &options = {})
{
    auto stream = detail::ostream_writer{options, on_write};
    detail::serialize<detail::field_attributes{}>(stream, message);
    return stream.end();
}

/**
 * @brief return JSON serialized size in bytes
 *
 * @param[in] message to be serialized
 * @param[in] options
 * @return serialized size in bytes
 */
[[nodiscard]] size_t serialize_size(const auto &message, const serialize_options &options = {})
{
    return detail::serialize_size(message, options);
}

size_t serialize(const auto &message, void *buffer, const serialize_options &options = {})
{
    const auto start = (uint8_t *)buffer;
    auto stream      = detail::ostream_buffer{options, (uint8_t *)buffer};
    detail::serialize<detail::field_attributes{}>(stream, message);
    return stream.end(start);
}

/**
 * @brief serialize message into JSON
 *
 * @param[in] message to be serialized
 * @param[in] options
 * @param[out] result serialized JSON
 * @return serialized size in bytes
 * @throws std::runtime_error on error
 * @example `std::string json;`
 *          `spb::json::serialize(message, json);`
 */
template <spb::resizable_container Container>
size_t serialize(const auto &message, Container &result, const serialize_options &options = {})
{
    static_assert(sizeof(*result.data()) == sizeof(uint8_t));

    const auto size = detail::serialize_size(message, options);
    result.resize(size);
    return serialize(message, result.data(), options);
}

/**
 * @brief serialize message into JSON
 *
 * @param[in] message to be serialized
 * @param[in] options
 * @return serialized JSON
 * @throws std::runtime_error on error
 * @example `auto serialized_message = spb::json::serialize< std::vector< std::byte > >( message );`
 */
template <spb::resizable_container Container = std::string>
[[nodiscard]] Container serialize(const auto &message, const serialize_options &options = {})
{
    auto result = Container();
    serialize(message, result, options);
    return result;
}

size_t deserialize(auto &message, const void *buffer, size_t size)
{
    detail::istream_buffer stream((const uint8_t *)buffer, size);
    deserialize<detail::field_attributes{}>(stream, message);
    return size - stream.size();
}

/**
 * @brief deserialize message from JSON
 *
 * @param[in] reader function for handling reads
 * @param[in] options
 * @param[out] message deserialized message
 * @throws std::runtime_error on error
 */
size_t deserialize(auto &message, spb::io::reader reader)
{
    detail::istream_reader stream{reader};
    deserialize<detail::field_attributes{}>(stream, message);
    return stream.consumed_size();
}

/**
 * @brief deserialize message from JSON
 *
 * @param[in] JSON string with JSON
 * @param[in] options
 * @param[out] message deserialized message
 * @throws std::runtime_error on error
 * @example `auto serialized = std::vector<std::byte>( ... );`
 *          `auto message = Message();`
 *          `spb::json::deserialize(message, serialized);`
 */
size_t deserialize(auto &message, const spb::size_container auto &json)
{
    return deserialize(message, json.data(), json.size());
}

/**
 * @brief deserialize message from JSON
 *
 * @param[in] JSON serialized JSON
 * @param[in] options
 * @return deserialized message
 * @throws std::runtime_error on error
 * @example `auto serialized = std::vector< std::byte >( ... );`
 *          `auto message = spb::json::deserialize< Message >( serialized );`
 */
template <typename Message> [[nodiscard]] Message deserialize(const spb::size_container auto &json)
{
    auto message = Message{};
    deserialize(message, json.data(), json.size());
    return message;
}

/**
 * @brief deserialize message from reader
 *
 * @param[in] reader function for handling reads
 * @param[in] options
 * @return deserialized message
 * @throws std::runtime_error on error
 */
template <typename Message> [[nodiscard]] Message deserialize(spb::io::reader reader)
{
    auto message = Message{};
    deserialize(message, reader);
    return message;
}
} // namespace spb::json
