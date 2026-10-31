/*
 * Protobuf compiler - C++ code generator for JSON
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "ast/proto-file.h"
#include <filesystem>

/**
 * @brief dump C++ header file for parsed proto
 *
 * @param file parsed proto
 * @param stream output stream
 * @return C++ header file for a ast
 */
void dump_json_header(const proto_file &file, std::ostream &stream);

/**
 * @brief dump C++ file for parsed proto
 *
 * @param file parsed proto
 * @param header_file generated C++ header file (ex: my.pb.h)
 * @param stream output stream
 */
void dump_json_cpp(const proto_file &file, const std::filesystem::path &header_file, std::ostream &stream);
