#include <array>

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

void replace_pattern_with(std::string &str, std::string_view to_replace, std::string_view replace_with) {
    auto result = std::search(str.begin(), str.end(), std::cbegin(to_replace), std::cend(to_replace) - 1);

    if (result == str.cend()) {
        return;
    }

    str.replace(result, result + to_replace.size(), replace_with.cbegin(), replace_with.cend());
}

bool is_source_file(const fs::path &path) {
    static const std::array<std::string, 4> extensions{".c", ".cxx", ".cpp", ".cc"};
    return matches_extension(path, extensions.cbegin(), extensions.cend());
}

bool is_header_file(const fs::path &path) {
    static const std::array<std::string, 3> extensions{".h", ".hxx", ".hpp"};
    return matches_extension(path, extensions.cbegin(), extensions.cend());
}
