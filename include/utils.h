#pragma once

#include <algorithm>

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
