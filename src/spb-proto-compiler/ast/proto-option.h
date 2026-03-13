/***************************************************************************\
* Name        : proto option                                                *
* Description : data structure for an single option                         *
* Author      : antonin.kriz@gmail.com                                      *
* ------------------------------------------------------------------------- *
* This is free software; you can redistribute it and/or modify it under the *
* terms of the MIT license. A copy of the license can be found in the file  *
* "LICENSE" at the root of this distribution.                               *
\***************************************************************************/
#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

using proto_options = std::unordered_map< std::string_view, std::string_view >;

struct enum_options
{
    // enum type: one of int8, uint8, int16, uint16, int32, uint32, int64, uint64
    // default: "int32"
    std::string_view type;
};

struct spb_options
{
    // maximum size for `repeated`, `string` or `bytes`
    // de/serialize will return false if `real_size > max_size`
    std::optional< uint32_t > max_size;
    // field's type: one of int8, uint8, int16, uint16, int32, uint32, int64, uint64
    // can be combined with an bit field: `int8:1`, `uint8:2`, ...
    // warning: due to compatibility with GPB, always use int types with the same sign, like `int32`
    // and `int8`, combinations like `int32` and `uint8` or `float and `int32` are invalid.
    std::string_view type;
    // enum type, one of int8, uint8, int16, uint16, int32, uint32, int64, uint64
    // default: "int32"
    std::string_view enum_;
    // extra files to include in generated `.pb.h`
    std::set< std::string_view > include;
    // files to exclude from includes in generated `.pb.h`
    std::set< std::string_view > exclude;
    // container type for optional label
    // default: "std::optional<$>"
    std::string_view optional;
    // container type for optional label if there is cyclic dependency between messages (A -> B, B
    // -> A) default: "std::unique_ptr<$>"
    std::string_view pointer;
    // container type for repeated label
    // default: "std::vector<$>", `$` will be replaced by a field's type
    std::string_view repeated;
    // container type for bytes type
    // default: "std::vector<$>", `$` will be replaced with `std::byte`
    std::string_view bytes;
    // container type for string type
    // default: "std::string"
    std::string_view string;
    // field name used for json de/serializer
    // default: <none>
    std::string_view json_name;
    // [[deprecated]] attribute
    bool deprecated = false;
    // packed attribute for an array or message
    std::optional< bool > packed;
    // generate `spb::json::deserialize()`
    // default: true
    std::optional< bool > json_deserialize;
    // generate `spb::json::serialize()`
    // default: true
    std::optional< bool > json_serialize;
    // generate `spb::pb::serialize()`
    // default: true
    std::optional< bool > pb_deserialize;
    // generate `spb::pb::deserialize()`
    // default: true
    std::optional< bool > pb_serialize;
};

/*
using option_name = std::string_view;
using proto_value =
    std::variant< std::nullptr_t, bool, double, int64_t, std::string_view,
                  std::vector< proto_value >, std::unordered_map< option_name, proto_value > >;

using proto_struct = std::unordered_map< option_name, proto_value >;
*/