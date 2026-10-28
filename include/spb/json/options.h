/***************************************************************************\
* Name        : serialize library for protobuf                              *
* Description : JSON serialize options                                      *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

namespace spb::json
{
struct serialize_options
{
    /**
     * @brief Quote 64-bit integers that cannot be represented exactly as a double, preventing precision loss
     * in consumers such as JavaScript that parse JSON numbers as doubles.
     * It will output {"value":"18446744073709551614"} instead of {"value":18446744073709551614}
     */
    bool quote_inexact_int64 = false;

    /**
     * @brief Adds whitespace, indentation, and line breaks to make it human-readable.
     */
    bool pretty_print = false;
};

} // namespace spb::json
