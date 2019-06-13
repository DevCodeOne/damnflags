#include <array>
#include <string>

#include "utils.h"

std::vector<std::string> split_command(const std::string &command_line) {
    std::vector<std::string> parts;
    std::string current_part;
    std::istringstream command_line_input(command_line);

    while (std::getline(command_line_input, current_part, ' ')) {
            auto new_end = std::remove_if(current_part.begin(), current_part.end(),
                                          [](const auto &current_char) { return std::isspace(current_char); });

        if (new_end != current_part.cend()) {
            current_part.erase(new_end);
        }

        if (current_part.size() == 0) {
            continue;
        }
        parts.emplace_back(current_part);
    }

    return std::move(parts);
}

bool is_source_file(const fs::path &path) {
    static const std::array<std::string, 4> extensions{".c", ".cxx", ".cpp", ".cc"};
    return matches_extension(path, extensions.cbegin(), extensions.cend());
}

bool is_header_file(const fs::path &path) {
    static const std::array<std::string, 3> extensions{".h", ".hxx", ".hpp"};
    return matches_extension(path, extensions.cbegin(), extensions.cend());
}
