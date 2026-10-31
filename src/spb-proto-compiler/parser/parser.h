/*
 * Protobuf compiler - proto file parser
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include "ast/proto-file.h"
#include <filesystem>
#include <set>
#include <span>
#include <string_view>

/**
 * @brief generate C++ file name from a .proto file name
 *        example: "foo.proto" -> "foo.pb.h"
 *
 * @param proto_file_path proto file name
 * @param extension CPP file extension (example: ".pb.h")
 * @return CPP file name
 */
[[nodiscard]] auto cpp_file_name_from_proto(const std::filesystem::path &proto_file_path,
                                            std::string_view extension) -> std::filesystem::path;

/**
 * @brief parse proto file
 *
 * @param file_path file path
 * @param import_paths proto import paths (searched in order)
 * @param base_dir working directory
 * @return proto_file parsed proto
 */
auto parse_proto_file(const std::filesystem::path &file_path,
                      std::span<const std::filesystem::path> import_paths,
                      const std::filesystem::path &base_dir = std::filesystem::current_path()) -> proto_file;

namespace fs       = std::filesystem;
using parsed_files = std::set<std::string>;

struct parsing_ctx
{
    const fs::path &base_dir;
    parsed_files &already_parsed;
    std::span<const fs::path> import_paths;
};

void parse_proto_file_content(proto_file &file, parsing_ctx &ctx);
