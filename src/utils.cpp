#include <array>
#include <string>

#include "utils.h"

bool is_source_file(const fs::path &path) {
    static const std::array<std::string, 4> extensions{".c", ".cxx", ".cpp", ".cc"};
    return matches_extension(path, extensions.cbegin(), extensions.cend());
}

bool is_header_file(const fs::path &path) {
    static const std::array<std::string, 3> extensions{".h", ".hxx", ".hpp"};
    return matches_extension(path, extensions.cbegin(), extensions.cend());
}
