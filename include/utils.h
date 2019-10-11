#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "filesys.h"

template<typename T>
bool matches_extension(const fs::path &path, T begin, T end) {
    if (!path.has_extension()) {
        return false;
    }

    return std::find(begin, end, path.extension().c_str()) != end;
}

bool is_source_file(const fs::path &path);
bool is_header_file(const fs::path &path);
std::vector<std::string> split_command(const std::string &command_line);
void replace_pattern_with(std::string &str, std::string_view to_replace, std::string_view replace_with);
