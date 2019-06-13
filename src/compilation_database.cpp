#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

#include "compilation_database.h"
#include "utils.h"

std::optional<compilation_database> compilation_database::read_from(const fs::path &path) {
    if (!fs::exists(path) && fs::is_regular_file(path)) {
        return std::nullopt;
    }

    std::ifstream compilation_database_in(path);

    if (!compilation_database_in) {
        return std::nullopt;
    }

    std::string read_compilation_database{std::istreambuf_iterator<char>(compilation_database_in),
                                          std::istreambuf_iterator<char>()};
    compilation_database_in.close();

    auto result = nlohmann::json::parse(read_compilation_database, nullptr, false);

    if (result == nlohmann::json::value_t::discarded) {
        return std::nullopt;
    }

    return compilation_database{result};
}

compilation_database::compilation_database(nlohmann::json database) : m_database(std::move(database)) {}

void compilation_database::swap(compilation_database &other) noexcept { m_database.swap(other.m_database); }

const nlohmann::json &compilation_database::database() const { return m_database; }

bool compilation_database::write_to(const fs::path &path) const {
    if (!fs::exists(path) && fs::is_regular_file(path)) {
        return false;
    }

    std::ofstream compilation_database_out(path);

    if (!compilation_database_out) {
        return false;
    }

    std::ostreambuf_iterator<char> out(compilation_database_out);
    std::string compilation_database_str = m_database.dump();
    std::copy(compilation_database_str.cbegin(), compilation_database_str.cend(), out);

    return true;
}

#include <iostream>

bool compilation_database::add_missing_files(const std::set<fs::path> &relevant_files) {
    if (!m_database.is_array() || m_database.size() < 1) {
        return false;
    }

    // Use multimap for filename collisions
    std::map<fs::path, nlohmann::json> file_map;
    std::vector<std::string> common_command;
    bool added_files = false;

    auto first_entry = m_database.front();

    if (auto command = first_entry.find("command");
        command != first_entry.end() || command->type() != nlohmann::json::value_t::string) {
        std::string current_part;
        std::istringstream flag_stream(command->get<std::string>());

        while (std::getline(flag_stream, current_part, ' ')) {
            auto new_end = std::remove_if(current_part.begin(), current_part.end(),
                                          [](const auto &current_char) { return std::isspace(current_char); });

            if (new_end != current_part.cend()) {
                current_part.erase(new_end);
            }

            if (current_part.size() == 0) {
                continue;
            }

            common_command.emplace_back(current_part);
        }
    }

    if (common_command.size() > 1) {
        // TODO Filter wrong flags

        auto new_end = std::remove_if(common_command.begin() + 1, common_command.end(), [](const auto &current_flag) {
            if (current_flag.size() == 0) {
                return true;
            }

            if ((current_flag[0] != '-') || (current_flag.find("-o") == 0) || (current_flag.find("-c") == 0)) {
                std::cout << "Erasing : " << current_flag << std::endl;
                return true;
            }

            return false;
        });

        if (new_end != common_command.end()) {
            common_command.erase(new_end, common_command.end());
        }
    }

    for (const auto &current_entry : m_database) {
        if (auto filename = current_entry.find("file"); filename != current_entry.end()) {
            auto filename_wo_extension = fs::path(filename->get<std::string>()).filename().replace_extension();
            file_map[filename_wo_extension] = current_entry;
        }
    }

    for (const auto &current_entry : relevant_files) {
        if (is_header_file(current_entry)) {
            auto filename_wo_extension = fs::path(current_entry).filename().replace_extension();
            auto result = file_map.find(filename_wo_extension);

            nlohmann::json new_entry;

            new_entry["file"] = current_entry;
            if (result != file_map.cend()) {
                new_entry["directory"] = result->second["directory"];
                // TODO Filter incorrect flags
                new_entry["command"] = result->second["command"];
            } else {
                std::cout << "Couldn't find a match for header " << current_entry
                          << " ... trying to guess flags from common ones " << std::endl;
                std::ostringstream command_line;
                std::copy(common_command.cbegin(), common_command.cend(),
                          std::ostream_iterator<std::string>(command_line, " "));

                std::cout << "Command line : " << command_line.str() << std::endl;

                // TODO Add something different here
                new_entry["directory"] = first_entry["directory"];
                new_entry["command"] = command_line.str();
            }

            m_database.emplace_back(new_entry);
            added_files = true;
        }
    }

    return added_files;
}