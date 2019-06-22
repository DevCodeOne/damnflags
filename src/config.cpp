#include <fstream>

#include "config.h"

// TODO add real config
std::string config::default_config() { return "{}"; }

std::string config::load_variables(const std::string &pattern) const {
    static constexpr char project_root_pattern[] = "${project_root}";
    std::string pattern_result(pattern);

    auto result = std::search(pattern_result.begin(), pattern_result.end(), std::cbegin(project_root_pattern),
                              std::cend(project_root_pattern) - 1);

    if (result == pattern_result.end()) {
        return pattern_result;
    }

    auto root_path = project_root();

    if (!root_path) {
        return pattern_result;
    }

    std::string_view root_path_view(root_path->c_str());
    pattern_result.replace(result, result + sizeof(project_root_pattern) - 1, root_path_view.cbegin(),
                           root_path_view.cend());
    return pattern_result;
}

// TODO actually implement this method with a json structure checker
bool config::is_config_valid(const nlohmann::json &conf) { return true; }

config config::create_empty_config() { return config{nlohmann::json{}}; }

std::optional<config> config::create_config(const nlohmann::json &conf) {
    if (!is_config_valid(conf)) {
        return std::nullopt;
    }

    return config{conf};
}

std::optional<config> config::load_config(const fs::path &config_path) {
    if (!fs::exists(config_path) && fs::is_regular_file(config_path)) {
        return std::nullopt;
    }

    std::ifstream config_in(config_path);

    if (!config_in) {
        return std::nullopt;
    }

    std::string read_config{std::istreambuf_iterator<char>(config_in), std::istreambuf_iterator<char>()};
    config_in.close();

    auto parsed_config = nlohmann::json::parse(read_config, nullptr, false);

    if (parsed_config == nlohmann::json::value_t::discarded) {
        return std::nullopt;
    }

    return create_config(parsed_config);
};

config::config(const nlohmann::json &conf) : m_conf(conf) { update_patterns(); }

void config::update_patterns() {
    m_prepared_blacklist.clear();
    m_prepared_whitelist.clear();

    try {
        auto blpatterns = blacklist_patterns();
        for (const auto &current_entry : blpatterns) {
            m_prepared_blacklist.emplace_back(load_variables(current_entry), std::regex::ECMAScript);
        }

        auto wlpatterns = whitelist_patterns();
        for (const auto &current_entry : wlpatterns) {
            m_prepared_whitelist.emplace_back(load_variables(current_entry), std::regex::ECMAScript);
        }
    } catch (std::regex_error e) {
        // TODO improve error handling here and log errors
        m_prepared_blacklist.clear();
        m_prepared_whitelist.clear();
    }
}

config &config::project_root(const fs::path &path) {
    m_conf[project_root_key] = path.c_str();
    update_patterns();
    return *this;
}

config &config::compilation_database_path(const fs::path &path) {
    m_conf[compilation_database_path_key] = path.c_str();
    return *this;
}

config &config::get_flags_from(const std::string &value) {
    m_conf[get_flags_from_key] = value;
    return *this;
}

config &config::whitelist_regex(const std::vector<std::string> &patterns) {
    m_conf[whitelist_patterns_key] = patterns;
    update_patterns();
    return *this;
}

config &config::blacklist_regex(const std::vector<std::string> &patterns) {
    m_conf[blacklist_patterns_key] = patterns;
    update_patterns();
    return *this;
}

// TODO remove code duplication
std::optional<fs::path> config::project_root() const {
    auto result = m_conf.find(project_root_key);

    if (result == m_conf.cend()) {
        return std::nullopt;
    }

    if (!result.value().is_string()) {
        return std::nullopt;
    }

    return fs::path(result.value().get<std::string>());
}

std::optional<fs::path> config::compilation_database_path() const {
    auto result = m_conf.find(compilation_database_path_key);

    if (result == m_conf.cend()) {
        return std::nullopt;
    }

    if (!result.value().is_string()) {
        return std::nullopt;
    }

    return fs::path(result.value().get<std::string>());
}

std::optional<std::string> config::get_flags_from() const {
    auto result = m_conf.find(get_flags_from_key);

    if (result == m_conf.cend()) {
        return std::nullopt;
    }

    if (!result.value().is_string()) {
        return std::nullopt;
    }

    return load_variables(result.value().get<std::string>());
}

const std::vector<std::regex> &config::prepared_blacklist_patterns() const { return m_prepared_blacklist; }

const std::vector<std::regex> &config::prepared_whitelist_patterns() const { return m_prepared_whitelist; }

std::vector<std::string> config::whitelist_patterns() const {
    auto result = m_conf.find(whitelist_patterns_key);

    if (result == m_conf.cend()) {
        return std::vector<std::string>();
    }

    if (!result.value().is_array()) {
        return std::vector<std::string>();
    }

    std::vector<std::string> patterns;
    bool valid = true;

    for (auto &current_entry : result.value()) {
        if (!current_entry.is_string()) {
            valid = false;
            break;
        }

        patterns.emplace_back(current_entry.get<std::string>());
    }

    if (!valid) {
        return std::vector<std::string>();
    }

    return std::move(patterns);
}

std::vector<std::string> config::blacklist_patterns() const {
    auto result = m_conf.find(blacklist_patterns_key);

    if (result == m_conf.cend()) {
        return {};
    }

    if (!result.value().is_array()) {
        return {};
    }

    std::vector<std::string> patterns;
    bool valid = true;

    for (auto &current_entry : result.value()) {
        if (!current_entry.is_string()) {
            valid = false;
            break;
        }

        patterns.emplace_back(current_entry.get<std::string>());
    }

    if (!valid) {
        return {};
    }

    return std::move(patterns);
}
