#pragma once

#include <optional>
#include <regex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "filesys.h"

class config {
   public:
    static std::optional<config> load_config(const fs::path &config_path);
    static std::optional<config> create_config(const nlohmann::json &conf);
    static config create_empty_config();
    static bool is_config_valid(const nlohmann::json &conf);
    static std::string default_config();

    config &project_root(const fs::path &path);
    config &compilation_database_path(const fs::path &path);
    config &whitelist_regex(const std::vector<std::string> &regex);
    config &blacklist_regex(const std::vector<std::string> &regex);

    std::optional<fs::path> project_root() const;
    std::optional<fs::path> compilation_database_path() const;
    std::vector<std::string> whitelist_patterns() const;
    std::vector<std::string> blacklist_patterns() const;
    const std::vector<std::regex> &prepared_blacklist_patterns() const;
    const std::vector<std::regex> &prepared_whitelist_patterns() const;

   private:
    config(const nlohmann::json &conf);
    void update_patterns();

    std::string prepare_pattern(const std::string &pattern);

    static inline constexpr char project_root_key[] = "project_root";
    static inline constexpr char compilation_database_path_key[] = "compdb_path";
    static inline constexpr char whitelist_patterns_key[] = "whitelist_patterns";
    static inline constexpr char blacklist_patterns_key[] = "blacklist_patterns";

    nlohmann::json m_conf{};
    std::vector<std::regex> m_prepared_blacklist{};
    std::vector<std::regex> m_prepared_whitelist{};
};
