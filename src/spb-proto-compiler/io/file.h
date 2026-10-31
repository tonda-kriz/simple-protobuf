/*
 * Protobuf compiler - basic file IO
 *
 * Copyright (c) 2024 Antonin Kriz <antonin.kriz@gmail.com>
 * Released under the MIT License. See LICENSE in the project root.
 */

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

/**
 * @brief loads file from file_path
 *
 * @param file_path files path
 * @return file's content
 */
auto load_file(const std::filesystem::path &file_path) -> std::string;

/**
 * @brief save file_content to a file_path
 *
 * @param file_path files path
 * @param file_content files content
 */
void save_file(const std::filesystem::path &file_path, std::string_view file_content);
