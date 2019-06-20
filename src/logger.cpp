#include "logger.h"

logger_configuration &logger_configuration::should_log_to_console(bool value) {
    m_should_log_to_console = value;

    return *this;
}

logger_configuration &logger_configuration::log_path(const fs::path &value) {
    m_log_path = value;

    return *this;
}

bool logger_configuration::should_log_to_console() const { return m_should_log_to_console; }

const fs::path &logger_configuration::log_path() const { return m_log_path; }

std::shared_ptr<logger> logger::instance(const std::optional<logger_configuration> config) {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance && config) {
        _instance = std::shared_ptr<logger>(new logger(*config));
    }

    return _instance;
}

// TODO implement
logger::logger(const logger_configuration &) {}
