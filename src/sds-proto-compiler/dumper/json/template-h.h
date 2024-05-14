/***************************************************************************\
* Name        : json dumper                                                 *
* Description : C++ template used for json de/serialization                 *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include <string_view>

constexpr std::string_view file_json_header_template = R"(
//////////////////////////////////////////////////////////
//- Json serialize for $
//////////////////////////////////////////////////////////
namespace sds::json
{
/**
 * @brief return json-string serialized size in bytes
 *
 * @param value to be serialized
 * @return serialized size in bytes
 */
auto serialize_size( const $ & value ) noexcept -> size_t;

/**
 * @brief serialize value into json-string
 *
 * @param value to be serialized
 * @return serialized json
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
//- Json deserialize for $
//////////////////////////////////////////////////////////

/**
 * @brief deserialize json-string into variable
 *
 * @param json string with json
 * @param result deserialized json
 */
void deserialize( $ & result, std::string_view json );

template < typename result >
auto deserialize( std::string_view json ) -> result;

/**
 * @brief deserialize json-string into variable
 *        use it like `auto value = sds::json::deserialize< $ >( json_string )`
 *
 * @param json string with json
 * @return deserialized json or throw an exception
 */
template <>
auto deserialize< $ >( std::string_view json ) -> $;

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
void serialize_value( ostream & stream, const $ & value );

/**
 * @brief serialize value from internal stream.
 *
 * @param value to be deserialized
 * @param stream internal buffer
 */
auto deserialize_value( istream & stream, $ & value ) -> bool;

/**
 * @brief return true if value contains only empty fields
 *
 * @param value
 * @return true if value is empty
 */
auto is_empty( const $ & value ) noexcept -> bool;

}// namespace detail
}// namespace sds::json
)";
