/***************************************************************************\
* Name        : pb dumper                                                   *
* Description : C++ template used for pb de/serialization                   *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include <string_view>

constexpr std::string_view file_pb_header_template = R"(
//////////////////////////////////////////////////////////
//- Protobuf serialize for $
//////////////////////////////////////////////////////////
namespace sds::pb
{
/**
 * @brief return protobuf serialized size in bytes
 *
 * @param value to be serialized
 * @return serialized size in bytes
 */
auto serialize_size( const $ & value ) noexcept -> size_t;

/**
 * @brief serialize value into protobuf
 *
 * @param value to be serialized
 * @return serialized protobuf
 */
auto serialize( const $ & value ) -> std::string;

/**
 * @brief serialize value into buffer.
 *        Warning: user is responsible for the buffer to be big enough.
 *        Use `auto buffer_size = serialize_size( value );` to obtain the buffer size
 *
 * @param value to be serialized
 * @param buffer buffer with atleast serialize_size(value) bytes
 * @return serialized size in bytes
 */
auto serialize( const $ & value, void * buffer ) -> size_t;

//////////////////////////////////////////////////////////
//- protobuf deserialize for $
//////////////////////////////////////////////////////////

/**
 * @brief deserialize protobuf into variable
 *
 * @param protobuf string with protobuf
 * @param result deserialized protobuf
 */
void deserialize( $ & result, std::string_view protobuf );

template < typename result >
auto deserialize( std::string_view protobuf ) -> result;

/**
 * @brief deserialize protobuf into variable
 *        use it like `auto value = sds::pb::deserialize< $ >( proto_string )`
 *
 * @param protobuf string with protobuf
 * @return deserialized protobuf or throw an exception
 */
template <>
auto deserialize< $ >( std::string_view protobuf ) -> $;

namespace detail
{
struct ostream;
struct istream;

/**
 * @brief serialize value into internal stream.
 *
 * @param value to be serialized
 * @param stream internal buffer
 */
void serialize( ostream & stream, const $ & value );

/**
 * @brief serialize value from internal stream.
 *
 * @param value to be deserialized
 * @param tag field tag and wire type
 * @param stream internal buffer
 */
void deserialize_value( istream & stream, $ & value, uint32_t tag );

/**
 * @brief return true if value contains only empty fields
 *
 * @param value
 * @return true if value is empty
 */
auto is_empty( const $ & value ) noexcept -> bool;

}// namespace detail
}// namespace sds::pb
)";
