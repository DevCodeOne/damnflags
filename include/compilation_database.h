#pragma once

#include <optional>
#include <set>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "config.h"
#include "filesys.h"

// TODO Merge compilation databases if multiple ones are available
class compilation_database final {
   public:
    static constexpr inline char database_name[] = "compile_commands.json";

    // TODO std::vector<fs::path> as argument for multiple compilation databases
    static std::optional<compilation_database> read_from(const fs::path &path);

    void swap(compilation_database &other) noexcept;

    bool write_to(const fs::path &compilation_database) const;

    bool add_missing_files(const std::set<fs::path> &relevant_files, const config &conf);

    const nlohmann::json &database() const;

   private:
    compilation_database(nlohmann::json database);

    nlohmann::json m_database;
};
