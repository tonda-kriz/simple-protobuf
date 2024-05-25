/***************************************************************************\
* Name        : serialize library for protobuf                              *
* Description : encoding in protobuf                                        *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/

#pragma once

#include <cstdint>

namespace sds::pb::detail
{

//- https://protobuf.dev/programming-guides/encoding/
enum class wire_type : uint8_t
{
    //- int32, int64, uint32, uint64, sint32, sint64, bool, enum
    varint = 0,
    //- fixed64, sfixed64, double
    fixed64 = 1,
    //- string, bytes, embedded messages, packed repeated fields
    length_delimited = 2,
    //- not used
    StartGroup = 3,
    //- not used
    EndGroup = 4,
    //- fixed32, sfixed32, float
    fixed32 = 5
};

enum class wire_encoder : uint8_t
{
    //- int32, int64, uint32, uint64, bool, enum
    varint = 0,
    //- 8 bytes
    fixed64 = 1,
    //- 4 bytes
    fixed32 = 5,
    //- zigzag int32 or int64
    svarint = 6,
};

}// namespace sds::pb::detail