/*
 * Protobuf compiler - C++ code generator for Protobuf and JSON
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "ast/proto-file.h"
#include "spb/io/function_ref.hpp"
#include <filesystem>

using message_dumper = spb::detail::function_ref<void(std::ostream &, const proto_file &,
                                                      const proto_message &, std::string_view)>;
using enum_dumper    = spb::detail::function_ref<void(std::ostream &, const proto_enum &, std::string_view)>;

void dump_cpp(std::ostream &stream, const proto_file &file, message_dumper dump_message,
              enum_dumper dump_enum);

/**
 * @brief dump C++ header file for parsed proto
 *
 * @param file parsed proto
 * @param header_file output file
 * @return C++ header file for a ast
 */
void dump_cpp_header(const proto_file &file, std::ostream &header_file);

/**
 * @brief dump C++ file for parsed proto
 *
 * @param file parsed proto
 * @param header_file generated C++ header file (ex: my.pb.h)
 * @param file_stream output file name (ex: my.pb.cpp)
 */
void dump_cpp(const proto_file &file, const std::filesystem::path &header_file, std::ostream &file_stream);
