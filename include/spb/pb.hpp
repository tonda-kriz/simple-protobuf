/***************************************************************************\
* Name        : Public API for protobuf                                     *
* Description : all protobuf serialize and deserialize functions            *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/
#pragma once

#include "pb/deserialize.hpp"
#include "pb/serialize.hpp"
#include "spb/io/io.hpp"
#include <algorithm>
#include <cstdint>

namespace spb::pb
{
/**
 * @brief serialize message via writer
 *
 * @param[in] message to be serialized
 * @param[in] on_write function for handling the writes
 * @return serialized size in bytes
 * @throws exceptions only from `on_write`
 */
static inline auto serialize( const auto & message, spb::io::writer on_write ) -> size_t
{
    return detail::serialize( message, on_write );
}

/**
 * @brief return protobuf serialized size in bytes
 *
 * @param[in] message to be serialized
 * @return serialized size in bytes
 */
[[nodiscard]] static inline auto serialize_size( const auto & message ) noexcept -> size_t
{
    return serialize( message, spb::io::writer( nullptr ) );
}

/**
 * @brief serialize message into protobuf
 *
 * @param[in] message to be serialized
 * @return serialized protobuf
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
 * @brief deserialize message from protobuf
 *
 * @param[in] reader function for handling reads
 * @param[out] message deserialized message
 * @throws std::runtime_error on error
 */
static inline void deserialize( auto & message, spb::io::reader reader )
{
    return detail::deserialize( message, reader );
}

/**
 * @brief deserialize message from protobuf
 *
 * @param[in] protobuf string with protobuf
 * @param[out] message deserialized message
 * @throws std::runtime_error on error
 */
static inline void deserialize( auto & message, std::string_view protobuf )
{
    auto reader = [ ptr = protobuf.data( ), end = protobuf.data( ) + protobuf.size( ) ]( void * data, size_t size ) mutable -> size_t
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
 * @brief deserialize message from protobuf
 *        use it like `auto message = spb::pb::deserialize< Message >( protobuf_str )`
 *
 * @param[in] protobuf string with protobuf
 * @return deserialized message
 * @throws std::runtime_error on error
 */
template < typename Message >
[[nodiscard]] static inline auto deserialize( std::string_view protobuf ) -> Message
{
    auto message = Message{ };
    deserialize( message, protobuf );
    return message;
}

/**
 * @brief deserialize message from reader
 *        use it like `auto message = spb::pb::deserialize< Message >( protobuf_str )`
 *
 * @param[in] reader function for handling reads
 * @return deserialized message
 * @throws std::runtime_error on error
 */
template < typename Message >
[[nodiscard]] static inline auto deserialize( spb::io::reader reader ) -> Message
{
    auto message = Message{ };
    deserialize( message, reader );
    return message;
}

}// namespace spb::pb
