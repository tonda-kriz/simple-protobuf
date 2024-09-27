/***************************************************************************\
* Name        : Public API for JSON                                         *
* Description : all json serialize and deserialize functions                *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/
#pragma once

#include "spb/io/io.hpp"
#include "json/deserialize.hpp"
#include "json/serialize.hpp"
#include <cstdint>

namespace spb::json
{

/**
 * @brief serialize message via writer
 *
 * @param message to be serialized
 * @param on_write function for handling the writes
 * @return serialized size in bytes
 * @throws exceptions only from `on_write`
 */
static inline auto serialize( const auto & message, spb::io::writer on_write ) -> size_t
{
    return detail::serialize( message, on_write );
}

/**
 * @brief return json-string serialized size in bytes
 *
 * @param message to be serialized
 * @return serialized size in bytes
 */
[[nodiscard]] static inline auto serialize_size( const auto & message ) noexcept -> size_t
{
    return serialize( message, spb::io::writer( nullptr ) );
}

/**
 * @brief serialize message into json-string
 *
 * @param message to be serialized
 * @return serialized json
 * @throws std::runtime_error on error
 */
[[nodiscard]] static inline auto serialize( const auto & message ) -> std::string
{
    auto result = std::string( serialize_size( message ), '\0' );
    auto writer = [ ptr = result.data( ) ]( const void * data, size_t size ) mutable
    {
        memcpy( ptr, data, size );
        ptr += size;
    };

    serialize( message, writer );
    return result;
}

/**
 * @brief deserialize json-string into variable
 *
 * @param on_read function for handling reads
 * @param result deserialized json
 * @throws std::runtime_error on error
 */
static inline void deserialize( auto & result, spb::io::reader on_read )
{
    return detail::deserialize( result, on_read );
}

/**
 * @brief deserialize json-string into variable
 *
 * @param json string with json
 * @param message deserialized json
 * @throws std::runtime_error on error
 */
static inline void deserialize( auto & message, std::string_view json )
{
    auto reader = [ ptr = json.data( ), end = json.data( ) + json.size( ) ]( void * data, size_t size ) mutable -> size_t
    {
        size_t bytes_left = end - ptr;

        size = std::min( size, bytes_left );
        memcpy( data, ptr, size );
        ptr += size;
        return size;
    };
    return deserialize( message, reader );
}

/**
 * @brief deserialize json-string into variable
 *        use it like `auto message = spb::json::deserialize< Message >( json_string )`
 *
 * @param json string with json
 * @return deserialized json or throw an exception
 * @throws std::runtime_error on error
 */
template < typename Message >
[[nodiscard]] static inline auto deserialize( std::string_view json ) -> Message
{
    auto message = Message{ };
    deserialize( message, json );
    return message;
}

/**
 * @brief deserialize json-string into variable
 *        use it like `auto message = spb::json::deserialize< Message >( reader )`
 *
 * @param on_read function for handling reads
 * @return deserialized json
 * @throws std::runtime_error on error
 */
template < typename Message >
[[nodiscard]] static inline auto deserialize( spb::io::reader on_read ) -> Message
{
    auto message = Message{ };
    return deserialize( message, on_read );
}

}// namespace spb::json
